#include "Items/GridItemLiquidVfxMigrationCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridItemVfxMigration, Log, All);

namespace ItemVFX012
{
    constexpr TCHAR SparklePath[] =
        TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle");

    // Strict allowlist: do not migrate torches, gems or unrelated gameplay items.
    constexpr const TCHAR* ItemPaths[] =
    {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.DA_Item_Potion_Healing_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Acid.DA_Item_Flask_Acid"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_CorrosiveCloud.DA_Item_Flask_CorrosiveCloud")
    };
    const FName SparkleId(TEXT("LiquidSparkle"));

    struct FPendingItem
    {
        UGridItemDefinitionAsset* Asset = nullptr;
        FGridItemVisualEffectsConfig Visuals;
        FGridLightEmitterConfig Light;
        bool bWouldChange = false;
    };
}

bool FGridItemLiquidVfxMigration::BuildTarget(
    const UGridItemDefinitionAsset& Item,
    UNiagaraSystem* SparkleSystem,
    FGridItemVisualEffectsConfig& OutVisuals,
    FGridLightEmitterConfig& OutLight,
    bool& bOutWouldChange,
    FString& OutError)
{
    OutError.Reset();
    bOutWouldChange = false;
    OutVisuals = Item.VisualEffects;
    OutLight = Item.LightEmitter;

    if (!IsValid(SparkleSystem) || !OutVisuals.IsValid() || !OutLight.IsValid())
    {
        OutError = TEXT("Missing Niagara sparkle system or invalid source configuration.");
        return false;
    }

    const FSoftObjectPath DesiredPath(SparkleSystem);
    const bool bHasLegacyNiagara = !Item.LightEmitter.NiagaraSystem.IsNull();
    if (bHasLegacyNiagara && Item.LightEmitter.NiagaraSystem.ToSoftObjectPath() != DesiredPath)
    {
        OutError = FString::Printf(TEXT("Refusing to replace an unrelated legacy Niagara system on %s."),
            *Item.GetPathName());
        return false;
    }

    int32 ExistingIndex = INDEX_NONE;
    for (int32 Index = 0; Index < OutVisuals.Effects.Num(); ++Index)
    {
        const FGridItemNiagaraEffectConfig& Entry = OutVisuals.Effects[Index];
        if (Entry.EffectId == ItemVFX012::SparkleId ||
            (Entry.NiagaraSystem.ToSoftObjectPath() == DesiredPath &&
             Entry.ActivationMode == EGridItemVfxActivationMode::Always))
        {
            ExistingIndex = Index;
            break;
        }
    }

    if (ExistingIndex != INDEX_NONE)
    {
        const FGridItemNiagaraEffectConfig& Existing = OutVisuals.Effects[ExistingIndex];
        if (Existing.NiagaraSystem.ToSoftObjectPath() != DesiredPath ||
            Existing.ActivationMode != EGridItemVfxActivationMode::Always)
        {
            OutError = FString::Printf(TEXT("Conflicting LiquidSparkle entry on %s."), *Item.GetPathName());
            return false;
        }
    }
    else
    {
        FGridItemNiagaraEffectConfig Entry;
        Entry.EffectId = ItemVFX012::SparkleId;
        Entry.NiagaraSystem = SparkleSystem;
        Entry.ActivationMode = EGridItemVfxActivationMode::Always;

        // Preserve existing authored placement where the legacy Niagara was configured.
        if (bHasLegacyNiagara)
        {
            Entry.RelativeTransform = FTransform(
                Item.LightEmitter.NiagaraRelativeRotation, Item.LightEmitter.NiagaraRelativeLocation);
        }
        OutVisuals.Effects.Add(Entry);
        bOutWouldChange = true;
    }

    if (bHasLegacyNiagara)
    {
        OutLight.NiagaraSystem.Reset();
        OutLight.NiagaraRelativeLocation = FVector::ZeroVector;
        OutLight.NiagaraRelativeRotation = FRotator::ZeroRotator;
        bOutWouldChange = true;
    }

    // Preserve point-light settings in full, even for a potion/flask with a light.
    if (!OutVisuals.IsValid() || !OutLight.IsValid())
    {
        OutError = TEXT("Migration would generate an invalid visual or light configuration.");
        return false;
    }
    return true;
}

