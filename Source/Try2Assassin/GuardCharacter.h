// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "Perception/AIPerceptionTypes.h"
#include "GuardCharacter.generated.h"

/**
 * 
 */
UCLASS()
class TRY2ASSASSIN_API AGuardCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AGuardCharacter();

protected:

	// AI Perception Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAISenseConfig_Sight* SightConfig;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* UpdatedActor, FAIStimulus Stimulus);
};
