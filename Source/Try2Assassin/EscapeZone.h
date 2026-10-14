#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Try2AssassinGameState.h"
#include "EscapeZone.generated.h"   // ต้องเป็น include สุดท้ายเสมอ

class UBoxComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UCLASS()
class TRY2ASSASSIN_API AEscapeZone : public AActor
{
	GENERATED_BODY()

public:
	AEscapeZone();

	// เปิด/ปิดเอฟเฟกต์ประตูเรืองแสง
	UFUNCTION(BlueprintCallable, Category = "EscapeZone")
	void SetPortalActive(bool bActive);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EscapeZone")
	TObjectPtr<UBoxComponent> Box;

	// เสาแสงทรงกระบอก (โปร่งแสง มองทะลุได้) ใส่ Mesh/Material ได้ใน Blueprint
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EscapeZone")
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EscapeZone")
	TObjectPtr<UPointLightComponent> PortalLight;

	// สีของประตู
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EscapeZone")
	FLinearColor PortalColor = FLinearColor(0.f, 1.f, 0.4f, 1.f);

	// ความสว่างของ Emissive ใน Material (ต้องมี parameter ชื่อ "Color" และ "Intensity")
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EscapeZone")
	float EmissiveIntensity = 8.f;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PortalMID;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandlePhaseChanged(EMatchPhase OldPhase, EMatchPhase NewPhase);

	void BindToGameState();

	FTimerHandle BindRetryTimer;
};