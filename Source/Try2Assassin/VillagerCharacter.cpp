// Fill out your copyright notice in the Description page of Project Settings.


#include "VillagerCharacter.h"
#include "MyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"

AVillagerCharacter::AVillagerCharacter()
{
	AIControllerClass = AMyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 150.0f;
	GetCharacterMovement()->AvoidanceWeight = 0.5f;
}
