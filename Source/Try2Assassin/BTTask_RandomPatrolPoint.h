// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_RandomPatrolPoint.generated.h"

/**
 * 
 */
UCLASS()
class TRY2ASSASSIN_API UBTTask_RandomPatrolPoint : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_RandomPatrolPoint();

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;
};
