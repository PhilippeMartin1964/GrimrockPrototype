#include "Items/GridItemTorchVfxMigrationCommandlet.h"

#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridItemTorchVfxMigration, Log, All);

namespace ItemVFX013
{
    constexpr TCHAR TorchPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.DA_Item_Torch");
    constexpr TCHAR FlamePath[] =
        TEXT("/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch.NS_Flame_8_Torch");
    const FName FlameId(TEXT("TorchFlame"));
}

bool FGridItemTorchVfxMigration::BuildTarget(
    const UGridItemDefinitionAsset& Torch,
    UNiagaraSystem* FlameSystem,
    FGridItemVisualEffectsConfig& OutVisuals,
    FGridLightEmitterConfig& OutLight,
    bool& bOutWouldChange,
    FString& OutError)
{
    OutError.Reset();
    bOutWouldChange = false;
    OutVisuals = Torch.VisualEffects;
    OutLight = Torch.LightEmitter;

    if (Torch.ItemType != EGridItemType::Torch || !IsValid(FlameSystem) ||
        !OutVisuals.IsValid() || !OutLight.IsValid() || !OutLight.bUsePointLight)
    {
        OutError = TEXT("The item is not a valid point-light torch, or the Niagara source is unavailable.");
        return false;
    }

    const FSoftObjectPath CanonicalFlame(FlameSystem);
    const bool bHasLegacyFlame = !OutLight.NiagaraSystem.IsNull();
    if (bHasLegacyFlame && OutLight.NiagaraSystem.ToSoftObjectPath() != CanonicalFlame)
    {
        OutError = FString::Printf(TEXT("Refusing to replace unrelated legacy Niagara on %s."),
            *Torch.GetPathName());
        return false;
    }

    int32 FlameIndex = INDEX_NONE;
    for (int32 Index = 0; Index < OutVisuals.Effects.Num(); ++Index)
    {
        const FGridItemNiagaraEffectConfig& Entry = OutVisuals.Effects[Index];
        if (Entry.EffectId == ItemVFX013::FlameId ||
            Entry.NiagaraSystem.ToSoftObjectPath() == CanonicalFlame)
        {
            if (FlameIndex != INDEX_NONE)
            {
                OutError = TEXT("Multiple torch flame effects found; refusing to create duplicates.");
                return false;
            }
            FlameIndex = Index;
        }
    }

    if (FlameIndex != INDEX_NONE)
    {
        const FGridItemNiagaraEffectConfig& Existing = OutVisuals.Effects[FlameIndex];
        if (Existing.EffectId != ItemVFX013::FlameId ||
            Existing.NiagaraSystem.ToSoftObjectPath() != CanonicalFlame ||
            Existing.ActivationMode != EGridItemVfxActivationMode::WhenLightEnabled)
        {
            OutError = TEXT("Conflicting torch flame effect (asset or activation mode).");
            return false;
        }
        if (bHasLegacyFlame)
        {
            const FTransform LegacyTransform(
                OutLight.NiagaraRelativeRotation, OutLight.NiagaraRelativeLocation);
            if (!Existing.RelativeTransform.Equals(LegacyTransform, 0.01f))
            {
                OutError = TEXT("Torch already has a conflicting Niagara transform; manual review required.");
                return false;
            }
        }
    }
    else
    {
        // No guessing if the original torch flame has already disappeared.
        if (!bHasLegacyFlame)
        {
            OutError = TEXT("Torch has neither legacy flame nor migrated TorchFlame entry.");
            return false;
        }

        FGridItemNiagaraEffectConfig Flame;
        Flame.EffectId = ItemVFX013::FlameId;
        Flame.NiagaraSystem = FlameSystem;
        Flame.RelativeTransform = FTransform(
            OutLight.NiagaraRelativeRotation, OutLight.NiagaraRelativeLocation);
        Flame.ActivationMode = EGridItemVfxActivationMode::WhenLightEnabled;
        OutVisuals.Effects.Add(Flame);
        bOutWouldChange = true;
    }

    if (bHasLegacyFlame)
    {
        // Move flame presentation only. Leave every PointLight/flicker field
        // and the instance-controlled default enabled state untouched.
        OutLight.NiagaraSystem.Reset();
        OutLight.NiagaraRelativeLocation = FVector::ZeroVector;
        OutLight.NiagaraRelativeRotation = FRotator::ZeroRotator;
        bOutWouldChange = true;
    }

    if (!OutVisuals.IsValid() || !OutLight.IsValid() || !OutLight.HasEmitter())
    {
        OutError = TEXT("Migration would invalidate Niagara effects or extinguish the torch light authority.");
        return false;
    }
    return true;
}

