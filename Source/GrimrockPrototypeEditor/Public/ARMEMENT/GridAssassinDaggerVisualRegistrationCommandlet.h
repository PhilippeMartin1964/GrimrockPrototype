#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridAssassinDaggerVisualRegistrationCommandlet.generated.h"

/**
 * ARMEMENT01.11.2.1: editor-only registration of approved UE-imported art.
 * Defaults to read-only diagnostics for one weapon (ARM002).
 * -Apply requires -Archetype=ARM002.
 * Writes only DA_WeaponVisualCatalogue, adding one binding and profile.
 * Never creates images/meshes, repairs art, or modifies existing bindings.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridAssassinDaggerVisualRegistrationCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridAssassinDaggerVisualRegistrationCommandlet();
    virtual int32 Main(const FString& Params) override;
};
