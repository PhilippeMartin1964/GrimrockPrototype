#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponPipelineCommandlet.generated.h"

/**
 * ARMEMENT-INDUS02: single manifest-driven armament commandlet.
 * Audit/Plan/Validate are read-only. Build -Apply -Step=Visual|Item
 * saves at most one asset package per invocation.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponPipelineCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridWeaponPipelineCommandlet();
    virtual int32 Main(const FString& Params) override;
};
