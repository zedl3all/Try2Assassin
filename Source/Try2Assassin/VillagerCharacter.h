// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "VillagerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class TRY2ASSASSIN_API AVillagerCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	AVillagerCharacter();
	// UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "AI")
	// TObjectPtr<AActor> InitialTargetPoint;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "AI")
	TArray<TObjectPtr<AActor>> PatrolPoints;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	int32 LastPatrolPointIndex = -1;
	
};
