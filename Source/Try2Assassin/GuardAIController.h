// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyAIController.h"
#include "GuardAIController.generated.h"

/**
 * 
 */
UCLASS()
class TRY2ASSASSIN_API AGuardAIController : public AMyAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;

	// ฟังก์ชันสำหรับสั่งให้ Guard ไล่ล่าผู้เล่น
	UFUNCTION(BlueprintCallable, Category = "AI")
	void HuntPlayer(AActor* PlayerTarget);

	// ฟังก์ชันสำหรับสั่งให้ Guard กลับไปตรวจตรา
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ResumePatrol();

	// แจ้งเตือนให้มาตรวจสอบจุดเกิดเหตุ (ยังไม่รู้ตัวผู้ต้องสงสัย)
	UFUNCTION(BlueprintCallable, Category = "AI")
	void AlertToLocation(const FVector& AlertLocation);

protected:
	// BehaviorTree เฉพาะของ Guard (ถ้าต้องการแยกจาก Villager)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> GuardBehaviorTreeAsset;
};
