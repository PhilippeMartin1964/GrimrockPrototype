#include "ARMEMENT/GridArmClean064X81LockRulesCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Dom/JsonObject.h"
#include "EditorTools/GridEditorLockKeyMigrationPreview.h"
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

namespace ARMCLEAN064X81
{
    struct FTarget
    {
        const TCHAR* ObjectPath;
        FName DefinitionId;
        FName KeyId;
    };

    const FTarget Locks[] = {
        {
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.DA_Lock_CopperWall"),
            FName(TEXT("Lock_CopperWallLock")), FName(TEXT("Key_Copper"))
        },
        {
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.DA_Lock_IronWall"),
            FName(TEXT("Lock_IronWallLock")), FName(TEXT("Key_Iron"))
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
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        Package->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Definition, *Filename, Args);
    }

    TArray<FName> RuntimeAcceptedSet(const FGridLockBehaviorParams& Lock)
    {
        TArray<FName> Keys;
        for (const TObjectPtr<UGridItemDefinitionAsset>& Ptr : Lock.AcceptedKeyItems)
        {
            if (IsValid(Ptr.Get()) && !Ptr->ItemDefinitionId.IsNone())
                Keys.AddUnique(Ptr->ItemDefinitionId);
        }
        for (FName Id : Lock.AcceptedKeyIds)
        {
            if (!Id.IsNone()) Keys.AddUnique(Id);
        }
        return Keys;
    }
}

