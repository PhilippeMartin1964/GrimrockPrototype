#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridItemTorchVfxMigrationCommandlet.generated.h"

class UNiagaraSystem;
class UGridItemDefinitionAsset;
struct FGridItemVisualEffectsConfig;
struct FGridLightEmitterConfig;

/**
 * ITEM-VFX01.3: move only the production torch's Niagara flame from
 * LightEmitter to the independently executed WhenLightEnabled VFX channel.
 * The authored point light and its runtime state remain unchanged.
 */
struct FGridItemTorchVfxMigration
{
    static bool BuildTarget(
        const UGridItemDefinitionAsset& Torch,
        UNiagaraSystem* FlameSystem,
        FGridItemVisualEffectsConfig& OutVisuals,
        FGridLightEmitterConfig& OutLight,
        bool& bOutWouldChange,
        FString& OutError);

    /** Read-only by default; -Apply saves the whitelisted DA_Item_Torch package. */
    static bool Run(bool bApply, FString& OutError);
};

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridItemTorchVfxMigrationCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridItemTorchVfxMigrationCommandlet();
    virtual int32 Main(const FString& Params) override;
};
