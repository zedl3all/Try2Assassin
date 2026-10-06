// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeZone.generated.h"   // ต้องเป็น include สุดท้ายเสมอ

class UBoxComponent;

UCLASS()
class TRY2ASSASSIN_API AEscapeZone : public AActor
{
	GENERATED_BODY()

public:
	AEscapeZone();

protected:
	virtual void BeginPlay() override;

	// กล่องตรวจจับผู้เล่นที่เข้ามาในโซน (ปรับขนาดได้ใน Details ของ instance)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EscapeZone")
	TObjectPtr<UBoxComponent> Box;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};