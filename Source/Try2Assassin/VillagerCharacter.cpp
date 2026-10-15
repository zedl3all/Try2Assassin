#include "VillagerCharacter.h"
#include "MyAIController.h"
#include "EscapeZone.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"


AVillagerCharacter::AVillagerCharacter()
{
	AIControllerClass = AMyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	GetCharacterMovement()->bUseRVOAvoidance = false;
	//GetCharacterMovement()->bUseRVOAvoidance = true;
	//GetCharacterMovement()->AvoidanceConsiderationRadius = 150.0f;
	//GetCharacterMovement()->AvoidanceWeight = 0.5f;
}

void AVillagerCharacter::StartPanicEscape(float PanicSpeed)
{
	if (!HasAuthority()) { return; }

	GetCharacterMovement()->MaxWalkSpeed = PanicSpeed;

	AMyAIController* AI = Cast<AMyAIController>(GetController());
	if (!AI) { return; }

	// หา EscapeZone ทั้งหมด
	TArray<AActor*> Zones;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEscapeZone::StaticClass(), Zones);

	if (Zones.IsEmpty()) { return; }

	// สุ่มเลือก 1 อัน (ใกล้หรือไกลก็ได้ เท่ากันหมด)
	// AActor* Chosen = Zones[FMath::RandRange(0, Zones.Num() - 1)];
	//
	// AI->StartEscape(Chosen);

	AActor* Nearest = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (AActor* Z : Zones)
	{
		const float D = FVector::DistSquared(Z->GetActorLocation(), GetActorLocation());
		if (D < BestDistSq)
		{
			BestDistSq = D;
			Nearest = Z;
		}
	}

	if (!Nearest) { return; }

	AI->StartEscape(Nearest);
}