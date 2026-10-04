#include "Try2AssassinGameMode.h"
#include "Try2AssassinGameState.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerState.h"

ATry2AssassinGameMode::ATry2AssassinGameMode()
{
	GameStateClass = ATry2AssassinGameState::StaticClass();
	
	// เปลี่ยน Default Actor เป็น ตัวที่สร้าง
	// หา Blueprint Class ของ Pawn ที่อยู่ใน Content Browser
	// หมายเหตุ: พาธต้องลงท้ายด้วย _C เสมอ
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/MyAsset/BP_BaseThirdPersonCharacter.BP_BaseThirdPersonCharacter_C"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void ATry2AssassinGameMode::BeginPlay()
{
	Super::BeginPlay();
	// ชั่วคราวสำหรับทดสอบ: เริ่ม Assassination หลัง 2 วินาที
	FTimerHandle TH;
	GetWorldTimerManager().SetTimer(TH, this,
		&ATry2AssassinGameMode::StartAssassinationPhase, 2.0f, false);
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
	GS->SetMatchPhase(EMatchPhase::Escape);   // เมืองเข้าสู่ Alert
}

void ATry2AssassinGameMode::HandleReachedSafeZone(AController* Player)
{
	auto* GS = GetGameState<ATry2AssassinGameState>();
	if (!GS || GS->MatchPhase != EMatchPhase::Escape || !Player) { return; }

	GS->Winner = Player->PlayerState;
	GS->SetMatchPhase(EMatchPhase::Finished);
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