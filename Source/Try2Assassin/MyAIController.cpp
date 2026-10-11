// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAIController.h"
#include "VillagerCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"

void AMyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Check BehaviorTree if exit
	if (!IsValid(BehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("MyAIController: BehaviorTreeAsset is missing"));
		return;
	}

	// Check Blackboard if exit
	UBlackboardComponent* BlackboardComp = nullptr;

	if (!UseBlackboard(
		BehaviorTreeAsset->BlackboardAsset,
		BlackboardComp))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("MyAIController: Failed to initialize Blackboard"));
		return;
	}

	// Get TargetPoints
	TArray<AActor*> FoundTargetPoints;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ATargetPoint::StaticClass(),
		FoundTargetPoints
	);

	AVillagerCharacter* Villager = Cast<AVillagerCharacter>(InPawn);

	if (Villager)
	{
		Villager->PatrolPoints.Reset();

		for (AActor* Point : FoundTargetPoints)
		{
			if (IsValid(Point))
			{
				Villager->PatrolPoints.Add(Point);
			}
		}
	}

	RunBehaviorTree(BehaviorTreeAsset);
}
