#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064WRealReferences,
    "Grimrock.RPG.ARM.CLEAN06_4.NativeWorldReferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064WRealReferences::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real canonical registry loaded"), Root.Get()) ||
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
    {
        return false;
    }
    TSet<FName> ValidIds;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!TestNotNull(TEXT("Concrete item definition"), Definition))
        {
            return false;
        }
        ValidIds.Add(Definition->ItemId);
    }
    if (!TestEqual(TEXT("All real identities unique"), ValidIds.Num(), Root->ItemDefinitions.Num()))
    {
        return false;
    }
    int32 Total = 0;
    int32 Materialized = 0;
    int32 Loose = 0;
    int32 Spawn = 0;
    int32 Receptacle = 0;
    int32 Palette = 0;
    auto Visit = [&](const UGridItemDefinitionAsset* Legacy, FName ItemId,
        bool bAllowPointerless) -> bool
    {
        // X.58: loose, palette and initial-content rows may be pointerless.
        if (!IsValid(Legacy) && bAllowPointerless)
        {
            if (ItemId.IsNone() || !ValidIds.Contains(ItemId)) { return false; }
            ++Total;
            ++Materialized;
            return true;
        }
        if (!IsValid(Legacy) || Legacy->ItemDefinitionId.IsNone() ||
            !ValidIds.Contains(Legacy->ItemDefinitionId) ||
            (!ItemId.IsNone() && ItemId != Legacy->ItemDefinitionId))
        {
            return false;
        }
        ++Total;
        if (!ItemId.IsNone()) { ++Materialized; }
        return true;
    };

    const TCHAR* LevelPaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };
    for (const TCHAR* Path : LevelPaths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("Historical level exists"), Level.Get()))
        {
            return false;
        }
        for (const FGridLooseItemInstance& Item : Level->LooseItemInstances)
        {
            if (!TestTrue(TEXT("Loose item maintains exact source ItemId"),
                Visit(nullptr, Item.ItemId, true))) { return false; }
            ++Loose;
        }
        for (const FGridItemSpawnInstance& Item : Level->ItemSpawns)
        {
            if (!TestTrue(TEXT("Spawn maintains exact source ItemId"),
                Visit(nullptr, Item.ItemId, true))) { return false; }
            ++Spawn;
        }
        for (const FGridWorldObjectInstance& World : Level->WorldObjectInstances)
        {
            for (const FGridReceptacleInitialItemConfig& Item :
                World.InstanceConfig.ReceptacleInitialContent)
            {
                if (!TestTrue(TEXT("Receptacle maintains exact source ItemId"),
                    Visit(nullptr, Item.ItemId, true))) { return false; }
                ++Receptacle;
            }
        }
    }
    TStrongObjectPtr<UGridObjectPaletteAsset> P(
        LoadObject<UGridObjectPaletteAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default."
                 "DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Real palette loaded"), P.Get())) { return false; }
    for (const FGridObjectPaletteEntry& Entry : P->Entries)
    {
        if (!Entry.IsDirectItemEntry()) { continue; }
        if (!TestTrue(TEXT("Palette item retains exact source ItemId"),
                Visit(nullptr, Entry.ItemId, true)))
        {
            return false;
        }
        ++Palette;
    }
    return TestEqual(TEXT("Real level loose placements"), Loose, 44) &&
        TestEqual(TEXT("Real item spawns"), Spawn, 0) &&
        TestEqual(TEXT("Real initial receptacle entries"), Receptacle, 18) &&
        TestEqual(TEXT("Real direct item palette entries"), Palette, 33) &&
        TestEqual(TEXT("All original world references"), Total, 95) &&
        TestTrue(TEXT("One-shot conversion is all-or-nothing"),
            Materialized == 0 || Materialized == 95);
}
#endif
