// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Try2AssassinGameMode.h"

// Sets default values
ABaseCharacter::ABaseCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
	if (!HasAuthority())
	{
		return;
	}

	if (!bCanBeTakedown)
	{
		return;
	}

	CharacterState = ECharacterState::Takedown;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("%s was Takedown!"),
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
		&& BestTarget->ActorHasTag(TEXT("MainTarget")))
	{
		if (ATry2AssassinGameMode* GM = GetWorld()->GetAuthGameMode<ATry2AssassinGameMode>())
		{
			GM->HandleTargetKilled(GetController());
		}
	}

	//Lock Movement
	GetCharacterMovement()->DisableMovement();
	BestTarget->GetCharacterMovement()->DisableMovement();

	MulticastPlayTakedownAnimation(
		BestTarget,
		GetActorRotation(),
		BestTarget->GetActorRotation()
	);
	MulticastPlayHitFX(BestTarget->GetActorLocation());

	// Destroy Target ตอนถึงจังหวะโดน
	FTimerHandle DestroyTimerHandle;

	GetWorldTimerManager().SetTimer(
		DestroyTimerHandle,
		[BestTarget]()
		{
			if (IsValid(BestTarget))
			{
				BestTarget->Destroy();
			}
		},
		1.20f,
		false
	);

	// รอจน Animation ของ Attacker เล่นจบ
	FTimerHandle MovementTimerHandle;

	GetWorldTimerManager().SetTimer(
		MovementTimerHandle,
		[this]()
		{
			if (IsValid(this) && GetCharacterMovement())
			{
				GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}
		},
		3.67f,
		false
	);

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