bool FGridItemLiquidVfxMigration::Run(bool bApply, FString& OutError)
{
    OutError.Reset();
    UNiagaraSystem* Sparkle = LoadObject<UNiagaraSystem>(nullptr, ItemVFX012::SparklePath);
    if (!IsValid(Sparkle))
    {
        OutError = FString::Printf(TEXT("Cannot load required Niagara system: %s"), ItemVFX012::SparklePath);
        return false;
    }

    TArray<ItemVFX012::FPendingItem> Pending;
    for (const TCHAR* Path : ItemVFX012::ItemPaths)
    {
        UGridItemDefinitionAsset* Asset = LoadObject<UGridItemDefinitionAsset>(nullptr, Path);
        if (!IsValid(Asset))
        {
            OutError = FString::Printf(TEXT("Missing production DataAsset (Git LFS checkout required): %s"), Path);
            return false;
        }

        ItemVFX012::FPendingItem Plan;
        Plan.Asset = Asset;
        if (!BuildTarget(*Asset, Sparkle, Plan.Visuals, Plan.Light, Plan.bWouldChange, OutError))
        {
            return false;
        }

        UGridItemDefinitionAsset* Candidate = DuplicateObject<UGridItemDefinitionAsset>(
            Asset, GetTransientPackage(), NAME_None);
        if (!IsValid(Candidate))
        {
            OutError = FString::Printf(TEXT("Unable to duplicate candidate for validation: %s"), Path);
            return false;
        }
        Candidate->VisualEffects = Plan.Visuals;
        Candidate->LightEmitter = Plan.Light;
        if (!Candidate->IsValidDefinition())
        {
            OutError = FString::Printf(TEXT("Proposed migration fails item validation: %s"), Path);
            return false;
        }

        UE_LOG(LogGridItemVfxMigration, Display,
            TEXT("[ITEM-VFX01.2] %s %s LegacyNiagara=%s Effects=%d"),
            Plan.bWouldChange ? TEXT("READY") : TEXT("ALREADY_MIGRATED"),
            Path,
            Asset->LightEmitter.NiagaraSystem.IsNull() ? TEXT("none") : TEXT("configured"),
            Plan.Visuals.Effects.Num());
        Pending.Add(MoveTemp(Plan));
    }

    if (!bApply)
    {
        UE_LOG(LogGridItemVfxMigration, Display, TEXT("[ITEM-VFX01.2] DRY_RUN OK: %d assets, no package writes."), Pending.Num());
        return true;
    }

    int32 SavedCount = 0;
    for (const ItemVFX012::FPendingItem& Plan : Pending)
    {
        if (!Plan.bWouldChange)
        {
            continue;
        }

        UGridItemDefinitionAsset* Asset = Plan.Asset;
        Asset->Modify();
        Asset->VisualEffects = Plan.Visuals;
        Asset->LightEmitter = Plan.Light;
        Asset->MarkPackageDirty();

        UPackage* Package = Asset->GetOutermost();
        const FString PackageName = Package->GetName();
        const FString FileName = FPackageName::LongPackageNameToFilename(
            PackageName, FPackageName::GetAssetPackageExtension());

        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Asset, *FileName, Args))
        {
            OutError = FString::Printf(
                TEXT("Could not save %s (some previous assets may have been saved)."),
                *FileName);
            return false;
        }

        ++SavedCount;
        UE_LOG(LogGridItemVfxMigration, Display, TEXT("[ITEM-VFX01.2] SAVED %s"), *FileName);
    }

    UE_LOG(LogGridItemVfxMigration, Display,
        TEXT("[ITEM-VFX01.2] APPLY OK: %d updated; %d checked; idempotent on rerun."),
        SavedCount, Pending.Num());
    return true;
}

UGridItemLiquidVfxMigrationCommandlet::UGridItemLiquidVfxMigrationCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridItemLiquidVfxMigrationCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Error;
    if (!FGridItemLiquidVfxMigration::Run(bApply, Error))
    {
        UE_LOG(LogGridItemVfxMigration, Error, TEXT("[ITEM-VFX01.2] FAILED: %s"), *Error);
        return 1;
    }
    return 0;
}
