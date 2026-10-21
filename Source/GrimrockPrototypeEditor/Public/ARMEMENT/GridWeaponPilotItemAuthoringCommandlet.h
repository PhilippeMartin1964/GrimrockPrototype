#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponPilotItemAuthoringCommandlet.generated.h"

/** ARMEMENT01.6.6: create exactly three new item DataAssets after complete preflight. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponPilotItemAuthoringCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridWeaponPilotItemAuthoringCommandlet();
    virtual int32 Main(const FString& Params) override;
};
