#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWarriorN1VisualRegistrationCommandlet.generated.h"

/**
 * ARMEMENT01.10.2.1: editor-only registration of approved UE-imported art.
 * Defaults to read-only diagnostics for four weapons.
 * -Apply requires -Archetype=ARM014/ARM015/ARM025/ARM031.
 * Writes only DA_WeaponVisualCatalogue, adding one binding and profile.
 * Never creates images/meshes, repairs art, or modifies existing bindings.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWarriorN1VisualRegistrationCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridWarriorN1VisualRegistrationCommandlet();
    virtual int32 Main(const FString& Params) override;
};
