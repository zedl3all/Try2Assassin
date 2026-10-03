#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Try2AssassinGameState.generated.h"   // ต้องเป็น include สุดท้ายเสมอ

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	WaitingToStart,
	Assassination,   // ตามหาและฆ่าเป้าหมาย
	Escape,          // เมือง Alert แล้ว หนีตำรวจไป Safe Zone
	Finished
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchPhaseChanged,
	EMatchPhase, OldPhase, EMatchPhase, NewPhase);

UCLASS()
class TRY2ASSASSIN_API ATry2AssassinGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATry2AssassinGameState();

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Match")
	EMatchPhase MatchPhase = EMatchPhase::WaitingToStart;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	bool bTargetKilled = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	TObjectPtr<APlayerState> Winner = nullptr;

	// UI / Police AI ผูกกับ event นี้เพื่อรู้ว่า phase เปลี่ยน
	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchPhaseChanged OnMatchPhaseChanged;

	// เรียกได้เฉพาะบน server (GameMode เป็นคนเรียก)
	void SetMatchPhase(EMatchPhase NewPhase);

	UFUNCTION()
	void OnRep_MatchPhase(EMatchPhase OldPhase);

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};