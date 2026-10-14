#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "VillagerCharacter.generated.h"

UCLASS()
class TRY2ASSASSIN_API AVillagerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AVillagerCharacter();
	
	// สั่งให้ชาวบ้านตื่นตระหนกและวิ่งหนีไปที่ EscapeZone ที่ใกล้ที่สุด (server เท่านั้น)
	void StartPanicEscape(float PanicSpeed);
};