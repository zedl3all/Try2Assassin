// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAIController.h"
#include "BaseCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"

AMyAIController::AMyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AMyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsValid(BehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("MyAIController: BehaviorTreeAsset is missing"));
		return;
	}

	UBlackboardComponent* BlackboardComp = nullptr;
	if (!UseBlackboard(BehaviorTreeAsset->BlackboardAsset, BlackboardComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("MyAIController: Failed to initialize Blackboard"));
		return;
	}

	// หา TargetPoints ทั้งหมดในแมพ
	TArray<AActor*> FoundTargetPoints;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ATargetPoint::StaticClass(),
		FoundTargetPoints
	);
	
	
	ABaseCharacter* BasePawn = Cast<ABaseCharacter>(InPawn);
	if (BasePawn)
	{
		BasePawn->PatrolPoints.Reset();
		for (AActor* Point : FoundTargetPoints)
		{
			if (IsValid(Point))
			{
				BasePawn->PatrolPoints.Add(Point);
			}
		}
	}

	RunBehaviorTree(BehaviorTreeAsset);
}

void AMyAIController::StartEscape(AActor* EscapeTarget)
{
	if (!IsValid(EscapeTarget)) { return; }

	// หยุด BT ไม่ให้มาสั่งเดินสุ่มทับ
	if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
	{
		BTComp->StopTree(EBTStopMode::Safe);
	}

	StopMovement();

	// AcceptanceRadius เล็ก ๆ เพื่อให้เดินเข้าไปใน Box จริง ๆ (Box ครึ่งกว้าง 300)
	MoveToActor(EscapeTarget, /*AcceptanceRadius=*/50.f, /*bStopOnOverlap=*/false);
}