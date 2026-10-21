#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponPilotPaletteAuthoringCommandlet.generated.h"

/** ARMEMENT01.7.1: add only the three pilot collectibles to the existing production palette. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponPilotPaletteAuthoringCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridWeaponPilotPaletteAuthoringCommandlet();
    virtual int32 Main(const FString& Params) override;
};
