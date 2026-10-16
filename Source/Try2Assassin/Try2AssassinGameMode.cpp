#include "Try2AssassinGameMode.h"
#include "Try2AssassinGameState.h"
#include "GameFramework/GameModeBase.h"
#include "BaseCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "VillagerCharacter.h"
#include "UObject/SoftObjectPath.h"
#include "NavigationSystem.h"
#include "TargetCharacter.h"
#include "GuardCharacter.h"
#include "GuardAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Engine/TargetPoint.h"
#include "EscapeZone.h"

ATry2AssassinGameMode::ATry2AssassinGameMode()
{
	GameStateClass = ATry2AssassinGameState::StaticClass();

	// Load Village
	FSoftClassPath ClassPath(
		TEXT("/Game/MyAsset/AI/BP_Villager.BP_Villager_C")
	);

	UClass* LoadedClass = ClassPath.TryLoadClass<AVillagerCharacter>();

	if (LoadedClass)
	{
		VillagerClass = LoadedClass;
	}
	else
	{
		UE_LOG(LogTemp, Error,
		       TEXT("Cannot load BP_Villager. Check parent class and asset path."));
	}

	// Load Target
	FSoftClassPath TargetClassPath(
		TEXT("/Game/MyAsset/AI/BP_Target.BP_Target_C")
	);

	UClass* LoadedTargetClass =
		TargetClassPath.TryLoadClass<ATargetCharacter>();

	if (LoadedTargetClass)
	{
		TargetClass = LoadedTargetClass;
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Failed to load BP_Target")
		);
	}

	// Load Guard
	FSoftClassPath GuardClassPath(
		TEXT("/Game/MyAsset/AI/BP_Guard.BP_Guard_C")
	);

	UClass* LoadedGuardClass =
		GuardClassPath.TryLoadClass<AGuardCharacter>();

	if (LoadedGuardClass)
	{
		GuardClass = LoadedGuardClass;
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Failed to load BP_Guard")
		);
	}

	// เปลี่ยน Default Actor เป็น ตัวที่สร้าง
	// หา Blueprint Class ของ Pawn ที่อยู่ใน Content Browser
	// หมายเหตุ: พาธต้องลงท้ายด้วย _C เสมอ
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/MyAsset/BP_BaseThirdPersonCharacter.BP_BaseThirdPersonCharacter_C"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void ATry2AssassinGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// หา TargetPoint / EscapeZone ทั้งหมดครั้งเดียว แล้ว cache ไว้ให้ AI ทุกตัวใช้ร่วมกัน
		// (เดิมแต่ละ AI สแกนโลกเอง -> เปลือง CPU)
		CacheWorldActors();

		// ทยอย spawn AI ทีละตัวด้วย timer แทนการ spawn 30 ตัวรวดในเฟรมเดียว
		// ซึ่งเดิมทำให้เกมกระตุกตอนเริ่มด่าน
		GetWorldTimerManager().SetTimer(
			SpawnQueueTimer,
			this,
			&ATry2AssassinGameMode::ProcessSpawnQueue,
			SpawnInterval,
			/*bLoop=*/true,
			/*FirstDelay=*/SpawnInterval
		);
	}

	// ชั่วคราวสำหรับทดสอบ: เริ่ม Assassination หลัง 2 วินาที
	FTimerHandle TH;
	GetWorldTimerManager().SetTimer(
		TH,
		this,
		&ATry2AssassinGameMode::StartAssassinationPhase,
		2.0f,
		false
	);
}

void ATry2AssassinGameMode::CacheWorldActors()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	// PatrolPoint (TargetPoint) ทั้งหมด — หาครั้งเดียวให้ AI ทุกตัวใช้ร่วมกัน
	CachedPatrolPoints.Reset();
	TArray<AActor*> FoundTargetPoints;
	UGameplayStatics::GetAllActorsOfClass(
		World,
		ATargetPoint::StaticClass(),
		FoundTargetPoints
	);
	for (AActor* Point : FoundTargetPoints)
	{
		if (IsValid(Point))
		{
			CachedPatrolPoints.Add(Point);
		}
	}

	// EscapeZone ทั้งหมด — หาครั้งเดียวให้ชาวบ้านทุกตัวใช้ร่วมกันตอนหนี
	CachedEscapeZones.Reset();
	TArray<AActor*> FoundZones;
	UGameplayStatics::GetAllActorsOfClass(
		World,
		AEscapeZone::StaticClass(),
		FoundZones
	);
	for (AActor* Zone : FoundZones)
	{
		if (AEscapeZone* EscapeZone = Cast<AEscapeZone>(Zone))
		{
			CachedEscapeZones.Add(EscapeZone);
		}
	}

	UE_LOG(LogTemp, Log,
	       TEXT("Cached world actors: %d PatrolPoints, %d EscapeZones"),
	       CachedPatrolPoints.Num(), CachedEscapeZones.Num());
}

