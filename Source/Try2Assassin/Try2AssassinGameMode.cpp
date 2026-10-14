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
		UNavigationSystemV1* NavSystem =
			FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

		if (NavSystem)
		{
			const FVector Center = FVector(0.f, 0.f, 0.f);
			const float SpawnRadius = 3000.f;

			// Spawn Villagers
			if (VillagerClass)
			{
				for (int32 i = 0; i < VillagerCount; ++i)
				{
					FNavLocation RandomLocation;

					const bool bFoundLocation =
						NavSystem->GetRandomReachablePointInRadius(
							Center,
							SpawnRadius,
							RandomLocation
						);

					if (bFoundLocation)
					{
						FVector SpawnLocation = RandomLocation.Location;
						SpawnLocation.Z += 30.0f;

						SpawnVillager(VillagerClass, SpawnLocation);
					}
					else
					{
						UE_LOG(
							LogTemp,
							Warning,
							TEXT("Could not find NavMesh location for Villager")
						);
					}
				}
			}

			//spawn Guard
			if (GuardClass)
			{
				for (int32 i = 0; i < GuardCount; ++i)
				{
					FNavLocation RandomLocation;

					const bool bFoundLocation =
						NavSystem->GetRandomReachablePointInRadius(
							Center,
							SpawnRadius,
							RandomLocation
						);

					if (bFoundLocation)
					{
						FVector SpawnLocation = RandomLocation.Location;
						SpawnLocation.Z += 30.0f;

						SpawnGuard(GuardClass, SpawnLocation);
					}
					else
					{
						UE_LOG(
							LogTemp,
							Warning,
							TEXT("Could not find NavMesh location for Guard")
						);
					}
				}
			}

			// Spawn Target (1 ตัว)
			if (bSpawnTarget && TargetClass)
			{
				FNavLocation RandomLocation;

				const bool bFoundLocation =
					NavSystem->GetRandomReachablePointInRadius(
						Center,
						SpawnRadius,
						RandomLocation
					);

				if (bFoundLocation)
				{
					FVector SpawnLocation = RandomLocation.Location;
					SpawnLocation.Z += 30.0f;

					SpawnTarget(TargetClass, SpawnLocation);
				}
				else
				{
					UE_LOG(
						LogTemp,
						Warning,
						TEXT("Could not find NavMesh location for Target")
					);
				}
			}
		}
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

	if (GS->MatchPhase != EMatchPhase::Assassination)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("HandleTargetKilled rejected: MatchPhase is not Assassination")
		);
		return;
	}

	if (GS->bTargetKilled)
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
	GS->OnTargetKilled(KillerPlayerState);

	APawn* KillerPawn = Killer->GetPawn();
	const FVector KillLocation = KillerPawn ? KillerPawn->GetActorLocation() : FVector::ZeroVector;

	AlertAllGuards(KillerPawn, KillLocation);
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
			Guard->GetCharacterMovement()->MaxWalkSpeed = 600.0f;
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