bool FGridItemTorchVfxMigration::Run(bool bApply, FString& OutError)
{
    OutError.Reset();
    UNiagaraSystem* Flame = LoadObject<UNiagaraSystem>(nullptr, ItemVFX013::FlamePath);
    UGridItemDefinitionAsset* Torch = LoadObject<UGridItemDefinitionAsset>(nullptr, ItemVFX013::TorchPath);
    if (!IsValid(Flame) || !IsValid(Torch))
    {
        OutError = FString::Printf(
            TEXT("Missing production torch or flame Niagara. Check Git LFS checkout: %s | %s"),
            ItemVFX013::TorchPath, ItemVFX013::FlamePath);
        return false;
    }

    FGridItemVisualEffectsConfig PlannedVisuals;
    FGridLightEmitterConfig PlannedLight;
    bool bWouldChange = false;
    if (!BuildTarget(*Torch, Flame, PlannedVisuals, PlannedLight, bWouldChange, OutError))
    {
        return false;
    }

    UGridItemDefinitionAsset* Candidate =
        DuplicateObject<UGridItemDefinitionAsset>(Torch, GetTransientPackage(), NAME_None);
    if (!IsValid(Candidate))
    {
        OutError = TEXT("Unable to duplicate torch for non-mutating preflight.");
        return false;
    }
    Candidate->VisualEffects = PlannedVisuals;
    Candidate->LightEmitter = PlannedLight;
    if (!Candidate->IsValidDefinition() ||
        Candidate->HasLightEmitter() != Torch->HasLightEmitter() ||
        Candidate->IsLightEnabledByDefault() != Torch->IsLightEnabledByDefault())
    {
        OutError = TEXT("Torch migration fails item/light state validation.");
        return false;
    }

    UE_LOG(LogGridItemTorchVfxMigration, Display,
        TEXT("[ITEM-VFX01.3] %s %s LegacyFlame=%s Effects=%d PointLight=%s DefaultOn=%s"),
        bWouldChange ? TEXT("READY") : TEXT("ALREADY_MIGRATED"),
        ItemVFX013::TorchPath,
        Torch->LightEmitter.NiagaraSystem.IsNull() ? TEXT("none") : TEXT("configured"),
        PlannedVisuals.Effects.Num(),
        PlannedLight.bUsePointLight ? TEXT("true") : TEXT("false"),
        PlannedLight.bDefaultEnabled ? TEXT("true") : TEXT("false"));

    if (!bApply)
    {
        UE_LOG(LogGridItemTorchVfxMigration, Display,
            TEXT("[ITEM-VFX01.3] DRY_RUN OK: torch checked; zero package writes."));
        return true;
    }
    if (!bWouldChange)
    {
        UE_LOG(LogGridItemTorchVfxMigration, Display,
            TEXT("[ITEM-VFX01.3] APPLY NOOP: torch already migrated."));
        return true;
    }

    Torch->Modify();
    Torch->VisualEffects = PlannedVisuals;
    Torch->LightEmitter = PlannedLight;
    Torch->MarkPackageDirty();

    UPackage* Package = Torch->GetOutermost();
    const FString FileName = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Torch, *FileName, Args))
    {
        OutError = FString::Printf(TEXT("Failed to save migrated torch: %s"), *FileName);
        return false;
    }

    UE_LOG(LogGridItemTorchVfxMigration, Display,
        TEXT("[ITEM-VFX01.3] SAVED %s"), *FileName);
    UE_LOG(LogGridItemTorchVfxMigration, Display,
        TEXT("[ITEM-VFX01.3] APPLY OK: 1 torch asset migrated; idempotent on rerun."));
    return true;
}

UGridItemTorchVfxMigrationCommandlet::UGridItemTorchVfxMigrationCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridItemTorchVfxMigrationCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Error;
    if (!FGridItemTorchVfxMigration::Run(bApply, Error))
    {
        UE_LOG(LogGridItemTorchVfxMigration, Error, TEXT("[ITEM-VFX01.3] FAILED: %s"), *Error);
        return 1;
    }
    return 0;
}
