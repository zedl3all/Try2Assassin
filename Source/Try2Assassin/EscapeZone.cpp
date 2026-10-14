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

	BindToGameState();
}

void AEscapeZone::BindToGameState()
{
	// GameState อาจยังไม่ replicate มาถึง client ตอน BeginPlay จึงลองซ้ำจนกว่าจะเจอ
	ATry2AssassinGameState* GS = GetWorld()->GetGameState<ATry2AssassinGameState>();
	if (!GS)
	{
		GetWorldTimerManager().SetTimer(BindRetryTimer, this,
			&AEscapeZone::BindToGameState, 0.2f, false);
		return;
	}

	GS->OnMatchPhaseChanged.AddUniqueDynamic(this, &AEscapeZone::HandlePhaseChanged);

	// เผื่อ client เข้าเกมทีหลังตอนที่ phase เป็น Escape ไปแล้ว
	SetPortalActive(GS->MatchPhase == EMatchPhase::Escape);
}

void AEscapeZone::HandlePhaseChanged(EMatchPhase OldPhase, EMatchPhase NewPhase)
{
	SetPortalActive(NewPhase == EMatchPhase::Escape);
}

void AEscapeZone::SetPortalActive(bool bActive)
{
	PortalMesh->SetVisibility(bActive);
	PortalLight->SetVisibility(bActive);
}

void AEscapeZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// ชาวบ้านวิ่งเข้าโซน -> หายไป
	if (AVillagerCharacter* Villager = Cast<AVillagerCharacter>(OtherActor))
	{
		Villager->Destroy();
		return;
	}

	APawn* P = Cast<APawn>(OtherActor);
	if (!P || !P->GetController()) { return; }

	if (ATry2AssassinGameMode* GM = GetWorld()->GetAuthGameMode<ATry2AssassinGameMode>())
	{
		GM->HandleReachedSafeZone(P->GetController());
	}
}