void ATry2AssassinGameMode::ProcessSpawnQueue()
{
	if (!HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(SpawnQueueTimer);
		return;
	}

	UNavigationSystemV1* NavSystem =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	// Nav ยังไม่พร้อม ลองใหม่รอบหน้า (กันค้าง: ยอมแพ้หลังลอง ~10 วินาที)
	if (!NavSystem)
	{
		if (++NavRetryCount > 200)
		{
			UE_LOG(LogTemp, Error,
			       TEXT("Spawn: NavMesh not available, aborting AI spawn"));
			GetWorldTimerManager().ClearTimer(SpawnQueueTimer);
		}
		return;
	}
	NavRetryCount = 0;

	// ทำทีละตัวต่อการเรียก 1 ครั้ง เพื่อกระจายภาระหลายเฟรม

	// 1) ชาวบ้าน
	if (SpawnedVillagers < VillagerCount && VillagerClass)
	{
		FNavLocation RandomLocation;
		if (NavSystem->GetRandomReachablePointInRadius(
			SpawnCenter, SpawnRadius, RandomLocation))
		{
			FVector SpawnLocation = RandomLocation.Location;
			SpawnLocation.Z += 50.0f;
			SpawnVillager(VillagerClass, SpawnLocation);
		}
		++SpawnedVillagers; // นับ attempt เสมอ เพื่อให้คิวจบแน่นอน
		return;
	}

	// 2) Guard
	if (SpawnedGuards < GuardCount && GuardClass)
	{
		FNavLocation RandomLocation;
		if (NavSystem->GetRandomReachablePointInRadius(
			SpawnCenter, SpawnRadius, RandomLocation))
		{
			FVector SpawnLocation = RandomLocation.Location;
			SpawnLocation.Z += 30.0f;
			SpawnGuard(GuardClass, SpawnLocation);
		}
		++SpawnedGuards;
		return;
	}

	// 3) Target (1 ตัว)
	if (bSpawnTarget && TargetClass && !bTargetSpawned)
	{
		FNavLocation RandomLocation;
		if (NavSystem->GetRandomReachablePointInRadius(
			SpawnCenter, SpawnRadius, RandomLocation))
		{
			FVector SpawnLocation = RandomLocation.Location;
			SpawnLocation.Z += 30.0f;
			SpawnTarget(TargetClass, SpawnLocation);
		}
		bTargetSpawned = true;
		return;
	}

	// spawn ครบหมดแล้ว หยุด timer
	GetWorldTimerManager().ClearTimer(SpawnQueueTimer);
	UE_LOG(LogTemp, Log,
	       TEXT("All AI spawned (Guards=%d)"), GuardActors.Num());
}

AEscapeZone* ATry2AssassinGameMode::FindNearestEscapeZone(const FVector& From) const
{
	AEscapeZone* Nearest = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AEscapeZone* Zone : CachedEscapeZones)
	{
		if (!IsValid(Zone)) { continue; }

		const float D = FVector::DistSquared(Zone->GetActorLocation(), From);
		if (D < BestDistSq)
		{
			BestDistSq = D;
			Nearest = Zone;
		}
	}

	return Nearest;
}

void ATry2AssassinGameMode::StartAssassinationPhase()
{
	if (auto* GS = GetGameState<ATry2AssassinGameState>())
	{
		GS->SetMatchPhase(EMatchPhase::Assassination);
	}
}

