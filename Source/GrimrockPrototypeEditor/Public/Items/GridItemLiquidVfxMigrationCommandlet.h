#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridItemLiquidVfxMigrationCommandlet.generated.h"

class UGridItemDefinitionAsset;
class UNiagaraSystem;
struct FGridItemVisualEffectsConfig;
struct FGridLightEmitterConfig;

/**
 * ITEM-VFX01.2: deterministic, explicitly triggered migration of authored
 * potion/flask .uasset data; no runtime hardcoded item-name behavior.
 */
struct FGridItemLiquidVfxMigration
{
    static bool BuildTarget(
        const UGridItemDefinitionAsset& Item,
        UNiagaraSystem* SparkleSystem,
        FGridItemVisualEffectsConfig& OutVisuals,
        FGridLightEmitterConfig& OutLight,
        bool& bOutWouldChange,
        FString& OutError);

    /** Dry run by default. Apply saves only the four whitelisted production assets. */
    static bool Run(bool bApply, FString& OutError);
};

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridItemLiquidVfxMigrationCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridItemLiquidVfxMigrationCommandlet();
    virtual int32 Main(const FString& Params) override;
};
