// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "BaseCharacter.generated.h"

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Normal,
	Takedown,
	Escaping,
	Caught
};

UCLASS()
class TRY2ASSASSIN_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// สถานะปัจจุบันของตัวละคร
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character")
    ECharacterState CharacterState;

	// ตัวละครนี้สามารถถูก Takedown ได้หรือไม่
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	bool bCanBeTakedown;
	
	// ใช้เมื่อ Character ถูก Takedown
	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void Takedown();
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerTakedown();
};
