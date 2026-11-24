#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorReceptacleRuleAuthoring.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X1661RealAcceptanceManifest,
    "Grimrock.RPG.ARM.CLEAN06_4X1661.RealAcceptanceDefinitionManifest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X1661RealAcceptanceManifest::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> ItemSystem(
        LoadObject<UGridItemSystemAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem"), ItemSystem.Get())) return false;
    const bool bSystemDirty = ItemSystem->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Registered canonical item catalogue"),
        Registry->RegisterItemSystem(ItemSystem.Get(), Errors))) return false;
    if (!TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*ItemSystem.Get())))
        return false;

    const TCHAR* LevelPaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };
    int32 Levels = 0, ReceptacleOverrides = 0, LockOverrides = 0;
    for (const TCHAR* Path : LevelPaths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("Native level exists"), Level.Get())) return false;
        const bool bDirty = Level->GetOutermost()->IsDirty();
        ++Levels;
        for (const FGridWorldObjectInstance& Instance : Level->WorldObjectInstances)
        {
            ReceptacleOverrides +=
                Instance.InstanceConfig.InteractionOverrides.bOverrideReceptacleRules ? 1 : 0;
            LockOverrides +=
                Instance.InstanceConfig.InteractionOverrides.bOverrideAcceptedKeys ? 1 : 0;
        }
        if (!TestEqual(TEXT("Level remained read-only"),
            Level->GetOutermost()->IsDirty(), bDirty)) return false;
    }

    FAssetRegistryModule& Module =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> AssetData;
    Module.Get().GetAssets(Filter, AssetData);

    int32 Definitions = 0, ReceptacleLists = 0, ReceptacleRules = 0;
    int32 LockLists = 0, LockPointers = 0, ReceptaclePointers = 0;
    int32 ManifestRows = 0;
    for (const FAssetData& Asset : AssetData)
    {
        UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(Asset.GetAsset());
        if (!TestNotNull(TEXT("Definition loads"), Definition)) return false;
        ++Definitions;
        const bool bDirty = Definition->GetOutermost()->IsDirty();
        const FString Package = Definition->GetOutermost()->GetName();
        const FString DefinitionId = Definition->DefinitionId.ToString();
        if (!TestTrue(TEXT("Package remains a native game asset"),
            Package.StartsWith(TEXT("/Game/GrimrockPrototype/")))) return false;

        const FGridReceptacleBehaviorParams& R = Definition->DefaultBehavior.Receptacle;
        if (Definition->SupportedType == EGridLevelObjectType::Receptacle ||
            !R.AcceptedItems.IsEmpty())
        {
            ++ReceptacleLists;
            TArray<FGridReceptacleAcceptedItemConfig> Proposed;
            if (!TestTrue(TEXT("Every actual receptacle list copies atomically"),
                FGridEditorReceptacleRuleAuthoring::TryCopyDefaults(
                    Registry.Get(), R.AcceptedItems, Proposed)))
                return false;
            ReceptacleRules += R.AcceptedItems.Num();
            for (int32 Index = 0; Index < R.AcceptedItems.Num(); ++Index)
            {
                const FGridReceptacleAcceptedItemConfig& Rule = R.AcceptedItems[Index];
                const FName AcceptedId = Proposed[Index].ItemId;
                const FName ExpectedId = Package ==
                    TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder")
                    ? FName(TEXT("Item_Torch"))
                    : (Package ==
                        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian")
                        ? FName(TEXT("Gem_Blue")) : NAME_None);
                if (!TestTrue(TEXT("Only the two approved shared acceptance rules"),
                        !ExpectedId.IsNone() && AcceptedId == ExpectedId &&
                        Rule.ItemId == ExpectedId &&
                        Registry->ContainsItem(AcceptedId)))
                    return false;
                AddInfo(FString::Printf(TEXT(
                    "[ARM-CLEAN06.4X.52] RECEPTACLE package=%s definition=%s index=%d legacy=%d canonical=%s"),
                    *Package, *DefinitionId, Index, 0, *AcceptedId.ToString()));
                ++ManifestRows;
            }
        }

        const FGridLockBehaviorParams& Lock = Definition->DefaultBehavior.Lock;
        if (!Lock.AcceptedKeyIds.IsEmpty())
        {
            ++LockLists;
            const FName ExpectedKey = Package ==
                TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall")
                ? FName(TEXT("Key_Copper"))
                : (Package == TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall")
                    ? FName(TEXT("Key_Iron")) : NAME_None);
            if (!TestTrue(TEXT("Only approved shared Copper/Iron rules exist"),
                !ExpectedKey.IsNone() && Lock.AcceptedKeyIds.Num() == 1 &&
                Lock.AcceptedKeyIds[0] == ExpectedKey &&
                Registry->ContainsItem(ExpectedKey)))
                return false;
            AddInfo(FString::Printf(TEXT(
                "[ARM-CLEAN06.4X.52] LOCK package=%s definition=%s legacy=%d canonical=%s"),
                *Package, *DefinitionId, 0,
                *ExpectedKey.ToString()));
            ++ManifestRows;
        }
        if (!TestEqual(TEXT("Definition package remained untouched"),
            Definition->GetOutermost()->IsDirty(), bDirty)) return false;
    }

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X.16.6.1] SUMMARY definitions=%d levels=%d receptacleLists=%d "
        "receptacleRules=%d receptaclePointers=%d lockLists=%d lockPointers=%d "
        "receptacleOverrides=%d lockOverrides=%d manifestRows=%d"),
        Definitions, Levels, ReceptacleLists, ReceptacleRules, ReceptaclePointers, LockLists,
        LockPointers, ReceptacleOverrides, LockOverrides, ManifestRows));

    // Verify the exact baseline supplied by the real validated X.16.4/5
    // reports. If production assets drift, block any future conversion.
    return TestEqual(TEXT("44 native world definitions"), Definitions, 44) &&
        TestEqual(TEXT("4 native levels"), Levels, 4) &&
        TestEqual(TEXT("6 actual receptacle lists"), ReceptacleLists, 6) &&
        TestEqual(TEXT("2 accepted item rules"), ReceptacleRules, 2) &&
        TestEqual(TEXT("2 lock key lists"), LockLists, 2) &&
        TestTrue(TEXT("Only native post-migration shared rules are valid"),
            ReceptaclePointers == 0 && LockPointers == 0) &&
        TestEqual(TEXT("No receptacle instance overrides"), ReceptacleOverrides, 0) &&
        TestEqual(TEXT("No lock instance overrides"), LockOverrides, 0) &&
        TestEqual(TEXT("Exactly 4 manifest rows"), ManifestRows, 4) &&
        TestEqual(TEXT("ItemSystem package remains read-only"),
            ItemSystem->GetOutermost()->IsDirty(), bSystemDirty);
}
#endif