UGridArmClean064X81LockRulesCommandlet::UGridArmClean064X81LockRulesCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X81LockRulesCommandlet::Main(const FString& Params)
{
    const bool bAudit = FParse::Param(*Params, TEXT("Audit"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString Output;
    if (static_cast<int32>(bAudit) + static_cast<int32>(bApply) +
        static_cast<int32>(bVerify) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || Output.IsEmpty())
        return 1;

    TStrongObjectPtr<UGridItemSystemAsset> ItemSystem(
        LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!IsValid(ItemSystem.Get()) || ItemSystem->ItemDefinitions.Num() != 34) return 2;
    const bool bItemSystemDirty = ItemSystem->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> RegistryErrors;
    if (!IsValid(Registry.Get()) || !Registry->RegisterItemSystem(ItemSystem.Get(), RegistryErrors))
        return 2;

    int32 LevelCount = 0, ReceptacleOverrides = 0, LockOverrides = 0;
    for (const TCHAR* Path : ARMCLEAN064X81::Levels)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!IsValid(Level.Get())) return 3;
        ++LevelCount;
        const bool WasDirty = Level->GetOutermost()->IsDirty();
        for (const FGridWorldObjectInstance& Instance : Level->WorldObjectInstances)
        {
            ReceptacleOverrides +=
                Instance.InstanceConfig.InteractionOverrides.bOverrideReceptacleRules ? 1 : 0;
            LockOverrides +=
                Instance.InstanceConfig.InteractionOverrides.bOverrideAcceptedKeys ? 1 : 0;
        }
        if (Level->GetOutermost()->IsDirty() != WasDirty) return 3;
    }

    FAssetRegistryModule& AssetRegistry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> AssetData;
    AssetRegistry.Get().GetAssets(Filter, AssetData);

    int32 Definitions = 0, ReceptacleLists = 0, ReceptacleRules = 0;
    int32 ReceptacleCanonical = 0, LockLists = 0, KeyPointers = 0;
    int32 ExplicitKeyIds = 0, AlreadyCanonical = 0;
    TArray<UGridWorldObjectDefinitionAsset*> Targets;
    for (const FAssetData& Data : AssetData)
    {
        UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(Data.GetAsset());
        if (!IsValid(Definition)) return 4;
        ++Definitions;
        const bool WasDirty = Definition->GetOutermost()->IsDirty();
        const FGridReceptacleBehaviorParams& Receptacle =
            Definition->DefaultBehavior.Receptacle;
        if (Definition->SupportedType == EGridLevelObjectType::Receptacle ||
            !Receptacle.AcceptedItems.IsEmpty())
        {
            ++ReceptacleLists;
            ReceptacleRules += Receptacle.AcceptedItems.Num();
            for (const FGridReceptacleAcceptedItemConfig& Rule : Receptacle.AcceptedItems)
            {
                if (!IsValid(Rule.ItemDefinition.Get()) ||
                    Rule.CanonicalItemId != Rule.ItemDefinition->ItemDefinitionId ||
                    !Registry->HasCanonicalItem(Rule.CanonicalItemId)) return 4;
                ++ReceptacleCanonical;
            }
        }

        const FGridLockBehaviorParams& Lock = Definition->DefaultBehavior.Lock;
        if (!Lock.AcceptedKeyItems.IsEmpty() || !Lock.AcceptedKeyIds.IsEmpty())
        {
            ++LockLists;
            KeyPointers += Lock.AcceptedKeyItems.Num();
            ExplicitKeyIds += Lock.AcceptedKeyIds.Num();
            TArray<FName> Proposed;
            FGridLockKeyPreviewStats Stats;
            if (!FGridLockKeyMigrationPreview::TryPreview(
                Registry.Get(), Lock.AcceptedKeyItems, Lock.AcceptedKeyIds,
                Proposed, Stats)) return 4;
            const TArray<FName> Before = ARMCLEAN064X81::RuntimeAcceptedSet(Lock);
            if (Before.Num() != 1 || Proposed.Num() != 1 ||
                !Proposed.Contains(Before[0])) return 4;
        }

        for (const ARMCLEAN064X81::FTarget& Target : ARMCLEAN064X81::Locks)
        {
            if (Definition->GetPathName() != Target.ObjectPath) continue;
            if (Definition->DefinitionId != Target.DefinitionId ||
                Lock.AcceptedKeyItems.Num() != 1 ||
                !IsValid(Lock.AcceptedKeyItems[0].Get()) ||
                Lock.AcceptedKeyItems[0]->ItemDefinitionId != Target.KeyId ||
                !Registry->HasCanonicalItem(Target.KeyId) ||
                (Lock.AcceptedKeyIds.Num() != 0 &&
                    !(Lock.AcceptedKeyIds.Num() == 1 &&
                      Lock.AcceptedKeyIds[0] == Target.KeyId)))
                return 5;
            const TArray<FName> Before = ARMCLEAN064X81::RuntimeAcceptedSet(Lock);
            if (Before.Num() != 1 || Before[0] != Target.KeyId) return 5;
            if (Lock.AcceptedKeyIds.Num() == 1) ++AlreadyCanonical;
            Targets.Add(Definition);
        }
        if (Definition->GetOutermost()->IsDirty() != WasDirty) return 4;
    }

    if (LevelCount != 4 || Definitions != 44 ||
        ReceptacleOverrides != 0 || LockOverrides != 0 ||
        ReceptacleLists != 6 || ReceptacleRules != 2 ||
        ReceptacleCanonical != 2 || LockLists != 2 ||
        KeyPointers != 2 || Targets.Num() != 2 ||
        Targets[0] == Targets[1] ||
        (ExplicitKeyIds != 0 && ExplicitKeyIds != 2) ||
        (AlreadyCanonical != 0 && AlreadyCanonical != 2) ||
        (ExplicitKeyIds != AlreadyCanonical) ||
        (bApply && AlreadyCanonical != 0) ||
        (bVerify && AlreadyCanonical != 2) ||
        ItemSystem->GetOutermost()->IsDirty() != bItemSystemDirty)
        return 6;

    int32 Saved = 0;
    if (bApply)
    {
        for (UGridWorldObjectDefinitionAsset* Definition : Targets)
        {
            FGridLockBehaviorParams& Lock = Definition->DefaultBehavior.Lock;
            const FName Key = Lock.AcceptedKeyItems[0]->ItemDefinitionId;
            Lock.AcceptedKeyIds.Add(Key); // Keep legacy pointer, order and flags.
            if (!ARMCLEAN064X81::SaveNative(Definition)) return 7;
            ++Saved;
        }
    }

    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X8/real-lock-key-acceptance/v1"));
    Report->SetStringField(TEXT("mode"),
        bAudit ? TEXT("Audit") : (bApply ? TEXT("Apply") : TEXT("Verify")));
    Report->SetNumberField(TEXT("definitions"), Definitions);
    Report->SetNumberField(TEXT("levels"), LevelCount);
    Report->SetNumberField(TEXT("receptacle_lists"), ReceptacleLists);
    Report->SetNumberField(TEXT("receptacle_rules"), ReceptacleRules);
    Report->SetNumberField(TEXT("receptacle_canonical"), ReceptacleCanonical);
    Report->SetNumberField(TEXT("lock_lists"), LockLists);
    Report->SetNumberField(TEXT("lock_pointers"), KeyPointers);
    Report->SetNumberField(TEXT("lock_explicit_ids"), ExplicitKeyIds);
    Report->SetNumberField(TEXT("overrides"), ReceptacleOverrides + LockOverrides);
    Report->SetNumberField(TEXT("targets"), Targets.Num());
    Report->SetNumberField(TEXT("already_canonical"), AlreadyCanonical);
    Report->SetNumberField(TEXT("eligible_legacy"), 2 - AlreadyCanonical);
    Report->SetNumberField(TEXT("packages_saved"), Saved);
    Report->SetBoolField(TEXT("legacy_pointers_retained"), true);
    Report->SetBoolField(TEXT("savegames_opened_or_written"), false);
    Report->SetBoolField(TEXT("accepted_key_set_unchanged"), true);

    FString Json;
    if (!FJsonSerializer::Serialize(Report,
        TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *Output,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        return 8;
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4X8] %s 2 lock keys, %d packages saved, 44 definitions, 4 levels."),
        bAudit ? TEXT("audited") : (bApply ? TEXT("applied") : TEXT("verified")),
        Saved);
    return 0;
}
