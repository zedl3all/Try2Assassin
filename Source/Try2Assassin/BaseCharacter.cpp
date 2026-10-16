// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"
#include "Try2AssassinGameMode.h"

// Sets default values
ABaseCharacter::ABaseCharacter()
{
	// ตัวละครกลุ่มนี้ไม่มี logic ใน Tick เลย (ไม่มี Tick override ทั้งใน C++ และ Blueprint)
	// เดิมเปิด Tick ทุกเฟรมให้ตัวละครทุกตัว (~30 ตัว: ชาวบ้าน/การ์ด/เป้าหมาย/ผู้เล่น)
	// ซึ่งกิน CPU เปล่า ๆ ทุกเฟรม -> ปิดเพื่อลด Frame/Game Lag
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	CharacterState = ECharacterState::Normal;
	bCanBeTakedown = false;

	TakeDownReach = CreateDefaultSubobject<USphereComponent>(TEXT("TakeDownReach"));
	TakeDownReach->SetupAttachment(GetMesh());
	TakeDownReach->SetSphereRadius(TakeDownRange);
	TakeDownReach->SetRelativeLocation(FVector(0, 0, 90));
	TakeDownReach->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TakeDownReach->SetCollisionResponseToAllChannels(ECR_Ignore);
	TakeDownReach->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TakeDownReach->SetGenerateOverlapEvents(true);
}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Cast PlayerInputComponent to UEnhancedInputComponent
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(TakeDownAction, ETriggerEvent::Started, this,
		                                   &ABaseCharacter::HandleTakedownInput);
	}
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseCharacter, CharacterState);
}

void ABaseCharacter::HandleTakedownInput(const FInputActionValue& Value)
{
	ServerTakedown();
}

void ABaseCharacter::ApplyTakedown()
{
	if (!HasAuthority() || !bCanBeTakedown)
	{
		return;
	}

	CharacterState = ECharacterState::Takedown;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("%s Takedown!"),
		*GetName()
	);
}

void ABaseCharacter::OnTakedownHit()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("%s Takedown Hit!"),
		*GetName()
	);
}

void ABaseCharacter::ServerTakedown_Implementation()
{
	// -------------------------------------------------
	// Server ตรวจสอบผู้ที่อยู่ในระยะ Takedown
	// -------------------------------------------------

	if (!TakeDownReach)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	TakeDownReach->GetOverlappingActors(
		OverlappingActors,
		ABaseCharacter::StaticClass()
	);

	const FVector Origin = GetActorLocation();
	const FVector Forward = GetActorForwardVector();

	const float CosThreshold =
		FMath::Cos(FMath::DegreesToRadians(TakeDownHalfAngle));

	ABaseCharacter* BestTarget = nullptr;
	float BestDistance = TakeDownRange;

	// -------------------------------------------------
	// หา Target ที่เหมาะที่สุด
	// -------------------------------------------------

	for (AActor* Actor : OverlappingActors)
	{
		ABaseCharacter* Target = Cast<ABaseCharacter>(Actor);

		if (!Target || Target == this)
		{
			continue;
		}

		// Target ต้องสามารถโดน Takedown ได้
		if (!Target->bCanBeTakedown)
		{
			continue;
		}

		// Target ต้องไม่อยู่ใน State ที่ไม่สามารถ Takedown ได้
		if (Target->CharacterState == ECharacterState::Takedown)
		{
			continue;
		}

		const FVector ToTarget =
			Target->GetActorLocation() - Origin;

		const float Distance = ToTarget.Size();

		if (Distance > TakeDownRange)
		{
			continue;
		}

		const FVector Direction =
			ToTarget.GetSafeNormal();

		// ตรวจสอบมุมด้านหน้า
		const float Dot =
			FVector::DotProduct(Forward, Direction);

		if (Dot < CosThreshold)
		{
			continue;
		}

		// เลือก Target ที่ใกล้ที่สุด
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestTarget = Target;
		}
	}

	// ไม่มี Target
	if (!BestTarget)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s attempted Takedown but no target was found."),
			*GetName()
		);

		return;
	}

	// -------------------------------------------------
	// Takedown Target
	// -------------------------------------------------

	//หัน Attacker ไปหา Target
	FVector Direction =
		BestTarget->GetActorLocation() - GetActorLocation();

	Direction.Z = 0.0f;

	FRotator TargetRotation =
		Direction.Rotation();

	SetActorRotation(TargetRotation);

	//หัน Target กลับมาหา Attacker
	FVector TargetDirection =
		GetActorLocation() - BestTarget->GetActorLocation();

	TargetDirection.Z = 0.0f;

	BestTarget->SetActorRotation(
		TargetDirection.Rotation()
	);

	//จัดตำแหน่ง Target
	FVector NewForWard = GetActorForwardVector();
	FVector NewTargetLocation =
		GetActorLocation()
		+ NewForWard * TakedownAnimationDistance;
	BestTarget->SetActorLocation(
		NewTargetLocation,
		false
	);

	// เริ่ม TakeDown
	BestTarget->ApplyTakedown();

	if (BestTarget->CharacterState == ECharacterState::Takedown
		&& (BestTarget->ActorHasTag(TEXT("MainTarget")) || BestTarget->ActorHasTag(TEXT("Assassin"))))
	{
		if (AController* KillerController = GetController())
		{
			if (ATry2AssassinGameMode* GM = GetWorld()->GetAuthGameMode<ATry2AssassinGameMode>())
			{
				GM->HandleTargetTakedown(this, BestTarget); // ต้องอยู่ข้างบน TargetKill
				GM->HandleTargetKilled(KillerController);
			}
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("ServerTakedown: KillerController is NULL")
			);
		}
	}

	MulticastPlayTakedownAnimation(
		BestTarget,
		GetActorRotation(),
		BestTarget->GetActorRotation()
	);
	MulticastPlayHitFX(BestTarget->GetActorLocation());

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("%s Takedown %s"),
		*GetName(),
		*BestTarget->GetName()
	);
}

bool ABaseCharacter::ServerTakedown_Validate()
{
	return true;
}

void ABaseCharacter::MulticastPlayTakedownAnimation_Implementation(
	ABaseCharacter* Target,
	FRotator AttackerRotation,
	FRotator TargetRotation)
{
	// หัน Attacker
	SetActorRotation(AttackerRotation);

	// หัน Victim
	if (Target)
	{
		Target->SetActorRotation(TargetRotation);
	}

	// เล่น Animation ของ Attacker
	if (TakedownMontage)
	{
		PlayAnimMontage(TakedownMontage);
	}
}

void ABaseCharacter::MulticastPlayHitFX_Implementation(FVector HitLocation)
{
}
