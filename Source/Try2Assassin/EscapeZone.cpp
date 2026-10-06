// Fill out your copyright notice in the Description page of Project Settings.

#include "EscapeZone.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Try2AssassinGameMode.h"
#include "DrawDebugHelpers.h"
#include "Components/TextRenderComponent.h"

AEscapeZone::AEscapeZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(300.f, 300.f, 200.f));
	Box->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	Box->SetGenerateOverlapEvents(true);
	RootComponent = Box;
}

void AEscapeZone::BeginPlay()
{
	Super::BeginPlay();

	// วาดกรอบกล่องค้างไว้ตลอด (ทุกเครื่องเห็น) ปรับสีได้ที่ FColor
	DrawDebugBox(GetWorld(), GetActorLocation(), Box->GetScaledBoxExtent(),
		GetActorQuat(), FColor::Green, /*bPersistentLines=*/true, /*LifeTime=*/-1.f,
		/*DepthPriority=*/0, /*Thickness=*/8.f);

	// ผูก event เฉพาะบน server (GameMode มีแค่บน server)
	if (HasAuthority())
	{
		Box->OnComponentBeginOverlap.AddDynamic(this, &AEscapeZone::OnOverlapBegin);
	}
}

void AEscapeZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APawn* P = Cast<APawn>(OtherActor);
	if (!P || !P->GetController()) { return; }

	if (ATry2AssassinGameMode* GM = GetWorld()->GetAuthGameMode<ATry2AssassinGameMode>())
	{
		GM->HandleReachedSafeZone(P->GetController());
	}
}