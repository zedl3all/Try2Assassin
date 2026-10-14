#include "Try2AssassinGameState.h"
#include "Net/UnrealNetwork.h"
#include "BaseCharacter.h"
#include "VillagerCharacter.h"
#include "AIController.h"
#include "Try2AssassinGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

ATry2AssassinGameState::ATry2AssassinGameState()
{
	// GameStateBase replicate อยู่แล้วโดยค่าเริ่มต้น (อย่าปิด Replicates)
}

void ATry2AssassinGameState::SetMatchPhase(EMatchPhase NewPhase)
{
	if (!HasAuthority()) { return; } // กฎ Lab 5: เปลี่ยนค่าได้เฉพาะ server
	if (MatchPhase == NewPhase) { return; }

	const EMatchPhase Old = MatchPhase;
	MatchPhase = NewPhase;
	
	ApplyPhaseToWorld(NewPhase);

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

void ATry2AssassinGameState::OnTargetKilled(APlayerState* Killer)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("OnTargetKilled CALLED! Killer = %s"),
		Killer ? *Killer->GetPlayerName() : TEXT("NULL")
	);

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Rejected: No Authority"));
		return;
	}

	if (bTargetKilled)
	{
		UE_LOG(LogTemp, Warning, TEXT("Rejected: Target already killed"));
		return;
	}

	if (!Killer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Rejected: Killer is NULL"));
		return;
	}

	bTargetKilled = true;
	Assassin = Killer;

	SetMatchPhase(EMatchPhase::Escape);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Target killed by: %s"),
		*Killer->GetPlayerName()
	);
}
void ATry2AssassinGameState::ApplyPhaseToWorld(EMatchPhase NewPhase)
{
	if (!HasAuthority()) { return; }

	TArray<AActor*> Chars;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), Chars);

	for (AActor* A : Chars)
	{
		ABaseCharacter* C = Cast<ABaseCharacter>(A);
		if (!IsValid(C) || !C->GetCharacterMovement()) { continue; }

		switch (NewPhase)
		{
		case EMatchPhase::Escape:
			// ชาวบ้านตกใจ: วิ่งหนีไป EscapeZone ที่ใกล้ที่สุด
			if (AVillagerCharacter* Villager = Cast<AVillagerCharacter>(C))
			{
				Villager->StartPanicEscape(450.f);
			}
			break;

		case EMatchPhase::Finished:
			C->GetCharacterMovement()->DisableMovement();
			if (AAIController* AI = Cast<AAIController>(C->GetController()))
			{
				AI->StopMovement();
			}
			break;

		default:
			break;
		}
	}
}
