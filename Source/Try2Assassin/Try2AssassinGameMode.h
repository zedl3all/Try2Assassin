// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VillagerCharacter.h"
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

	// ===== เพิ่มใหม่: คำสั่งทดสอบ =====
	UFUNCTION(Exec)
	void DebugKillTarget();

	UFUNCTION(Exec)
	void DebugReachSafeZone();

	UFUNCTION()
	void SpawnVillager(
		TSubclassOf<AVillagerCharacter> VillagerClass,
		const FVector& SpawnLocation
	);

private:
	void StartAssassinationPhase();

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TSubclassOf<AVillagerCharacter> VillagerClass;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	int32 VillagerCount = 30;
};
