// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TargetCharacter.h"
#include "VillagerCharacter.h"
#include "GuardCharacter.h"
#include "GuardAIController.h"
#include "Try2AssassinGameMode.generated.h"

class AEscapeZone;

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

	// ---- Cache รายการ actor ในโลก (หา GetAllActorsOfClass แค่ครั้งเดียวตอน BeginPlay) ----
	// AI ทุกตัวใช้รายการ PatrolPoint ร่วมกัน ไม่ต้องสแกนโลกใหม่ต่อ AI 1 ตัว
	const TArray<TObjectPtr<AActor>>& GetCachedPatrolPoints() const { return CachedPatrolPoints; }

	// หา EscapeZone ที่ใกล้ที่สุดจากตำแหน่งที่กำหนด (ใช้ cache เดียวกัน)
	// ชาวบ้านทุกตัวเรียกผ่านฟังก์ชันนี้แทนการสแกนโลกเอง
	AEscapeZone* FindNearestEscapeZone(const FVector& From) const;

private:
	void StartAssassinationPhase();

	// ---- Spawn แบบกระจายเวลา (กันเฟรมกระตุกตอน BeginPlay) ----
	// สร้างเป็นคิวแล้วทยอย spawn ทีละตัวด้วย timer
	void ProcessSpawnQueue();

	// เก็บ TargetPoint / EscapeZone ทั้งหมดในโลกไว้ใช้ซ้ำ
	void CacheWorldActors();

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

	// ระยะเวลาระหว่างการ spawn AI แต่ละตัว (วินาที)
	// ยิ่งมาก ยิ่งกระจายภาระ แต่ spawn เสร็จช้าลง
	UPROPERTY(EditDefaultsOnly, Category = "AI|Spawn")
	float SpawnInterval = 0.05f;

	// จุดศูนย์กลางและรัศมีที่ใช้สุ่มตำแหน่ง spawn AI
	UPROPERTY(EditDefaultsOnly, Category = "AI|Spawn")
	FVector SpawnCenter = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Spawn")
	float SpawnRadius = 3000.f;

	// สถานะของ spawn queue
	FTimerHandle SpawnQueueTimer;
	int32 SpawnedVillagers = 0;
	int32 SpawnedGuards = 0;
	bool bTargetSpawned = false;
	int32 NavRetryCount = 0;

	// Cache รายการ actor ในโลก
	UPROPERTY()
	TArray<TObjectPtr<AActor>> CachedPatrolPoints;

	UPROPERTY()
	TArray<TObjectPtr<AEscapeZone>> CachedEscapeZones;

	// สั่ง Guard ทั้งเมืองให้ตื่นตัว
	void AlertAllGuards(AActor* HuntTarget, const FVector& AlertLocation);

	// รายการ Guard ทั้งหมดที่ spawn ไว้
	UPROPERTY()
	TArray<TObjectPtr<AGuardCharacter>> GuardActors;

	// 🔹 เพิ่มฟังก์ชันนี้เพื่อใช้รับช่วงต่อหลังหมดเวลา Delay
	void ExecuteAlertAllGuards(AActor* HuntTarget);
};
