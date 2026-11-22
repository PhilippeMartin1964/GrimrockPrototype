#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "Engine/GameInstance.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "../Runtime/GridReceptacleInitialReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X91RealReceptacleSources,
    "Grimrock.RPG.ARM.CLEAN06_4X91.RealInitialReceptacleIdentities",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X91RealReceptacleSources::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real native registry"), Root.Get()) ||
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
    {
        return false;
    }
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(
        NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Single canonical registry"),
        Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) {AddError(Error);}
        return false;
    }

    const TCHAR* Paths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };

    int32 ReceptacleCount = 0;
    int32 LevelIndex = 0;
    int32 ArmCount = 0;
    int32 NonArmCount = 0;
    for (const TCHAR* Path : Paths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(
            LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("One native level"), Level.Get()))
        {
            return false;
        }
        const bool bInitiallyDirty = Level->GetOutermost()->IsDirty();
        const int32 Before = ReceptacleCount;
        for (const FGridWorldObjectInstance& World : Level->WorldObjectInstances)
        {
            for (const FGridReceptacleInitialItemConfig& Config :
                World.InstanceConfig.ReceptacleInitialContent)
            {
                ++ReceptacleCount;
                FGridInitialReceptacleItem Initial;
                if (!TestTrue(TEXT("Persisted initial identity resolves exactly"),
                    FGridReceptacleInitialReadModel::TryResolve(
                        Registry.Get(), Config, Initial)) ||
                    !TestTrue(TEXT("Old pointer is not the identity authority"),
                        Initial.ItemDefinitionId == Config.CanonicalItemId) ||
                    !TestEqual(TEXT("Physical level is preserved"),
                        Initial.ItemLevel, Config.ItemLevel) ||
                    !TestEqual(TEXT("Quantity is preserved"),
                        Initial.Quantity, Config.Quantity))
                {
                    return false;
                }
                FGridResolvedItem Resolved;
                if (!TestTrue(TEXT("Real family and physical level resolve"),
                    Registry->ResolveItem(Initial.ItemDefinitionId,
                        Initial.ItemLevel, Resolved)))
                {
                    return false;
                }
                if (Resolved.bIsArmament) {++ArmCount;}
                else {++NonArmCount;}
                FGridInitialReceptacleItem Unused;
                if (!TestFalse(TEXT("Canonical entry without real registry fails closed"),
                    FGridReceptacleInitialReadModel::TryResolve(
                        nullptr, Config, Unused)))
                {
                    return false;
                }
            }
        }
        const int32 Expected = LevelIndex == 0 ? 17 : LevelIndex == 1 ? 1 : 0;
        ++LevelIndex;
        if (!TestEqual(TEXT("Original per-level distribution"), ReceptacleCount - Before, Expected))
            return false;
        if (!TestEqual(TEXT("Read-only model never dirties native level"),
            Level->GetOutermost()->IsDirty(), bInitiallyDirty))
        {
            return false;
        }
    }
    if (!TestEqual(TEXT("All 18 materialized initial receptacle contents"),
        ReceptacleCount, 18) ||
        !TestEqual(TEXT("Exact canonical families"), ArmCount + NonArmCount, 18))
    {
        return false;
    }

    TestNull(TEXT("Initial content historical schema removed"), FGridReceptacleInitialItemConfig::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
    TestNull(TEXT("Runtime initial pointer removed"), FGridInitialReceptacleItem::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
    TestNull(TEXT("Runtime transition provenance removed"), FGridInitialReceptacleItem::StaticStruct()->FindPropertyByName(TEXT("bHasPersistedCanonicalIdentity")));
    FGridInitialReceptacleItem Out;
    FGridReceptacleInitialItemConfig Invalid;
    Invalid.CanonicalItemId = TEXT("Weapon_StraightSword_01");
    Invalid.ItemLevel = 0;
    Invalid.Quantity = 1;
    if (!TestFalse(TEXT("Persisted ARM N0 cannot inherit an arbitrary N1"),
        FGridReceptacleInitialReadModel::TryResolve(
            Registry.Get(), Invalid, Out)))
    {
        return false;
    }
    Invalid.CanonicalItemId = TEXT("Item_Arrows_01");
    Invalid.ItemLevel = 1;
    if (!TestFalse(TEXT("Canonical ammunition cannot become N1"),
        FGridReceptacleInitialReadModel::TryResolve(
            Registry.Get(), Invalid, Out)))
    {
        return false;
    }
    Invalid.CanonicalItemId = TEXT("ARM_CLEAN064X91_UNKNOWN");
    Invalid.ItemLevel = 0;
    if (!TestFalse(TEXT("Unknown canonical identity cannot fall back"),
        FGridReceptacleInitialReadModel::TryResolve(
            Registry.Get(), Invalid, Out)))
    {
        return false;
    }

    Invalid.CanonicalItemId = NAME_None;
    Invalid.Quantity = 1;
    if (!TestFalse(TEXT("Empty persisted identity fails closed"),
            FGridReceptacleInitialReadModel::TryResolve(Registry.Get(), Invalid, Out)))
        return false;
    Invalid.CanonicalItemId = TEXT("Item_Arrows_01");
    Invalid.Quantity = 0;
    if (!TestFalse(TEXT("Zero quantity cannot be clamped into an item"),
            FGridReceptacleInitialReadModel::TryResolve(Registry.Get(), Invalid, Out)) ||
        !TestTrue(TEXT("No physical identity on rejection"), Out.ItemDefinitionId.IsNone()))
        return false;
    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X9.1] 18 real receptacle initial IDs: "
        "%d ARM / %d nonARM; strict physical levels; "
        "accepted rules and logical links remain untouched."),
        ArmCount, NonArmCount));
    return true;
}
#endif
