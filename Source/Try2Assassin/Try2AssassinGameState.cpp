#include "Try2AssassinGameState.h"
#include "Net/UnrealNetwork.h"

ATry2AssassinGameState::ATry2AssassinGameState()
{
	// GameStateBase replicate อยู่แล้วโดยค่าเริ่มต้น (อย่าปิด Replicates)
}

void ATry2AssassinGameState::SetMatchPhase(EMatchPhase NewPhase)
{
	if (!HasAuthority()) { return; }          // กฎ Lab 5: เปลี่ยนค่าได้เฉพาะ server
	if (MatchPhase == NewPhase) { return; }

	const EMatchPhase Old = MatchPhase;
	MatchPhase = NewPhase;

	// OnRep ไม่ทำงานบน server (กฎ Lab 5) จึงต้องเรียกเองให้ host เห็นด้วย
	OnRep_MatchPhase(Old);
}

void ATry2AssassinGameState::OnRep_MatchPhase(EMatchPhase OldPhase)
{
	UE_LOG(LogTemp, Warning, TEXT("Phase changed: %d -> %d"),
		(int32)OldPhase, (int32)MatchPhase);
	OnMatchPhaseChanged.Broadcast(OldPhase, MatchPhase);
}

void ATry2AssassinGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATry2AssassinGameState, MatchPhase);
	DOREPLIFETIME(ATry2AssassinGameState, bTargetKilled);
	DOREPLIFETIME(ATry2AssassinGameState, Winner);
	DOREPLIFETIME(ATry2AssassinGameState, Assassin);
}