void ATry2AssassinGameMode::HandleTargetKilled(AController* Killer)
{
	if (!HasAuthority())
	{
		return;
	}

	ATry2AssassinGameState* GS = GetGameState<ATry2AssassinGameState>();
	if (!GS)
	{
		return;
	}

	if (GS->MatchPhase != EMatchPhase::Assassination && GS->MatchPhase != EMatchPhase::Escape)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleTargetKilled rejected: MatchPhase is not valid"));
		return;
	}

	if (GS->bTargetKilled && GS->MatchPhase != EMatchPhase::Escape)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("HandleTargetKilled rejected: Target already killed")
		);
		return;
	}

	APlayerState* KillerPlayerState = nullptr;

	if (Killer)
	{
		KillerPlayerState = Killer->GetPlayerState<APlayerState>();

		ABaseCharacter* KillerPlayer = Cast<ABaseCharacter>(Killer->GetCharacter());
		if (KillerPlayer)
		{
			// ทำให้ผู้เล่นโดน takedown ได้จากอีกคน
			KillerPlayer->bCanBeTakedown = true;
		}
	}

	if (!KillerPlayerState)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("HandleTargetKilled rejected: KillerPlayerState is NULL")
		);
		return;
	}

	// คนฆ่าเข้าสู่สถานะ Escaping
	if (ABaseCharacter* Pawn = Cast<ABaseCharacter>(Killer->GetPawn()))
	{
		Pawn->CharacterState = ECharacterState::Escaping;
	}

	// ให้ GameState เป็นคนจัดการกฎเกมและเปลี่ยน Phase

	if (GS->MatchPhase != EMatchPhase::Escape)
	{
		GS->OnTargetKilled(KillerPlayerState);

		APawn* KillerPawn = Killer->GetPawn();
		const FVector KillLocation = KillerPawn ? KillerPawn->GetActorLocation() : FVector::ZeroVector;

		AlertAllGuards(KillerPawn, KillLocation);
	}
	else
	{
		TWeakObjectPtr<APlayerState> WeakKiller = KillerPlayerState;
		FTimerHandle MovementTimerHandle;

		GetWorld()->GetTimerManager().SetTimer(
			MovementTimerHandle,
			[this, WeakKiller, GS]()
			{
				// 1. Ensure GS is still valid (if it's a UObject)
				if (GS)
				{
					// 2. Safely get the raw pointer from the weak pointer
					if (APlayerState* Killer = WeakKiller.Get()) // Also fixed the typo on this line
					{
						GS->OnTargetKilled(Killer);
					}
				}
			},
			3.67f,
			false
		);
	}
}

void ATry2AssassinGameMode::HandleReachedSafeZone(AController* Player)
{
	auto* GS = GetGameState<ATry2AssassinGameState>();
	if (!GS || GS->MatchPhase != EMatchPhase::Escape || !Player) { return; }

	// เฉพาะคนฆ่าเท่านั้นที่ชนะได้ (ถ้า Assassin เป็น null คือโหมด debug ให้ผ่าน)
	if (GS->Assassin && Player->PlayerState != GS->Assassin) { return; }

	GS->Winner = Player->PlayerState;
	GS->SetMatchPhase(EMatchPhase::Finished);

	UE_LOG(LogTemp, Warning, TEXT("Winner: %s"),
	       Player->PlayerState ? *Player->PlayerState->GetPlayerName() : TEXT("None"));
}

void ATry2AssassinGameMode::DebugKillTarget()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();

	if (PC)
	{
		HandleTargetKilled(PC);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("DebugKillTarget: No PlayerController")
		);
	}
}

void ATry2AssassinGameMode::DebugReachSafeZone()
{
	// เอา PlayerController คนแรกมาเป็นผู้ชนะ (ทดสอบเท่านั้น)
	HandleReachedSafeZone(GetWorld()->GetFirstPlayerController());
}


