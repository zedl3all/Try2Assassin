#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"   // เปลี่ยนจาก AIController.h
#include "BehaviorTree/BehaviorTree.h"
#include "MyAIController.generated.h"

UCLASS()
class TRY2ASSASSIN_API AMyAIController : public ADetourCrowdAIController   // เปลี่ยน parent
{
	GENERATED_BODY()

public:
	AMyAIController(const FObjectInitializer& ObjectInitializer);   // เพิ่ม constructor

	virtual void OnPossess(APawn* InPawn) override;
	void StartEscape(AActor* EscapeTarget);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "AI")
	TObjectPtr<AActor> InitialTargetPoint;
};