#include "Items/GridItemVfx014AuditCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridItemVfx014, Log, All);

namespace ItemVFX014
{
    constexpr TCHAR HealingPotionPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.DA_Item_Potion_Healing_01");
    constexpr TCHAR SparklePath[] =
        TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle");

    constexpr const TCHAR* FlaskPaths[] =
    {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Acid.DA_Item_Flask_Acid"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_CorrosiveCloud.DA_Item_Flask_CorrosiveCloud")
    };

    const FName SparkleId(TEXT("LiquidSparkle"));

    struct FPendingFlask
    {
        UGridItemDefinitionAsset* Asset = nullptr;
        FGridItemVisualEffectsConfig PlannedVisuals;
        bool bWouldChange = false;
    };

    bool IsWhitelistedFlask(const FString& ObjectPath)
    {
        for (const TCHAR* Path : FlaskPaths)
        {
            if (ObjectPath == Path)
            {
                return true;
            }
        }
        return false;
    }
}

bool FGridItemVfx014Audit::BuildFlaskCleanup(
    const UGridItemDefinitionAsset& Flask,
    const UNiagaraSystem* Sparkle,
    FGridItemVisualEffectsConfig& OutVisuals,
    bool& bOutWouldChange,
    FString& OutError)
{
    OutError.Reset();
    OutVisuals = Flask.VisualEffects;
    bOutWouldChange = false;

    if (!IsValid(Sparkle) || !OutVisuals.IsValid() || !Flask.LightEmitter.IsValid())
    {
        OutError = TEXT("Invalid flask VFX/lighting configuration or unavailable canonical sparkle.");
        return false;
    }

    const FSoftObjectPath CanonicalPath(Sparkle);
    for (int32 Index = OutVisuals.Effects.Num() - 1; Index >= 0; --Index)
    {
        const FGridItemNiagaraEffectConfig& Effect = OutVisuals.Effects[Index];
        const bool bNamedSparkle = Effect.EffectId == ItemVFX014::SparkleId;
        const bool bUsesSparkle = Effect.NiagaraSystem.ToSoftObjectPath() == CanonicalPath;
        if (!bNamedSparkle && !bUsesSparkle)
        {
            continue;
        }

        if (!bNamedSparkle || !bUsesSparkle)
        {
            OutError = FString::Printf(
                TEXT("Refusing to remove ambiguous LiquidSparkle (ID or system mismatch) from %s."),
                *Flask.GetPathName());
            return false;
        }

        OutVisuals.Effects.RemoveAt(Index);
        bOutWouldChange = true;
    }

    if (!OutVisuals.IsValid())
    {
        OutError = TEXT("Removing LiquidSparkle would leave an invalid VFX configuration.");
        return false;
    }
    return true;
}

