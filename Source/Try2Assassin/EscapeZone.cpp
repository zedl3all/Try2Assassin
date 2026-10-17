#include "EscapeZone.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Try2AssassinGameMode.h"
#include "VillagerCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

AEscapeZone::AEscapeZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(300.f, 300.f, 200.f));
	Box->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	Box->SetGenerateOverlapEvents(true);
	RootComponent = Box;
	
	Tags.Add(FName("Exit"));                // Tag ของ Actor
	Box->ComponentTags.Add(FName("Exit"));  // Tag ของกรอบสี่เหลี่ยม (component)

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootComponent);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalMesh->SetCastShadow(false);
	PortalMesh->SetVisibility(false);

	// ใช้ Cylinder พื้นฐานของ Engine เป็นเสาแสง (เปลี่ยนใน Blueprint ได้)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		PortalMesh->SetStaticMesh(CylinderMesh.Object);
		// Box extent 300x300x200 -> กว้าง 6 เมตร สูง 4 เมตร (Cylinder พื้นฐานขนาด 100 หน่วย)
		PortalMesh->SetRelativeScale3D(FVector(6.f, 6.f, 4.f));
	}

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(RootComponent);
	PortalLight->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	PortalLight->SetIntensity(20000.f);
	PortalLight->SetAttenuationRadius(1200.f);
	PortalLight->SetCastShadows(false);
	PortalLight->SetVisibility(false);
}

void AEscapeZone::BeginPlay()
{
	Super::BeginPlay();

	// สร้าง Dynamic Material เพื่อปรับสีและความสว่าง
	// ถ้า Material ที่ใส่ใน PortalMesh ไม่มี parameter "Color"/"Intensity" ก็ไม่เป็นไร แค่สีจะไม่เปลี่ยน
	if (PortalMesh && PortalMesh->GetMaterial(0))
	{
		PortalMID = PortalMesh->CreateDynamicMaterialInstance(0);
		if (PortalMID)
		{
			PortalMID->SetVectorParameterValue(TEXT("Color"), PortalColor);
			PortalMID->SetScalarParameterValue(TEXT("Intensity"), EmissiveIntensity);
		}
	}
	PortalLight->SetLightColor(PortalColor);

	if (HasAuthority())
	{
		Box->OnComponentBeginOverlap.AddDynamic(this, &AEscapeZone::OnOverlapBegin);
	}
}

void AEscapeZone::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEscapeZone, bPortalActive);
}

void AEscapeZone::OnRep_PortalActive()
{
	ApplyPortalState();
}

void AEscapeZone::SetPortalActive(bool bActive)
{
	// server เป็นคนตัดสิน แล้ว replicate ไป client
	if (!HasAuthority()) { return; }

	bPortalActive = bActive;
	ApplyPortalState(); // listen server ไม่เรียก OnRep จึงต้อง apply เอง
}

void AEscapeZone::ApplyPortalState()
{
	// คุมแค่การมองเห็น ส่วน collision เปิดตลอดเพื่อให้ชาวบ้านวิ่งเข้าแล้วหายได้
	if (PortalMesh) { PortalMesh->SetVisibility(bPortalActive); }
	if (PortalLight) { PortalLight->SetVisibility(bPortalActive); }
}

void AEscapeZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// ชาวบ้านวิ่งเข้าโซน -> หายไป (ทุกโซน ไม่ว่าถูกสุ่มให้ active หรือไม่)
	if (AVillagerCharacter* Villager = Cast<AVillagerCharacter>(OtherActor))
	{
		Villager->Destroy();
		return;
	}

	APawn* P = Cast<APawn>(OtherActor);
	if (!P || !P->GetController()) { return; }

	// ผู้เล่นชนะได้เฉพาะโซนที่ถูกเปิด (active) เท่านั้น
	if (!bPortalActive) { return; }

	if (ATry2AssassinGameMode* GM = GetWorld()->GetAuthGameMode<ATry2AssassinGameMode>())
	{
		GM->HandleReachedSafeZone(P->GetController());
	}
}