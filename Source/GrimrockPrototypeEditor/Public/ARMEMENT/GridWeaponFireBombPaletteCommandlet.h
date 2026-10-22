#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponFireBombPaletteCommandlet.generated.h"

/** ARMEMENT01.10.2.3: non-destructive palette migration; -Apply is explicit. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponFireBombPaletteCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridWeaponFireBombPaletteCommandlet();
    virtual int32 Main(const FString& Params) override;
};