bool FGridItemVfx014Audit::ScanProduction(
    bool bAllowPendingFlaskCleanup,
    int32& OutScannedCount,
    FString& OutError)
{
    OutError.Reset();
    OutScannedCount = 0;

    FAssetRegistryModule& RegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& Registry = RegistryModule.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(UGridItemDefinitionAsset::StaticClass()->GetClassPathName(), Assets, true);
    if (Assets.IsEmpty())
    {
        OutError = TEXT("Asset registry returned zero item definitions; refusing a false-clean audit.");
        return false;
    }

    const FSoftObjectPath CanonicalSparkle(ItemVFX014::SparklePath);
    FString Report;
    int32 LegacyNiagaraCount = 0;
    int32 ForbiddenSparkleCount = 0;
    int32 MissingAssets = 0;
    bool bFoundHealingPotion = false;

    for (const FAssetData& AssetData : Assets)
    {
        UGridItemDefinitionAsset* Definition = Cast<UGridItemDefinitionAsset>(AssetData.GetAsset());
        if (!IsValid(Definition))
        {
            ++MissingAssets;
            UE_LOG(LogGridItemVfx014, Error, TEXT("[ITEM-VFX01.4A] LOAD_FAILURE %s"),
                *AssetData.PackageName.ToString());
            continue;
        }

        ++OutScannedCount;
        const FString Path = Definition->GetPathName();
        const bool bIsHealingPotion = Path == ItemVFX014::HealingPotionPath;
        bFoundHealingPotion |= bIsHealingPotion;

        const FGridLightEmitterConfig& Light = Definition->LightEmitter;
        const bool bLegacyNiagara = !Light.NiagaraSystem.IsNull();
        const bool bLegacyOffset = !Light.NiagaraRelativeLocation.IsNearlyZero() ||
            !Light.NiagaraRelativeRotation.IsNearlyZero();

        if (bLegacyNiagara || bLegacyOffset)
        {
            ++LegacyNiagaraCount;
            const FString Reference = Light.NiagaraSystem.ToSoftObjectPath().ToString();
            UE_LOG(LogGridItemVfx014, Error,
                TEXT("[ITEM-VFX01.4A] BLOCKER_LEGACY_LIGHT %s Niagara=%s Offset=(%s | %s)"),
                *Path, *Reference, *Light.NiagaraRelativeLocation.ToString(),
                *Light.NiagaraRelativeRotation.ToString());
            Report += FString::Printf(TEXT("Legacy light Niagara: %s\n"), *Path);
        }

        bool bHasCanonicalSparkle = false;
        for (const FGridItemNiagaraEffectConfig& Effect : Definition->VisualEffects.Effects)
        {
            const bool bUsesCanonicalSparkle = Effect.NiagaraSystem.ToSoftObjectPath() == CanonicalSparkle;
            const bool bNamedCanonicalSparkle = Effect.EffectId == ItemVFX014::SparkleId;
            if (!bUsesCanonicalSparkle && !bNamedCanonicalSparkle)
            {
                continue;
            }

            if (bIsHealingPotion)
            {
                if (bUsesCanonicalSparkle && bNamedCanonicalSparkle &&
                    Effect.ActivationMode == EGridItemVfxActivationMode::Always)
                {
                    bHasCanonicalSparkle = true;
                    continue;
                }
            }
            else if (bAllowPendingFlaskCleanup && ItemVFX014::IsWhitelistedFlask(Path) &&
                     bUsesCanonicalSparkle && bNamedCanonicalSparkle)
            {
                UE_LOG(LogGridItemVfx014, Display,
                    TEXT("[ITEM-VFX01.4A] PENDING_FLASK_CLEANUP %s"), *Path);
                continue;
            }

            ++ForbiddenSparkleCount;
            UE_LOG(LogGridItemVfx014, Error,
                TEXT("[ITEM-VFX01.4A] BLOCKER_SPARKLE_POLICY %s EffectId=%s Niagara=%s"),
                *Path, *Effect.EffectId.ToString(), *Effect.NiagaraSystem.ToSoftObjectPath().ToString());
            Report += FString::Printf(TEXT("Unexpected sparkle authoring: %s\n"), *Path);
        }

        if (bIsHealingPotion && !bHasCanonicalSparkle)
        {
            ++ForbiddenSparkleCount;
            UE_LOG(LogGridItemVfx014, Error,
                TEXT("[ITEM-VFX01.4A] BLOCKER_MISSING_HEALING_SPARKLE %s"), *Path);
            Report += TEXT("Healing potion missing canonical LiquidSparkle\n");
        }
    }

    if (!bFoundHealingPotion)
    {
        ++MissingAssets;
        Report += TEXT("Healing potion absent from AssetRegistry scan\n");
    }

    UE_LOG(LogGridItemVfx014, Display,
        TEXT("[ITEM-VFX01.4A] SCAN items=%d legacy=%d invalidSparkle=%d loadFailure=%d"),
        OutScannedCount, LegacyNiagaraCount, ForbiddenSparkleCount, MissingAssets);

    if (LegacyNiagaraCount > 0 || ForbiddenSparkleCount > 0 || MissingAssets > 0)
    {
        OutError = FString::Printf(
            TEXT("Asset audit blocks C++ schema removal (%d legacy, %d sparkle policy, %d load failures).\n%s"),
            LegacyNiagaraCount, ForbiddenSparkleCount, MissingAssets, *Report);
        return false;
    }

    UE_LOG(LogGridItemVfx014, Display,
        TEXT("[ITEM-VFX01.4A] AUDIT OK: no legacy Niagara in any scanned item definition."));
    return true;
}

