// Fill out your copyright notice in the Description page of Project Settings.


#include "GuardCharacter.h"
#include "GuardAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"

AGuardCharacter::AGuardCharacter()
{
	// ใช้ GuardAIController แทน MyAIController
	AIControllerClass = AGuardAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// สร้าง AI Perception Component
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	// ตั้งค่าการมองเห็น
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 45.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.0f;

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AGuardCharacter::OnTargetPerceptionUpdated);
}

void AGuardCharacter::OnTargetPerceptionUpdated(AActor* UpdatedActor, FAIStimulus Stimulus)
{
	if (Stimulus.WasSuccessfullySensed() && UpdatedActor)
	{
		if (AGuardAIController* AICon = Cast<AGuardAIController>(GetController()))
		{
			// ให้ Guard ไล่ล่าทันทีที่มองเห็นผู้เล่น
			AICon->HuntPlayer(const_cast<AActor*>(UpdatedActor));
		}
	}
}
