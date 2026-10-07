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

class UAnimMontage;
// Forward declarations for Enhanced Input types
class UInputAction;
struct FInputActionValue;

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

	void HandleTakedownInput(const FInputActionValue& Value);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/////////////////////////////////////////////////////////////////

	// สถานะปัจจุบันของตัวละคร
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Character")
	ECharacterState CharacterState;

	// ตัวละครนี้สามารถถูก Takedown ได้หรือไม่
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	bool bCanBeTakedown;

	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void ApplyTakedown();

	// For Animation
	UFUNCTION(BlueprintCallable, Category="Takedown")
	void OnTakedownHit();

	// --- client -> server: ---
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerTakedown();

	// --- server -> everyone: play Animation ---
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayTakedownAnimation(
		ABaseCharacter* Target,
		FRotator AttackerRotation,
		FRotator TargetRotation
	);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitFX(FVector HitLocation);

	/////////////////////////////////////////////////////////////////
	UPROPERTY(EditAnywhere, Category="TakeDown")
	float TakeDownRange = 150.f;

	UPROPERTY(EditAnywhere, Category="TakeDown")
	float TakeDownHalfAngle = 60.f;

	UPROPERTY(VisibleAnywhere, Category="TakeDown")
	class USphereComponent* TakeDownReach;

	// For Animation TakeDown Range
	UPROPERTY(EditAnywhere, Category="TakeDown|Animation")
	float TakedownAnimationDistance = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Takedown|Animation")
	float VictimAnimationDelay = 1.04f;

	/////////////////////////////////////////////////////////////////
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TakeDownAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Takedown")
	UAnimMontage* TakedownMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Takedown")
	UAnimMontage* TakedownReactionMontage;
};