bool FGridItemVfx014Audit::Run(bool bApply, FString& OutError)
{
    OutError.Reset();
    UNiagaraSystem* Sparkle = LoadObject<UNiagaraSystem>(nullptr, ItemVFX014::SparklePath);
    if (!IsValid(Sparkle))
    {
        OutError = TEXT("Cannot load NS_LiquideSparkle. Check Git LFS checkout.");
        return false;
    }

    // Prepare every targeted asset, validate all candidates and audit all
    // other item definitions BEFORE writing a single binary package.
    TArray<ItemVFX014::FPendingFlask> Pending;
    for (const TCHAR* Path : ItemVFX014::FlaskPaths)
    {
        UGridItemDefinitionAsset* Flask = LoadObject<UGridItemDefinitionAsset>(nullptr, Path);
        if (!IsValid(Flask))
        {
            OutError = FString::Printf(TEXT("Missing required flask: %s"), Path);
            return false;
        }

        ItemVFX014::FPendingFlask Plan;
        Plan.Asset = Flask;
        if (!BuildFlaskCleanup(*Flask, Sparkle, Plan.PlannedVisuals, Plan.bWouldChange, OutError))
        {
            return false;
        }

        UGridItemDefinitionAsset* Candidate =
            DuplicateObject<UGridItemDefinitionAsset>(Flask, GetTransientPackage(), NAME_None);
        if (!IsValid(Candidate))
        {
            OutError = FString::Printf(TEXT("Cannot duplicate flask %s for validation."), Path);
            return false;
        }
        Candidate->VisualEffects = Plan.PlannedVisuals;
        if (!Candidate->IsValidDefinition())
        {
            OutError = FString::Printf(TEXT("Proposed flask visual effects are invalid: %s"), Path);
            return false;
        }

        UE_LOG(LogGridItemVfx014, Display,
            TEXT("[ITEM-VFX01.4A] %s %s PlannedEffects=%d"),
            Plan.bWouldChange ? TEXT("READY_TO_REMOVE_SPARKLE") : TEXT("ALREADY_CLEAN"),
            Path, Plan.PlannedVisuals.Effects.Num());
        Pending.Add(MoveTemp(Plan));
    }

    int32 ScannedCount = 0;
    if (!ScanProduction(true, ScannedCount, OutError))
    {
        return false;
    }

    if (!bApply)
    {
        UE_LOG(LogGridItemVfx014, Display,
            TEXT("[ITEM-VFX01.4A] DRY_RUN OK: %d flasks planned, %d item definitions audited; no writes."),
            Pending.Num(), ScannedCount);
        return true;
    }

    int32 SavedCount = 0;
    for (const ItemVFX014::FPendingFlask& Plan : Pending)
    {
        if (!Plan.bWouldChange)
        {
            continue;
        }

        UGridItemDefinitionAsset* Flask = Plan.Asset;
        Flask->Modify();
        Flask->VisualEffects = Plan.PlannedVisuals;
        Flask->MarkPackageDirty();

        UPackage* Package = Flask->GetOutermost();
        const FString FileName = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Flask, *FileName, Args))
        {
            OutError = FString::Printf(TEXT("Saving %s failed after %d successful saves."),
                *FileName, SavedCount);
            return false;
        }

        ++SavedCount;
        UE_LOG(LogGridItemVfx014, Display, TEXT("[ITEM-VFX01.4A] SAVED %s"), *FileName);
    }

    // Re-scan without exemptions. An audit failure here is a release blocker;
    // the caller must not publish the resulting assets.
    if (!ScanProduction(false, ScannedCount, OutError))
    {
        return false;
    }

    UE_LOG(LogGridItemVfx014, Display,
        TEXT("[ITEM-VFX01.4A] APPLY OK: %d flasks saved, %d item definitions verified."),
        SavedCount, ScannedCount);
    return true;
}

UGridItemVfx014AuditCommandlet::UGridItemVfx014AuditCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridItemVfx014AuditCommandlet::Main(const FString& Params)
{
    FString Error;
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    if (!FGridItemVfx014Audit::Run(bApply, Error))
    {
        UE_LOG(LogGridItemVfx014, Error, TEXT("[ITEM-VFX01.4A] FAILED: %s"), *Error);
        return 1;
    }
    return 0;
}
