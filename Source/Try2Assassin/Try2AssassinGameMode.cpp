#include "Try2AssassinGameMode.h"
#include "Try2AssassinGameState.h"
#include "GameFramework/GameModeBase.h"
#include "BaseCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "VillagerCharacter.h"
#include "UObject/SoftObjectPath.h"
#include "NavigationSystem.h"
#include "VillagerCharacter.h"
#include "GameFramework/PlayerState.h"

ATry2AssassinGameMode::ATry2AssassinGameMode()
{
	GameStateClass = ATry2AssassinGameState::StaticClass();

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

	// Spawn Village in Nav Mesh
	if (HasAuthority() && VillagerClass)
	{
		UNavigationSystemV1* NavSystem =
			FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

		if (NavSystem)
		{
			const FVector Center = FVector(0.f, 0.f, 0.f);
			const float SpawnRadius = 3000.f;

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
						TEXT("Could not find random NavMesh location")
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
	auto* GS = GetGameState<ATry2AssassinGameState>();
	if (!GS || GS->MatchPhase != EMatchPhase::Assassination) { return; }

	GS->bTargetKilled = true;

	if (Killer)
	{
		GS->Assassin = Killer->PlayerState;

		// ใช้ enum ของเพื่อน: คนฆ่าเข้าสู่สถานะ Escaping
		// (CharacterState เป็น public และ replicate อยู่แล้ว ไม่ต้องแก้โค้ดเพื่อน)
		if (ABaseCharacter* Pawn = Cast<ABaseCharacter>(Killer->GetPawn()))
		{
			Pawn->CharacterState = ECharacterState::Escaping;
		}
	}

	GS->SetMatchPhase(EMatchPhase::Escape);
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
	HandleTargetKilled(nullptr);
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
