#include "VillagerCharacter.h"
#include "MyAIController.h"
#include "EscapeZone.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Try2AssassinGameMode.h"


AVillagerCharacter::AVillagerCharacter()
{
	AIControllerClass = AMyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	GetCharacterMovement()->bUseRVOAvoidance = false;
	//GetCharacterMovement()->bUseRVOAvoidance = true;
	//GetCharacterMovement()->AvoidanceConsiderationRadius = 150.0f;
	//GetCharacterMovement()->AvoidanceWeight = 0.5f;
}

void AVillagerCharacter::StartPanicEscape(float PanicSpeed)
{
	if (!HasAuthority()) { return; }

	GetCharacterMovement()->MaxWalkSpeed = PanicSpeed;

	AMyAIController* AI = Cast<AMyAIController>(GetController());
	if (!AI) { return; }

	// ใช้ EscapeZone ที่ GameMode cache ไว้ตอน BeginPlay
	// (เดิมชาวบ้านแต่ละตัวเรียก GetAllActorsOfClass เอง = สแกนโลกซ้ำ 22 ครั้ง)
	ATry2AssassinGameMode* GM = GetWorld()->GetAuthGameMode<ATry2AssassinGameMode>();
	if (!GM) { return; }

	AEscapeZone* Nearest = GM->FindNearestEscapeZone(GetActorLocation());
	if (!Nearest) { return; }

	AI->StartEscape(Nearest);
}