// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TargetCharacter.h"
#include "VillagerCharacter.h"
#include "GuardCharacter.h"
#include "GuardAIController.h"
#include "Try2AssassinGameMode.generated.h"

/**
 * 
 */
UCLASS()
class TRY2ASSASSIN_API ATry2AssassinGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATry2AssassinGameMode();
	virtual void BeginPlay() override;

	// จะถูกเรียกจากระบบสังหารในอนาคต (บน server เท่านั้น)
	void HandleTargetKilled(AController* Killer);
	void HandleReachedSafeZone(AController* Player);

	void HandleTargetTakedown(ACharacter* Killer, ACharacter* Target);

	// ===== เพิ่มใหม่: คำสั่งทดสอบ =====
	UFUNCTION(Exec)
	void DebugKillTarget();

	UFUNCTION(Exec)
	void DebugReachSafeZone();

	void SpawnVillager(
		TSubclassOf<AVillagerCharacter> VillagerClass,
		const FVector& SpawnLocation
	);

	void SpawnTarget(
		TSubclassOf<ATargetCharacter> InTargetClass,
		const FVector& SpawnLocation
	);

	void SpawnGuard(
		TSubclassOf<AGuardCharacter> InGuardClass,
		const FVector& SpawnLocation
	);
	
	void HandleGameOver();

private:
	void StartAssassinationPhase();

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TSubclassOf<AVillagerCharacter> VillagerClass;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	int32 VillagerCount = 22;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TSubclassOf<ATargetCharacter> TargetClass;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	bool bSpawnTarget = true;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TSubclassOf<AGuardCharacter> GuardClass;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	int32 GuardCount = 7;

	// สั่ง Guard ทั้งเมืองให้ตื่นตัว
	void AlertAllGuards(AActor* HuntTarget, const FVector& AlertLocation);

	// รายการ Guard ทั้งหมดที่ spawn ไว้
	UPROPERTY()
	TArray<TObjectPtr<AGuardCharacter>> GuardActors;

	// 🔹 เพิ่มฟังก์ชันนี้เพื่อใช้รับช่วงต่อหลังหมดเวลา Delay
	void ExecuteAlertAllGuards(AActor* HuntTarget);
};
