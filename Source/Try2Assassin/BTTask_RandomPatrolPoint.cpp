// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_RandomPatrolPoint.h"
#include "VillagerCharacter.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_RandomPatrolPoint::UBTTask_RandomPatrolPoint()
{
	NodeName = TEXT("Random Patrol Point");
}


EBTNodeResult::Type UBTTask_RandomPatrolPoint::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	AAIController* AIController = OwnerComp.GetAIOwner();

	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AVillagerCharacter* Villager =
		Cast<AVillagerCharacter>(AIController->GetPawn());

	if (!Villager || Villager->PatrolPoints.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}

	const int32 NumPoints = Villager->PatrolPoints.Num();

	int32 RandomIndex = 0;

	// ถ้ามีหลายจุด จะไม่เลือกจุดเดิมติดกัน
	if (NumPoints > 1)
	{
		do
		{
			RandomIndex = FMath::RandRange(0, NumPoints - 1);
		}
		while (RandomIndex == Villager->LastPatrolPointIndex);
	}

	AActor* TargetPoint = Villager->PatrolPoints[RandomIndex];

	if (!IsValid(TargetPoint))
	{
		return EBTNodeResult::Failed;
	}

	// บันทึก Index ของจุดที่เลือก
	Villager->LastPatrolPointIndex = RandomIndex;

	UBlackboardComponent* Blackboard =
		OwnerComp.GetBlackboardComponent();

	if (!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsVector(
		GetSelectedBlackboardKey(),
		TargetPoint->GetActorLocation()
	);

	return EBTNodeResult::Succeeded;
}
