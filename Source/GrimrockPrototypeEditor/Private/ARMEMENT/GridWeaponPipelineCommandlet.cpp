#include "ARMEMENT/GridWeaponPipelineCommandlet.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace WeaponPipeline
{
    const TCHAR* const RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    const TCHAR* const VisualsPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");
    const TCHAR* const ItemFolder =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");

    struct FState
    {
        FString Art = TEXT("BLOCKED");
        FString Visual = TEXT("MISSING");
        FString Item = TEXT("MISSING");
        FString Summary = TEXT("BLOCKED");
        FString Detail = TEXT("None");
    };

    FString PackageForObject(const FString& ObjectPath)
    {
        return FPackageName::ObjectPathToPackageName(ObjectPath);
    }

    bool PackageFileExists(const FString& ObjectPath)
    {
        const FString File = FPackageName::LongPackageNameToFilename(
            PackageForObject(ObjectPath), FPackageName::GetAssetPackageExtension());
        return FPaths::FileExists(File);
    }

    bool LoadArt(const FGridWeaponPipelineRow& Spec, FString& Detail)
    {
        UTexture2D* Icon = LoadObject<UTexture2D>(nullptr, *Spec.IconPath);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Spec.MeshPath);
        UMaterialInstance* Material =
            LoadObject<UMaterialInstance>(nullptr, *Spec.MaterialPath);
        if (!IsValid(Icon) || !IsValid(Mesh) || !IsValid(Material))
        {
            Detail = TEXT("MissingIconMeshOrMaterial");
            return false;
        }
        const FIntPoint IconSize = Icon->GetImportedSize();
        if (IconSize.X < 256 || IconSize.X != IconSize.Y)
        {
            Detail = TEXT("InvalidIconSize");
            return false;
        }
        if (Mesh->GetNumLODs() <= 0 || Mesh->GetBounds().BoxExtent.ContainsNaN() ||
            Mesh->GetBounds().BoxExtent.GetMax() < 5.0)
        {
            Detail = TEXT("InvalidMeshBoundsOrLODs");
            return false;
        }
        if (!IsValid(Material->Parent))
        {
            Detail = TEXT("MissingMaterialParent");
            return false;
        }
        bool bUsed = false;
        if (Mesh->GetStaticMaterials().IsEmpty())
        {
            Detail = TEXT("NoMaterialSlots");
            return false;
        }
        for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
        {
            if (!IsValid(Slot.MaterialInterface))
            {
                Detail = TEXT("EmptyMeshMaterialSlot");
                return false;
            }
            bUsed |= Slot.MaterialInterface == Material;
        }
        if (!bUsed)
        {
            Detail = TEXT("MaterialNotInMeshSlots");
            return false;
        }
        const FString TexturePaths[] =
        {
            Spec.BaseColorPath, Spec.NormalPath, Spec.OrmPath
        };
        for (int32 Index = 0; Index < 3; ++Index)
        {
            UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePaths[Index]);
            if (!IsValid(Texture))
            {
                Detail = FString::Printf(TEXT("MissingPBR%d"), Index);
                return false;
            }
            const FIntPoint Imported = Texture->GetImportedSize();
            if (Imported.X < 1 || Imported.Y < 1 ||
                Texture->SRGB != (Index == 0))
            {
                Detail = FString::Printf(TEXT("InvalidPBR%dSizeOrSRGB"), Index);
                return false;
            }
        }
        Detail = TEXT("ArtChecked");
        return true;
    }

    bool IsVisualSame(const FGridWeaponPipelineRow& Spec,
        const UGridWeaponVisualCatalogueAsset& Visuals)
    {
        if (Visuals.FindVisualId(FName(Spec.ArchetypeId)) != FName(Spec.VisualId))
        {
            return false;
        }
        const FGridWeaponVisualProfile* Look =
            Visuals.FindProfileForArchetype(FName(Spec.ArchetypeId));
        return Look && Look->IsValidProfile() &&
            Look->VisualId == FName(Spec.VisualId) &&
            Look->FamilyTag == FName(Spec.VisualFamily) &&
            Look->BaseIcon.ToSoftObjectPath() == FSoftObjectPath(Spec.IconPath) &&
            Look->WorldMesh.ToSoftObjectPath() == FSoftObjectPath(Spec.MeshPath);
    }

    bool KindAgreesWithSource(const FGridWeaponPipelineRow& Spec,
        const FGridWeaponLevelProfile& N1)
    {
        if (!N1.IsValidProfile())
        {
            return false;
        }
        if (Spec.IsPassive())
        {
            return !N1.DamageDice.bEnabled && N1.ActionPointCost == 0 &&
                N1.RangeCells == 0;
        }
        const bool bTwoHands = Spec.BuildKind == TEXT("MeleeTwoHand") ||
            Spec.BuildKind == TEXT("Ranged");
        if (N1.HandUsage != (bTwoHands ? TEXT("2 mains") : TEXT("1 main")))
        {
            return false;
        }
        FGridAttackDefinition Attack;
        return N1.DamageDice.bEnabled && N1.TryBuildAttackDefinition(Attack) &&
            N1.ActionPointCost >= 1 && N1.RangeCells >= 1;
    }

    EGridItemHandUsage Usage(const FGridWeaponPipelineRow& Spec)
    {
        if (Spec.IsPassive()) return EGridItemHandUsage::NotHandHeld;
        if (Spec.BuildKind == TEXT("MeleeTwoHand") || Spec.BuildKind == TEXT("Ranged"))
            return EGridItemHandUsage::TwoHanded;
        return EGridItemHandUsage::OneHanded;
    }

    bool AllowsSlot(const FGridWeaponPipelineRow& Spec, EGridEquipmentSlot Slot)
    {
        return Slot == EGridEquipmentSlot::MainHand
            ? Spec.EquipmentSlots.Contains(TEXT("MainHand"))
            : (Slot == EGridEquipmentSlot::OffHand &&
                Spec.EquipmentSlots.Contains(TEXT("OffHand")));
    }

    bool BuildItem(const FGridWeaponPipelineRow& Spec,
        UGridWeaponCatalogueAsset* Rpg,
        UGridWeaponVisualCatalogueAsset* Visuals,
        UGridItemDefinitionAsset* Item)
    {
        const FGridWeaponLevelProfile* N1 =
            Rpg->FindProfile(FName(Spec.ArchetypeId), 1);
        const FGridWeaponVisualProfile* Look =
            Visuals->FindProfileForArchetype(FName(Spec.ArchetypeId));
        if (!N1 || !KindAgreesWithSource(Spec, *N1) ||
            !IsVisualSame(Spec, *Visuals) ||
            !Look || !Look->IsValidProfile() ||
            Look->BaseIcon.IsNull() || Look->WorldMesh.IsNull() ||
            !Look->BaseIcon.LoadSynchronous() || !Look->WorldMesh.LoadSynchronous())
        {
            return false;
        }

        Item->ItemDefinitionId = FName(Spec.ItemDefinitionId);
        Item->DisplayName = N1->DisplayName;
        Item->ItemType = Spec.IsPassive() ? EGridItemType::Shield : EGridItemType::Weapon;
        Item->Weight = N1->WeightKg;
        Item->bStackable = false;
        Item->MaxStackSize = 1;
        Item->HandUsage = Usage(Spec);
        Item->CompatibleEquipmentSlots.Reset();
        if (AllowsSlot(Spec, EGridEquipmentSlot::MainHand))
            Item->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
        if (AllowsSlot(Spec, EGridEquipmentSlot::OffHand))
            Item->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::OffHand);
        Item->WeaponArchetypeId = FName(Spec.ArchetypeId);
        Item->DefaultWeaponItemLevel = 1;
        Item->bUseWeaponCatalogue = true;
        Item->WeaponCatalogue = Rpg;
        Item->bUseWeaponVisualCatalogue = true;
        Item->WeaponVisualCatalogue = Visuals;
        Item->Icon = Look->BaseIcon;
        Item->WorldMesh = Look->WorldMesh;
        Item->EquippedMesh = Look->EquippedMesh.IsNull() ? Look->WorldMesh : Look->EquippedMesh;
        Item->bCombatThrowWeapon = false;
        Item->bProvidesQuickItemCombatAction = false;

        if (Spec.IsPassive())
        {
            Item->EquipmentStatBonus.ArmorBonus = N1->PhysicalArmor;
        }
        else
        {
            FGridAttackDefinition Attack;
            if (!N1->TryBuildAttackDefinition(Attack))
                return false;
            FGridCombatActionDefinition Action;
            Action.ActionId = FName(Spec.AttackId);
            Action.DisplayName = N1->DisplayName;
            Action.Icon = Look->BaseIcon;
            Action.ActionType = Spec.BuildKind == TEXT("Ranged")
                ? EGridCombatActionType::RangedAttack
                : EGridCombatActionType::MeleeAttack;
            Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
            Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
            Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
            Action.ActionPointCost = N1->ActionPointCost;
            Action.RangeCells = N1->RangeCells;
            Action.bRequiresLineOfSight = Spec.BuildKind == TEXT("Ranged");
            Action.OffensiveProfile.AttackId = Action.ActionId;
            Action.OffensiveProfile.AttackDefinition = Attack;
            Action.OffensiveProfile.DamageScalingAttribute = N1->ScalingAttribute;
            Action.OffensiveProfile.RangeCells = N1->RangeCells;
            if (!Action.IsValid()) return false;
            Item->CombatActions.Add(Action);
        }
        return Item->IsValidDefinition() &&
            Item->CanEquipToSlot(Spec.IsPassive()
                ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand) &&
            (Spec.IsPassive() ? Item->CombatActions.IsEmpty()
                : Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
    }

    bool ItemMatches(const FGridWeaponPipelineRow& Spec,
        const UGridItemDefinitionAsset& Existing,
        const UGridWeaponCatalogueAsset& Rpg,
        const UGridWeaponVisualCatalogueAsset& Visuals)
    {
        const FGridWeaponLevelProfile* N1 =
            Rpg.FindProfile(FName(Spec.ArchetypeId), 1);
        if (!N1) return false;
        if (Existing.GetPathName() != Spec.ItemPath ||
            Existing.ItemDefinitionId != FName(Spec.ItemDefinitionId) ||
            Existing.WeaponArchetypeId != FName(Spec.ArchetypeId) ||
            Existing.DefaultWeaponItemLevel != 1 || Existing.bStackable ||
            !Existing.IsValidDefinition() ||
            !Existing.bUseWeaponCatalogue || !Existing.bUseWeaponVisualCatalogue ||
            Existing.WeaponCatalogue.LoadSynchronous() != &Rpg ||
            Existing.WeaponVisualCatalogue.LoadSynchronous() != &Visuals ||
            !FMath::IsNearlyEqual(Existing.Weight, N1->WeightKg, 0.0001f) ||
            Existing.HandUsage != Usage(Spec) ||
            Existing.Icon.ToSoftObjectPath() != FSoftObjectPath(Spec.IconPath) ||
            Existing.WorldMesh.ToSoftObjectPath() != FSoftObjectPath(Spec.MeshPath) ||
            Existing.CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand) !=
                AllowsSlot(Spec, EGridEquipmentSlot::MainHand) ||
            Existing.CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::OffHand) !=
                AllowsSlot(Spec, EGridEquipmentSlot::OffHand))
        {
            return false;
        }
        if (Spec.IsPassive())
        {
            return Existing.ItemType == EGridItemType::Shield &&
                Existing.CombatActions.IsEmpty() &&
                FMath::IsNearlyEqual(Existing.EquipmentStatBonus.ArmorBonus,
                    N1->PhysicalArmor, 0.001f);
        }
        if (Existing.ItemType != EGridItemType::Weapon ||
            Existing.CombatActions.Num() != 1)
        {
            return false;
        }
        const FGridCombatActionDefinition& A = Existing.CombatActions[0];
        FGridAttackDefinition Expected;
        if (!N1->TryBuildAttackDefinition(Expected))
            return false;
        const FGridAttackDefinition& Actual = A.OffensiveProfile.AttackDefinition;
        return A.IsValid() && A.ActionId == FName(Spec.AttackId) &&
            A.ActionType == (Spec.BuildKind == TEXT("Ranged")
                ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack) &&
            A.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
            A.ActionPointCost == N1->ActionPointCost &&
            A.RangeCells == N1->RangeCells &&
            A.bRequiresLineOfSight == (Spec.BuildKind == TEXT("Ranged")) &&
            Actual.DamageType == Expected.DamageType &&
            Actual.PhysicalSubtype == Expected.PhysicalSubtype &&
            Actual.AccuracyBonus == Expected.AccuracyBonus &&
            Actual.MinDamage == Expected.MinDamage &&
            Actual.MaxDamage == Expected.MaxDamage &&
            Actual.DamageDice.bEnabled == Expected.DamageDice.bEnabled &&
            Actual.DamageDice.DiceCount == Expected.DamageDice.DiceCount &&
            Actual.DamageDice.DiceSides == Expected.DamageDice.DiceSides &&
            Actual.DamageDice.FlatBonus == Expected.DamageDice.FlatBonus &&
            A.OffensiveProfile.DamageScalingAttribute == N1->ScalingAttribute &&
            Existing.CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand);
    }

    FState Inspect(const FGridWeaponPipelineRow& Spec,
        UGridWeaponCatalogueAsset& Rpg,
        UGridWeaponVisualCatalogueAsset& Visuals,
        const TArray<FAssetData>& AllItems)
    {
        FState S;
        const FGridWeaponLevelProfile* N1 =
            Rpg.FindProfile(FName(Spec.ArchetypeId), 1);
        if (!N1 || !KindAgreesWithSource(Spec, *N1))
        {
            S.Summary = TEXT("CONFLICT");
            S.Detail = TEXT("CatalogueKindOrN1Mismatch");
            return S;
        }

        if (!Spec.ClassId.IsEmpty() && Spec.ProductionState == TEXT("Planned"))
        {
            const EGridWeaponKitAffinity Affinity =
                Rpg.GetKitRecommendationAffinity(FName(Spec.ArchetypeId), FName(Spec.ClassId));
            if (Affinity != EGridWeaponKitAffinity::Primary &&
                Affinity != EGridWeaponKitAffinity::Secondary)
            {
                S.Summary = TEXT("CONFLICT");
                S.Detail = TEXT("ProposedKitAffinityNotApproved");
                return S;
            }
        }

        FString ArtDetail;
        S.Art = LoadArt(Spec, ArtDetail) ? TEXT("READY") : TEXT("MISSING");
        const FName ExistingId = Visuals.FindVisualId(FName(Spec.ArchetypeId));
        if (!ExistingId.IsNone())
        {
            if (!IsVisualSame(Spec, Visuals))
            {
                S.Visual = TEXT("CONFLICT");
                S.Summary = TEXT("CONFLICT");
                S.Detail = TEXT("ExistingVisualDiffers");
                return S;
            }
            S.Visual = TEXT("PRESENT_SAME");
        }
        else if (Visuals.FindVisualProfile(FName(Spec.VisualId)))
        {
            S.Visual = TEXT("CONFLICT");
            S.Summary = TEXT("CONFLICT");
            S.Detail = TEXT("VisualIdAlreadyInUse");
            return S;
        }

        int32 ItemCount = 0;
        UGridItemDefinitionAsset* Found = nullptr;
        for (const FAssetData& Data : AllItems)
        {
            UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(Data.GetAsset());
            if (Item && (Item->WeaponArchetypeId == FName(Spec.ArchetypeId) ||
                Item->ItemDefinitionId == FName(Spec.ItemDefinitionId) ||
                Item->GetPathName() == Spec.ItemPath))
            {
                ++ItemCount;
                Found = Item;
            }
        }
        if (ItemCount > 1 || (Found && !ItemMatches(Spec, *Found, Rpg, Visuals)) ||
            (!Found && PackageFileExists(Spec.ItemPath)))
        {
            S.Item = TEXT("CONFLICT");
            S.Summary = TEXT("CONFLICT");
            S.Detail = TEXT("ExistingItemOrPathDiffers");
            return S;
        }
        if (Found)
        {
            S.Item = TEXT("PRESENT_SAME");
        }

        if (S.Art != TEXT("READY"))
        {
            S.Summary = TEXT("ART_MISSING");
            S.Detail = ArtDetail;
        }
        else if (S.Visual == TEXT("MISSING"))
        {
            S.Summary = TEXT("VISUAL_MISSING");
            S.Detail = TEXT("VisualCanBeRegistered");
        }
        else if (S.Item == TEXT("MISSING"))
        {
            S.Summary = TEXT("ITEM_MISSING");
            S.Detail = TEXT("ItemCanBeAuthored");
        }
        else
        {
            S.Summary = TEXT("COMPLETE");
            S.Detail = TEXT("ExistingDefinitionsChecked");
        }
        return S;
    }

    bool SaveVisual(const FGridWeaponPipelineRow& Spec,
        UGridWeaponVisualCatalogueAsset& Visuals)
    {
        if (Visuals.GetOutermost()->IsDirty())
            return false;
        TArray<FGridWeaponVisualArchetypeBinding> Bindings = Visuals.ArchetypeBindings;
        TArray<FGridWeaponVisualProfile> Profiles = Visuals.VisualProfiles;
        FGridWeaponVisualArchetypeBinding Binding;
        Binding.ArchetypeId = FName(Spec.ArchetypeId);
        Binding.VisualId = FName(Spec.VisualId);
        Bindings.Add(Binding);
        FGridWeaponVisualProfile Look;
        Look.VisualId = FName(Spec.VisualId);
        Look.FamilyTag = FName(Spec.VisualFamily);
        Look.BaseIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Spec.IconPath));
        Look.WorldMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Spec.MeshPath));
        Profiles.Add(Look);
        UGridWeaponVisualCatalogueAsset* Candidate =
            NewObject<UGridWeaponVisualCatalogueAsset>(GetTransientPackage());
        Candidate->ArchetypeBindings = Bindings;
        Candidate->VisualProfiles = Profiles;
        if (!Candidate->IsValidCatalogue())
            return false;

        Visuals.Modify();
        Visuals.ArchetypeBindings = MoveTemp(Bindings);
        Visuals.VisualProfiles = MoveTemp(Profiles);
        UPackage* P = Visuals.GetOutermost();
        P->MarkPackageDirty();
        const FString File = FPackageName::LongPackageNameToFilename(
            P->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(P, &Visuals, *File, Args);
    }

    bool SaveItem(const FGridWeaponPipelineRow& Spec,
        UGridWeaponCatalogueAsset& Rpg,
        UGridWeaponVisualCatalogueAsset& Visuals)
    {
        if (PackageFileExists(Spec.ItemPath))
            return false;
        UGridItemDefinitionAsset* Preview =
            NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
        if (!BuildItem(Spec, &Rpg, &Visuals, Preview))
            return false;

        const FString Name = PackageForObject(Spec.ItemPath);
        UPackage* P = CreatePackage(*Name);
        if (!IsValid(P))
            return false;
        const FString AssetName = FPackageName::GetLongPackageAssetName(Name);
        UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>(
            P, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
        if (!Item || !BuildItem(Spec, &Rpg, &Visuals, Item))
            return false;
        FAssetRegistryModule::AssetCreated(Item);
        Item->MarkPackageDirty();
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Name, FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(P, Item, *Filename, Args);
    }

    FString Csv(const FString& Value)
    {
        FString Escaped = Value.Replace(TEXT("\""), TEXT("\"\""));
        return TEXT("\"") + Escaped + TEXT("\"");
    }

    bool WriteReport(const FString& Report, const TArray<FString>& Lines)
    {
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Report), true);
        return FFileHelper::SaveStringArrayToFile(Lines, *Report);
    }
}

