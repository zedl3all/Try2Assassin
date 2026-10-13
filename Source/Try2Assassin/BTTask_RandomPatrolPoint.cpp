// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_RandomPatrolPoint.h"
#include "BaseCharacter.h"
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

	// ⬇️ เปลี่ยนจาก AVillagerCharacter เป็น ABaseCharacter
	ABaseCharacter* BaseChar =
		Cast<ABaseCharacter>(AIController->GetPawn());

	if (!BaseChar || BaseChar->PatrolPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("RandomPatrolPoint: No PatrolPoints! Pawn: %s"),
		       *AIController->GetPawn()->GetName());
		return EBTNodeResult::Failed;
	}

	const int32 NumPoints = BaseChar->PatrolPoints.Num();
	int32 RandomIndex = 0;

	// ถ้ามีหลายจุด จะไม่เลือกจุดเดิมติดกัน
	if (NumPoints > 1)
	{
		do
		{
			RandomIndex = FMath::RandRange(0, NumPoints - 1);
		}
		while (RandomIndex == BaseChar->LastPatrolPointIndex);
	}

	AActor* TargetPoint = BaseChar->PatrolPoints[RandomIndex];
	if (!IsValid(TargetPoint))
	{
		return EBTNodeResult::Failed;
	}

	// บันทึก Index ของจุดที่เลือก
	BaseChar->LastPatrolPointIndex = RandomIndex;

	UBlackboardComponent* Blackboard =
		OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	// ⚠️ เป็น Vector ไม่ใช่ Object!
	Blackboard->SetValueAsVector(
		GetSelectedBlackboardKey(),
		TargetPoint->GetActorLocation()
	);

	UE_LOG(LogTemp, Warning,
	       TEXT("RandomPatrolPoint: Selected point %d at %s"),
	       RandomIndex, *TargetPoint->GetActorLocation().ToString());

	return EBTNodeResult::Succeeded;
}
