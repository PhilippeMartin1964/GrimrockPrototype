#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridItemVfx014AuditCommandlet.generated.h"

class UGridItemDefinitionAsset;
class UNiagaraSystem;
struct FGridItemVisualEffectsConfig;

/**
 * ITEM-VFX01.4A: read legacy item-light Niagara before schema removal.
 * Only the health potion may author NS_LiquideSparkle; other item VFX
 * systems and point-light settings are never overwritten.
 */
struct FGridItemVfx014Audit
{
    static bool BuildFlaskCleanup(
        const UGridItemDefinitionAsset& Flask,
        const UNiagaraSystem* Sparkle,
        FGridItemVisualEffectsConfig& OutVisuals,
        bool& bOutWouldChange,
        FString& OutError);

    /** Reports every UGridItemDefinitionAsset with legacy Niagara or forbidden sparkle. */
    static bool ScanProduction(
        bool bAllowPendingFlaskCleanup,
        int32& OutScannedCount,
        FString& OutError);

    /** Dry-run by default. Apply removes only LiquidSparkle from three whitelisted flasks. */
    static bool Run(bool bApply, FString& OutError);
};

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridItemVfx014AuditCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridItemVfx014AuditCommandlet();
    virtual int32 Main(const FString& Params) override;
};
