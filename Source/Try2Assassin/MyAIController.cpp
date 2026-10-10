// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAIController.h"
#include "VillagerCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

void AMyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsValid(BehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("MyAIController: BehaviorTreeAsset is missing"));
		return;
	}

	UBlackboardComponent* BlackboardComp = nullptr;

	if (!UseBlackboard(
		BehaviorTreeAsset->BlackboardAsset,
		BlackboardComp))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("MyAIController: Failed to initialize Blackboard"));
		return;
	}

	AVillagerCharacter* Villager = Cast<AVillagerCharacter>(InPawn);

	if (Villager && IsValid(Villager->InitialTargetPoint))
	{
		BlackboardComp->SetValueAsVector(
			TEXT("TargetLocation"),
			Villager->InitialTargetPoint->GetActorLocation()
		);
	}

	RunBehaviorTree(BehaviorTreeAsset);
}
