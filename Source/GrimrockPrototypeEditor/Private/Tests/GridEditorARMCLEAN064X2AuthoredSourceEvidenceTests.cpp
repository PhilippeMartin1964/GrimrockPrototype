#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

/**
 * ARM-CLEAN06.4X.2 is a strictly read-only gate. It proves which authored
 * ARM N0 values need source-owned default-level materialization before any
 * native pointers can be removed, and inventories the separate receptacle
 * acceptance rules that were NOT among 06.4W's 95 physical references.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridArmClean064X2AuthoredSourceEvidence,
    "Grimrock.RPG.ARM.CLEAN06_4X2.AuthoredSourceEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridArmClean064X2AuthoredSourceEvidence::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Native DA_ItemSystem"), Root.Get()) ||
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
    {
        return false;
    }

    TMap<FName, const UGridCanonicalItemDefinitionAsset*> Registry;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Registry.Contains(Definition->ItemId))
        {
            AddError(TEXT("Invalid or duplicate item in native registry."));
            return false;
        }
        Registry.Add(Definition->ItemId, Definition);
    }

    int32 Total = 0;
    int32 ArmReferences = 0;
    int32 InheritedArmN0 = 0;
    int32 NonArmN0 = 0;
    int32 Loose = 0;
    int32 Spawns = 0;
    int32 InitialReceptacle = 0;
    int32 Palette = 0;
    TSet<FGuid> LoosePlacementGuids;

    auto CheckSource = [&](FName CanonicalId, const UGridItemDefinitionAsset* Source,
        int32 AuthoredLevel, int32 Quantity, bool bAllowPointerless) -> bool
    {
        // X.58: loose, palette and initial-content rows may be pointerless.
        if (!IsValid(Source) && bAllowPointerless)
        {
            const UGridCanonicalItemDefinitionAsset* const* Found = Registry.Find(CanonicalId);
            if (CanonicalId.IsNone() || !Found || !IsValid(*Found) ||
                Quantity < 1 || AuthoredLevel < 0 || AuthoredLevel > 20)
            {
                return false;
            }
            if (Cast<UGridArmamentDefinitionAsset>(*Found))
            {
                if (AuthoredLevel == 0) { return false; }
                ++ArmReferences;
            }
            else if (Cast<UGridNonArmamentDefinitionAsset>(*Found) && AuthoredLevel == 0)
            {
                ++NonArmN0;
            }
            else
            {
                return false;
            }
            ++Total;
            return true;
        }
        if (!IsValid(Source) || CanonicalId.IsNone() ||
            CanonicalId != Source->ItemDefinitionId ||
            Quantity < 1 || AuthoredLevel < 0 || AuthoredLevel > 20)
        {
            return false;
        }
        const UGridCanonicalItemDefinitionAsset* const* Found =
            Registry.Find(CanonicalId);
        if (!Found || !IsValid(*Found))
        {
            return false;
        }

        if (!Source->WeaponArchetypeId.IsNone())
        {
            const UGridArmamentDefinitionAsset* Weapon =
                Cast<UGridArmamentDefinitionAsset>(*Found);
            if (!Weapon || Weapon->ArchetypeId != Source->WeaponArchetypeId)
            {
                return false;
            }
            // Authored N0 is an inheritance instruction, NOT an actual
            // persisted ARM level. Never synthesize a default of N1.
            const int32 EffectiveLevel = AuthoredLevel == 0
                ? Source->DefaultWeaponItemLevel : AuthoredLevel;
            if (EffectiveLevel < 1 || EffectiveLevel > 20)
            {
                return false;
            }
            ++ArmReferences;
            if (AuthoredLevel == 0) { ++InheritedArmN0; }
        }
        else
        {
            if (!Cast<UGridNonArmamentDefinitionAsset>(*Found) ||
                AuthoredLevel != 0)
            {
                return false; // Non-ARM authoring really stays at N0.
            }
            ++NonArmN0;
        }
        ++Total;
        return true;
    };

    const TCHAR* LevelPaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };

    for (const TCHAR* Path : LevelPaths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(LoadObject<UGridLevelAsset>(
            nullptr, Path));
        if (!TestNotNull(TEXT("Real migrated level exists"), Level.Get()))
        {
            return false;
        }
        for (const FGridLooseItemInstance& Item : Level->LooseItemInstances)
        {
            if (!Item.InstanceId.IsValid() ||
                LoosePlacementGuids.Contains(Item.InstanceId) ||
                !CheckSource(Item.CanonicalItemId, nullptr,
                    Item.ItemLevel, Item.Quantity, true))
            {
                AddError(TEXT("Invalid loose placement identity, GUID or authored level."));
                return false;
            }
            LoosePlacementGuids.Add(Item.InstanceId);
            ++Loose;
        }
        for (const FGridItemSpawnInstance& Item : Level->ItemSpawns)
        {
            if (!Item.SpawnId.IsValid() ||
                !CheckSource(Item.CanonicalItemId, nullptr,
                    Item.ItemLevel, Item.Quantity, true))
            {
                AddError(TEXT("Invalid authored item spawn identity or level."));
                return false;
            }
            ++Spawns;
        }
        for (const FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
        {
            for (const FGridReceptacleInitialItemConfig& Item :
                Object.InstanceConfig.ReceptacleInitialContent)
            {
                if (!CheckSource(Item.CanonicalItemId, nullptr,
                    Item.ItemLevel, Item.Quantity, true))
                {
                    AddError(TEXT("Invalid receptacle initial item identity or level."));
                    return false;
                }
                ++InitialReceptacle;
            }
        }
    }

    TStrongObjectPtr<UGridObjectPaletteAsset> PaletteAsset(
        LoadObject<UGridObjectPaletteAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
                 "DA_ObjectPalette_Default.DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Native direct item palette"), PaletteAsset.Get()))
    {
        return false;
    }
    for (const FGridObjectPaletteEntry& Entry : PaletteAsset->Entries)
    {
        if (!Entry.IsDirectItemEntry()) { continue; }
        // X.5 materialized the physical palette default.
        if (!CheckSource(Entry.CanonicalItemId, nullptr,
            Entry.CanonicalDefaultItemLevel, 1, true))
        {
            AddError(TEXT("Invalid direct palette identity or inherited level."));
            return false;
        }
        ++Palette;
    }

    IAssetRegistry& AssetRegistry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
            TEXT("AssetRegistry")).Get();
    AssetRegistry.SearchAllAssets(true);
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype/Core/DataAssets")));
    Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> WorldDefinitions;
    AssetRegistry.GetAssets(Filter, WorldDefinitions);
    if (!TestEqual(TEXT("World definitions audited"), WorldDefinitions.Num(), 44))
    {
        return false;
    }

    int32 AcceptanceRules = 0;
    int32 UnmappedAcceptanceRules = 0;
    int32 WorldDefinitionInitialItems = 0;
    for (const FAssetData& Data : WorldDefinitions)
    {
        const UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(Data.GetAsset());
        if (!IsValid(Definition))
        {
            AddError(TEXT("Could not load an actual world-object definition."));
            return false;
        }
        const FGridReceptacleBehaviorParams& Receptacle =
            Definition->DefaultBehavior.Receptacle;
        WorldDefinitionInitialItems += Receptacle.InitialContent.Num();
        for (const FGridReceptacleAcceptedItemConfig& Rule :
            Receptacle.AcceptedItems)
        {
            ++AcceptanceRules;
            if (Rule.CanonicalItemId.IsNone() || !Registry.Contains(Rule.CanonicalItemId))
            {
                ++UnmappedAcceptanceRules;
            }
        }
    }
    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X2] ReadOnly Sources=%d ARM=%d InheritedARMN0=%d "
        "NonARMN0=%d AcceptanceRules=%d UnmappedAcceptanceRules=%d "
        "WorldDefinitionInitialItems=%d; no packages or SaveGames written."),
        Total, ArmReferences, InheritedArmN0, NonArmN0,
        AcceptanceRules, UnmappedAcceptanceRules, WorldDefinitionInitialItems));

    return TestEqual(TEXT("Persisted original loose item references"), Loose, 44) &&
        TestEqual(TEXT("Persisted authored spawn references"), Spawns, 0) &&
        TestEqual(TEXT("Persisted initial receptacle references"), InitialReceptacle, 18) &&
        TestEqual(TEXT("Persisted direct palette references"), Palette, 33) &&
        TestEqual(TEXT("All canonical authored references"), Total, 95) &&
        TestTrue(TEXT("Real canonical ARM families retain an authored physical level"),
            ArmReferences > 0) &&
        TestEqual(TEXT("Canonical family partition"), ArmReferences + NonArmN0, 95) &&
        TestEqual(TEXT("Exactly two registered acceptance rules"), AcceptanceRules, 2) &&
        TestEqual(TEXT("No unmapped acceptance rules"), UnmappedAcceptanceRules, 0) &&
        TestEqual(TEXT("No world-definition initial content in 06.4W scope"),
            WorldDefinitionInitialItems, 0);
}

#endif
