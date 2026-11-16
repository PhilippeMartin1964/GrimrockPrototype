#include "ARMEMENT/GridArmClean064X71ReceptacleRulesCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Dom/JsonObject.h"
#include "EditorTools/GridEditorReceptacleAcceptanceMigrationPreview.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/GameInstance.h"

namespace ARMCLEAN064X71
{
    struct FTarget
    {
        const TCHAR* ObjectPath;
        FName DefinitionId;
        FName ExpectedItemId;
    };

    const FTarget Receptacles[] = {
        {
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.DA_Receptacle_TorchHolder"),
            FName(TEXT("Receptacle_TorchHolder")), FName(TEXT("Item_Torch"))
        },
        {
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian.DA_Guardian"),
            FName(TEXT("Guardian")), FName(TEXT("Gem_Blue"))
        }
    };

    const TCHAR* Levels[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };

    bool SaveNative(UGridWorldObjectDefinitionAsset* Definition)
    {
        UPackage* Package = IsValid(Definition) ? Definition->GetOutermost() : nullptr;
        if (!IsValid(Package)) return false;
        const FString File = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        Package->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Definition, *File, Args);
    }
}

UGridArmClean064X71ReceptacleRulesCommandlet::
    UGridArmClean064X71ReceptacleRulesCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X71ReceptacleRulesCommandlet::Main(const FString& Params)
{
    const bool bAudit = FParse::Param(*Params, TEXT("Audit"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString Output;
    if (static_cast<int32>(bAudit) + static_cast<int32>(bApply) +
            static_cast<int32>(bVerify) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || Output.IsEmpty())
    {
        return 1;
    }

    TStrongObjectPtr<UGridItemSystemAsset> ItemSystem(
        LoadObject<UGridItemSystemAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!IsValid(ItemSystem.Get()) || ItemSystem->ItemDefinitions.Num() != 34) return 2;
    const bool bItemSystemDirty = ItemSystem->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Resolver(
        NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> RegistryErrors;
    if (!IsValid(Resolver.Get()) ||
        !Resolver->RegisterItemSystem(ItemSystem.Get(), RegistryErrors)) return 2;

    int32 OverrideReceptacles = 0, OverrideLocks = 0, LoadedLevels = 0;
    for (const TCHAR* Path : ARMCLEAN064X71::Levels)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(
            LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!IsValid(Level.Get())) return 3;
        ++LoadedLevels;
        const bool bDirty = Level->GetOutermost()->IsDirty();
        for (const FGridWorldObjectInstance& Instance : Level->WorldObjectInstances)
        {
            OverrideReceptacles +=
                Instance.InstanceConfig.InteractionOverrides.bOverrideReceptacleRules ? 1 : 0;
            OverrideLocks +=
                Instance.InstanceConfig.InteractionOverrides.bOverrideAcceptedKeys ? 1 : 0;
        }
        if (Level->GetOutermost()->IsDirty() != bDirty) return 3;
    }

    FAssetRegistryModule& Module =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;
    TArray<FAssetData> Assets;
    Module.Get().GetAssets(Filter, Assets);

    int32 DefinitionCount = 0, ReceptacleLists = 0, Rules = 0;
    int32 KeyLists = 0, KeyPointers = 0, ExplicitKeys = 0;
    int32 AlreadyCanonical = 0;
    TArray<UGridWorldObjectDefinitionAsset*> Targets;
    for (const FAssetData& Data : Assets)
    {
        UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(Data.GetAsset());
        if (!IsValid(Definition)) return 4;
        ++DefinitionCount;
        const bool bDirty = Definition->GetOutermost()->IsDirty();
        const FGridReceptacleBehaviorParams& R = Definition->DefaultBehavior.Receptacle;
        if (Definition->SupportedType == EGridLevelObjectType::Receptacle ||
            !R.AcceptedItems.IsEmpty())
        {
            ++ReceptacleLists;
            TArray<FGridReceptacleAcceptedItemConfig> Proposed;
            FGridReceptacleAcceptancePlanStats Stats;
            if (!FGridReceptacleAcceptanceMigrationPreview::TryPreview(
                Resolver.Get(), R.AcceptedItems, Proposed, Stats)) return 4;
            Rules += R.AcceptedItems.Num();
        }
        const FGridLockBehaviorParams& Lock = Definition->DefaultBehavior.Lock;
        if (!Lock.AcceptedKeyItems.IsEmpty() || !Lock.AcceptedKeyIds.IsEmpty())
        {
            ++KeyLists;
            KeyPointers += Lock.AcceptedKeyItems.Num();
            ExplicitKeys += Lock.AcceptedKeyIds.Num();
        }
        for (const ARMCLEAN064X71::FTarget& Target : ARMCLEAN064X71::Receptacles)
        {
            if (Definition->GetPathName() != Target.ObjectPath) continue;
            if (Definition->DefinitionId != Target.DefinitionId ||
                R.AcceptedItems.Num() != 1 ||
                !IsValid(R.AcceptedItems[0].ItemDefinition.Get()) ||
                R.AcceptedItems[0].ItemDefinition->ItemDefinitionId != Target.ExpectedItemId ||
                !Resolver->HasCanonicalItem(Target.ExpectedItemId) ||
                (bVerify
                    ? R.AcceptedItems[0].CanonicalItemId != Target.ExpectedItemId
                    : (bApply && !R.AcceptedItems[0].CanonicalItemId.IsNone())))
            {
                return 5;
            }
            AlreadyCanonical +=
                (R.AcceptedItems[0].CanonicalItemId == Target.ExpectedItemId) ? 1 : 0;
            Targets.Add(Definition);
        }
        if (Definition->GetOutermost()->IsDirty() != bDirty) return 4;
    }
    if (LoadedLevels != 4 || DefinitionCount != 44 || ReceptacleLists != 6 ||
        Rules != 2 || KeyLists != 2 || KeyPointers != 2 ||
        ExplicitKeys != 0 ||
        OverrideReceptacles != 0 || OverrideLocks != 0 ||
        Targets.Num() != 2 || Targets[0] == Targets[1] ||
        (bApply && AlreadyCanonical != 0) ||
        (bVerify && AlreadyCanonical != 2) ||
        ItemSystem->GetOutermost()->IsDirty() != bItemSystemDirty)
    {
        return 6;
    }

    int32 Saved = 0;
    if (bApply)
    {
        for (UGridWorldObjectDefinitionAsset* Target : Targets)
        {
            FGridReceptacleAcceptedItemConfig& Rule =
                Target->DefaultBehavior.Receptacle.AcceptedItems[0];
            Rule.CanonicalItemId = Rule.ItemDefinition->ItemDefinitionId;
            if (!ARMCLEAN064X71::SaveNative(Target)) return 7;
            ++Saved;
        }
    }

    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"),
        TEXT("ARM-CLEAN06.4X7/real-receptacle-acceptance/v1"));
    Report->SetStringField(TEXT("mode"),
        bAudit ? TEXT("Audit") : (bApply ? TEXT("Apply") : TEXT("Verify")));
    Report->SetNumberField(TEXT("definitions"), DefinitionCount);
    Report->SetNumberField(TEXT("levels"), LoadedLevels);
    Report->SetNumberField(TEXT("receptacle_lists"), ReceptacleLists);
    Report->SetNumberField(TEXT("receptacle_rules"), Rules);
    Report->SetNumberField(TEXT("lock_lists"), KeyLists);
    Report->SetNumberField(TEXT("lock_pointers"), KeyPointers);
    Report->SetNumberField(TEXT("lock_explicit_ids"), ExplicitKeys);
    Report->SetNumberField(TEXT("overrides"), OverrideReceptacles + OverrideLocks);
    Report->SetNumberField(TEXT("targets"), Targets.Num());
    Report->SetNumberField(TEXT("already_canonical"), AlreadyCanonical);
    Report->SetNumberField(TEXT("eligible_legacy"), 2 - AlreadyCanonical);
    Report->SetNumberField(TEXT("packages_saved"), Saved);
    Report->SetBoolField(TEXT("legacy_pointers_retained"), true);
    Report->SetBoolField(TEXT("savegames_opened_or_written"), false);

    FString Json;
    if (!FJsonSerializer::Serialize(Report,
        TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *Output,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return 8;

    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4X7] %s exactly two native receptacle rules, %d packages saved, 44 definitions, 4 levels."),
        bAudit ? TEXT("audited") : (bApply ? TEXT("applied") : TEXT("verified")),
        Saved);
    return 0;
}