UGridWeaponPipelineCommandlet::UGridWeaponPipelineCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridWeaponPipelineCommandlet::Main(const FString& Params)
{
    using namespace WeaponPipeline;
    FString Action = TEXT("Audit"), Stage, Requested, ManifestFile, Report;
    FParse::Value(*Params, TEXT("Action="), Action);
    FParse::Value(*Params, TEXT("Stage="), Stage);
    FParse::Value(*Params, TEXT("Archetype="), Requested);
    FParse::Value(*Params, TEXT("Manifest="), ManifestFile);
    FParse::Value(*Params, TEXT("Report="), Report);
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));

    if (Action != TEXT("Audit") && Action != TEXT("Plan") &&
        Action != TEXT("Build") && Action != TEXT("Validate"))
    {
        UE_LOG(LogTemp, Error, TEXT("[INDUS02] Unknown Action"));
        return 1;
    }
    if ((bApply && (Action != TEXT("Build") ||
            (Stage != TEXT("Visual") && Stage != TEXT("Item")))) ||
        (Action == TEXT("Build") && Requested.IsEmpty()) ||
        ((Action != TEXT("Build")) && bApply))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[INDUS02] Build requires one Archetype; -Apply requires -Action=Build and -Stage=Visual|Item"));
        return 1;
    }
    if (ManifestFile.IsEmpty())
        ManifestFile = FGridWeaponPipelineManifest::DefaultFile();
    if (Report.IsEmpty())
        Report = FPaths::Combine(FPaths::ProjectSavedDir(),
            TEXT("ARMEMENT-INDUS/INDUS02_WeaponAudit.csv"));

    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!FGridWeaponPipelineManifest::Load(ManifestFile, Rows, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[INDUS02] Manifest refused: %s"), *Error);
        return 1;
    }
    if (!Requested.IsEmpty() && !FGridWeaponPipelineManifest::Find(Rows, Requested))
    {
        UE_LOG(LogTemp, Error, TEXT("[INDUS02] Unknown archetype %s"), *Requested);
        return 1;
    }
    UGridWeaponCatalogueAsset* Rpg = LoadObject<UGridWeaponCatalogueAsset>(nullptr, RpgPath);
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, VisualsPath);
    if (!IsValid(Rpg) || !IsValid(Visuals) ||
        !Rpg->IsValidCatalogue() || !Rpg->IsValidClassAffinityData() ||
        !Visuals->IsValidCatalogue())
    {
        UE_LOG(LogTemp, Error, TEXT("[INDUS02] Existing catalogues are invalid"));
        return 1;
    }
    const bool bRpgDirty = Rpg->GetOutermost()->IsDirty();
    const bool bVisualDirty = Visuals->GetOutermost()->IsDirty();
    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FString> ScanPaths = { FString(ItemFolder) };
    Registry.ScanPathsSynchronous(ScanPaths, true);
    TArray<FAssetData> AllItems;
    Registry.GetAssetsByPath(FName(ItemFolder), AllItems, true);

    TArray<FString> ReportLines =
    {
        TEXT("ArchetypeId,ProductionState,BuildKind,Art,Visual,Item,Status,Detail")
    };
    int32 Checked = 0, Complete = 0, Blocked = 0, Conflicts = 0, Planned = 0;
    int32 Changes = 0;

    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (!Requested.IsEmpty() && Requested != Row.ArchetypeId)
            continue;
        ++Checked;
        FState S = Inspect(Row, *Rpg, *Visuals, AllItems);
        if (S.Summary == TEXT("CONFLICT")) ++Conflicts;
        else if (S.Summary == TEXT("COMPLETE")) ++Complete;
        else if (Row.ProductionState == TEXT("Planned")) ++Planned;
        else ++Blocked;

        UE_LOG(LogTemp, Display,
            TEXT("[INDUS02] RESULT Archetype=%s Status=%s Art=%s Visual=%s Item=%s Detail=%s"),
            *Row.ArchetypeId, *S.Summary, *S.Art, *S.Visual, *S.Item, *S.Detail);
        TArray<FString> Columns = {
            Csv(Row.ArchetypeId), Csv(Row.ProductionState), Csv(Row.BuildKind),
            Csv(S.Art), Csv(S.Visual), Csv(S.Item), Csv(S.Summary), Csv(S.Detail)
        };
        ReportLines.Add(FString::Join(Columns, TEXT(",")));

        if (bApply)
        {
            if (S.Summary == TEXT("CONFLICT") || S.Art != TEXT("READY"))
            {
                UE_LOG(LogTemp, Error, TEXT("[INDUS02] APPLY refused for %s: %s"),
                    *Row.ArchetypeId, *S.Detail);
                WriteReport(Report, ReportLines);
                return 2;
            }
            if (Stage == TEXT("Visual"))
            {
                if (S.Visual == TEXT("PRESENT_SAME"))
                {
                    UE_LOG(LogTemp, Display,
                        TEXT("[INDUS02] PRESENT_SAME Visual for %s; no package saved"),
                        *Row.ArchetypeId);
                }
                else if (S.Visual == TEXT("MISSING") && SaveVisual(Row, *Visuals))
                {
                    ++Changes;
                    UE_LOG(LogTemp, Display,
                        TEXT("[INDUS02] SAVED Visual for %s; one catalogue package"),
                        *Row.ArchetypeId);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[INDUS02] Visual registration failed"));
                    WriteReport(Report, ReportLines);
                    return 2;
                }
            }
            else
            {
                if (S.Visual != TEXT("PRESENT_SAME"))
                {
                    UE_LOG(LogTemp, Error, TEXT("[INDUS02] Item requires registered visual"));
                    WriteReport(Report, ReportLines);
                    return 2;
                }
                if (S.Item == TEXT("PRESENT_SAME"))
                {
                    UE_LOG(LogTemp, Display,
                        TEXT("[INDUS02] PRESENT_SAME Item for %s; no package saved"),
                        *Row.ArchetypeId);
                }
                else if (S.Item == TEXT("MISSING") && SaveItem(Row, *Rpg, *Visuals))
                {
                    ++Changes;
                    UE_LOG(LogTemp, Display,
                        TEXT("[INDUS02] SAVED Item for %s; one new package"),
                        *Row.ArchetypeId);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[INDUS02] Item authoring failed"));
                    WriteReport(Report, ReportLines);
                    return 2;
                }
            }
        }
    }
    if (!WriteReport(Report, ReportLines))
    {
        UE_LOG(LogTemp, Error, TEXT("[INDUS02] Cannot save report CSV"));
        return 1;
    }
    if (!bApply && (Rpg->GetOutermost()->IsDirty() != bRpgDirty ||
                    Visuals->GetOutermost()->IsDirty() != bVisualDirty))
    {
        UE_LOG(LogTemp, Error, TEXT("[INDUS02] Read-only operation dirtied a catalogue"));
        return 1;
    }

    // Historical definitions may deliberately use custom names/handlers.
    // Make them visible in logs without inventing their ArchetypeId.
    if (Requested.IsEmpty())
    {
        int32 Unmanaged = 0;
        for (const FAssetData& Data : AllItems)
        {
            const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(Data.GetAsset());
            if (!Item || !Item->bUseWeaponCatalogue)
                continue;
            if (!FGridWeaponPipelineManifest::Find(Rows, Item->WeaponArchetypeId.ToString()))
            {
                ++Unmanaged;
                UE_LOG(LogTemp, Display,
                    TEXT("[INDUS02] LEGACY_UNMANAGED Asset=%s Archetype=%s"),
                    *Item->GetPathName(), *Item->WeaponArchetypeId.ToString());
            }
        }
        UE_LOG(LogTemp, Display, TEXT("[INDUS02] LEGACY_UNMANAGED count=%d"), Unmanaged);
    }

    UE_LOG(LogTemp, Display,
        TEXT("[INDUS02] SUMMARY Action=%s Checked=%d Complete=%d Blocked=%d Planned=%d Conflicts=%d SavedPackages=%d Report=%s"),
        *Action, Checked, Complete, Blocked, Planned, Conflicts, Changes, *Report);
    if ((Action == TEXT("Build") && bApply && Changes > 1) ||
        (Action == TEXT("Validate") && (Conflicts > 0 || Blocked > 0)) ||
        (Action == TEXT("Build") && Conflicts > 0))
    {
        return 2;
    }
    return 0;
}