void ATry2AssassinGameMode::SpawnVillager(
	TSubclassOf<AVillagerCharacter> InVillagerClass,
	const FVector& SpawnLocation)
{
	if (!HasAuthority() || !InVillagerClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	GetWorld()->SpawnActor<AVillagerCharacter>(
		InVillagerClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);
}

void ATry2AssassinGameMode::SpawnTarget(TSubclassOf<ATargetCharacter> InTargetClass, const FVector& SpawnLocation)
{
	if (!HasAuthority() || !InTargetClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ATargetCharacter* SpawnedTarget =
		GetWorld()->SpawnActor<ATargetCharacter>(
			InTargetClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

	if (SpawnedTarget)
	{
		SpawnedTarget->Tags.Add(TEXT("MainTarget"));

		UE_LOG(LogTemp, Warning, TEXT("Target Spawned!"));
	}
}

void ATry2AssassinGameMode::SpawnGuard(TSubclassOf<AGuardCharacter> InGuardClass, const FVector& SpawnLocation)
{
	if (!HasAuthority() || !InGuardClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGuardCharacter* NewGuard = GetWorld()->SpawnActor<AGuardCharacter>(
		InGuardClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams
	);

	if (NewGuard)
	{
		GuardActors.Add(NewGuard); // ⬅️ เก็บไว้
		UE_LOG(LogTemp, Warning,
		       TEXT("Guard Spawned (%d total)"), GuardActors.Num());
	}
}


void ATry2AssassinGameMode::AlertAllGuards(AActor* HuntTarget, const FVector& AlertLocation)
{
	if (!HasAuthority() || !IsValid(HuntTarget))
	{
		return;
	}

	float DelaySeconds = 3.7f; // ⏱️ กำหนดเวลาหน่วงตรงนี้

	// ใช้ Lambda ร่วมกับ FTimerDelegate เพื่อส่งตัวแปร HuntTarget เข้าไปด้วย
	FTimerHandle AlertTimerHandle;
	FTimerDelegate AlertDelegate;
	AlertDelegate.BindUObject(this, &ATry2AssassinGameMode::ExecuteAlertAllGuards, HuntTarget);

	GetWorldTimerManager().SetTimer(
		AlertTimerHandle,
		AlertDelegate,
		DelaySeconds,
		false // เล่นครั้งเดียว ไม่วนลูป
	);

	UE_LOG(LogTemp, Warning, TEXT("Target killed! Guards will be alerted in %.1f seconds..."), DelaySeconds);
}

void ATry2AssassinGameMode::ExecuteAlertAllGuards(AActor* HuntTarget)
{
	if (!HasAuthority() || !IsValid(HuntTarget) || !IsValid(GuardActors[0]))
	{
		return;
	}

	int32 AlertedCount = 0;
	for (AGuardCharacter* Guard : GuardActors)
	{
		if (!IsValid(Guard)) { continue; }

		// เพิ่มความเร็วให้ Guard เปลี่ยนเป็นวิ่ง
		if (Guard->GetCharacterMovement())
		{
			Guard->GetCharacterMovement()->MaxWalkSpeed = 700.0f;
		}

		AGuardAIController* GuardAI = Cast<AGuardAIController>(Guard->GetController());
		if (!GuardAI) { continue; }

		GuardAI->HuntPlayer(HuntTarget);
		++AlertedCount;
	}

	UE_LOG(LogTemp, Warning,
	       TEXT("CITY ALERT! %d guards hunting %s"),
	       AlertedCount, *HuntTarget->GetName());
}

void ATry2AssassinGameMode::HandleTargetTakedown(ACharacter* Killer, ACharacter* Target)
{
	if (!Target || !Killer) return;

	// Lock การเคลื่อนไหวของทั้งสองฝ่าย
	if (Killer->GetCharacterMovement())
	{
		Killer->GetCharacterMovement()->DisableMovement();
	}
	if (Target->GetCharacterMovement())
	{
		Target->GetCharacterMovement()->DisableMovement();
	}

	ATry2AssassinGameState* GS = GetGameState<ATry2AssassinGameState>();

	// กรณีเป็น Target หลักเป็น Bot
	if (Target->ActorHasTag(TEXT("MainTarget")) && GS && GS->MatchPhase == EMatchPhase::Assassination)
	{
		// 1. ตั้ง Timer ทำลาย Target หลังจบจังหวะโดน
		TWeakObjectPtr<ACharacter> WeakTarget = Target;

		FTimerHandle DestroyTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			DestroyTimerHandle,
			[WeakTarget]()
			{
				if (WeakTarget.IsValid())
				{
					WeakTarget->Destroy();
				}
			},
			1.20f,
			false
		);

		Killer->Tags.Add(FName("Assassin"));
	}

	if (Target->ActorHasTag(TEXT("Assassin")) && GS && GS->MatchPhase == EMatchPhase::Escape)
	{
		// 1. ตั้ง Timer ทำลาย Target หลังจบจังหวะโดน
		TWeakObjectPtr<ACharacter> WeakTarget = Target;

		FTimerHandle DestroyTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			DestroyTimerHandle,
			[WeakTarget]()
			{
				if (WeakTarget.IsValid())
				{
					WeakTarget->Destroy();
				}
			},
			1.20f,
			false
		);
	}

	// 2. รอจน Animation ของ Attacker เล่นจบ แล้วคืนค่าการเดิน
	TWeakObjectPtr<ACharacter> WeakKiller = Killer;
	FTimerHandle MovementTimerHandle;

	GetWorld()->GetTimerManager().SetTimer(
		MovementTimerHandle,
		[WeakKiller]()
		{
			if (WeakKiller.IsValid() && WeakKiller->GetCharacterMovement())
			{
				WeakKiller->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}
		},
		3.67f,
		false
	);
}

void ATry2AssassinGameMode::HandleGameOver()
{
	UGameplayStatics::SetGamePaused(GetWorld(), true);
}
