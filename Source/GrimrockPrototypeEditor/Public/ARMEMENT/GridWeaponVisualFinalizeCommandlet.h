#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponVisualFinalizeCommandlet.generated.h"

/** ARMEMENT01.6.3-FINAL: safe validation and explicit authoring of three accepted UE visual profiles. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponVisualFinalizeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridWeaponVisualFinalizeCommandlet();
	virtual int32 Main(const FString& Params) override;
};
