// Fill out your copyright notice in the Description page of Project Settings.


#include "GuardAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BaseCharacter.h"

void AGuardAIController::OnPossess(APawn* InPawn)
{
	// เรียก logic พื้นฐานจาก MyAIController (โหลด BT, หา PatrolPoints)
	// แต่เราจะใช้ GuardBehaviorTreeAsset แทน ถ้ามี
	if (GuardBehaviorTreeAsset)
	{
		// Override BehaviorTreeAsset ชั่วคราว
		BehaviorTreeAsset = GuardBehaviorTreeAsset;
	}

	Super::OnPossess(InPawn);

	// เพิ่ม logic เฉพาะของ Guard ที่นี่
	// เช่น ตั้งค่า Blackboard เริ่มต้น
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(TEXT("bIsAlerted"), false);
		BB->SetValueAsBool(TEXT("bIsHunting"), false);
	}
}

void AGuardAIController::HuntPlayer(AActor* PlayerTarget)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(TEXT("TargetActor"), PlayerTarget);
		BB->SetValueAsBool(TEXT("bIsAlerted"), true);
		BB->SetValueAsBool(TEXT("bIsHunting"), true);

		UE_LOG(LogTemp, Warning, TEXT("Guard %s is now hunting player!"), *GetName());
	}
}

void AGuardAIController::ResumePatrol()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(TEXT("bIsAlerted"), false);
		BB->SetValueAsBool(TEXT("bIsHunting"), false);
		BB->ClearValue(TEXT("TargetActor"));

		UE_LOG(LogTemp, Warning, TEXT("Guard %s resumed patrol."), *GetName());
	}
}

void AGuardAIController::AlertToLocation(const FVector& AlertLocation)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(TEXT("TargetLocation"), AlertLocation); // จุดที่ BT Search ใช้ MoveTo
		BB->SetValueAsBool(TEXT("bIsAlerted"), true);
		BB->SetValueAsBool(TEXT("bIsHunting"), false); // ⬅️ ยังไม่ล่า จนกว่าจะ "เห็น"

		UE_LOG(LogTemp, Warning,
		       TEXT("Guard %s alerted to %s"), *GetName(), *AlertLocation.ToString());
	}
}
