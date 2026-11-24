#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "ARMEMENT/GridArmClean064X3AuthoredLevelMaterializer.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

/**
 * ARM-CLEAN06.4X.4: assert that the *real, already migrated* native UE
 * authoring is eligible for a deterministic N0-to-physical-level cutover.
 * Never modify UAssets, SaveGames, item defaults or item identities here.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X4RealAuthoredLevels,
    "Grimrock.RPG.ARM.CLEAN06_4X4.RealAuthoredLevelPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X4RealAuthoredLevels::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Native ItemSystem root"), Root.Get()) ||
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
    {
        return false;
    }

    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(
        NewObject<UGridItemSubsystem>(GI.Get()));
    if (!TestNotNull(TEXT("Real canonical resolver"), Registry.Get()))
    {
        return false;
    }
    TArray<FString> RegisterErrors;
    if (!TestTrue(TEXT("One root registers without a legacy catalogue"),
        Registry->RegisterItemSystem(Root.Get(), RegisterErrors)))
    {
        for (const FString& Message : RegisterErrors)
        {
            AddError(Message);
        }
        return false;
    }

    TMap<FName, const UGridCanonicalItemDefinitionAsset*> Definitions;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Definitions.Contains(Definition->ItemId))
        {
            AddError(TEXT("Duplicate, absent or invalid real ItemId."));
            return false;
        }
        Definitions.Add(Definition->ItemId, Definition);
    }
    if (!TestEqual(TEXT("Exact unique registry identities"), Definitions.Num(), Root->ItemDefinitions.Num()))
    {
        return false;
    }

    int32 LooseCount = 0;
    int32 SpawnCount = 0;
    int32 ReceptacleCount = 0;
    int32 PaletteCount = 0;
    int32 CanonicalCount = 0;
    int32 ArmCount = 0;
    int32 InheritedArmCount = 0;
    int32 NonArmCount = 0;
    TSet<FGuid> PhysicalGuids;

    const auto Check = [this, &Definitions, &Registry, &CanonicalCount,
        &ArmCount, &InheritedArmCount, &NonArmCount](
            const FName AuthoredItemId,
            int32 AuthoredLevel, int32 Quantity) -> bool
    {
        const UGridCanonicalItemDefinitionAsset* const* Found =
            Definitions.Find(AuthoredItemId);
        if (AuthoredItemId.IsNone() || !Found || !IsValid(*Found) ||
            Quantity < 1 || AuthoredLevel < 0 || AuthoredLevel > 20)
        {
            return false;
        }
        const UGridArmamentDefinitionAsset* Arm =
            Cast<UGridArmamentDefinitionAsset>(*Found);
        if (Arm)
        {
            if (AuthoredLevel == 0 || Arm->ArchetypeId.IsNone()) { return false; }
            ++ArmCount;
        }
        else if (Cast<UGridNonArmamentDefinitionAsset>(*Found) &&
            AuthoredLevel == 0)
        {
            ++NonArmCount;
        }
        else
        {
            return false;
        }
        FGridResolvedItem Physical;
        if (!Registry->ResolveItem(AuthoredItemId, AuthoredLevel, Physical) ||
            !Physical.IsValid() || Physical.ItemId != AuthoredItemId ||
            Physical.ItemLevel != AuthoredLevel ||
            Physical.bIsArmament != (Arm != nullptr) ||
            (Arm && Physical.ArchetypeId != Arm->ArchetypeId))
        {
            return false;
        }
        ++CanonicalCount;
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
        TStrongObjectPtr<UGridLevelAsset> Level(
            LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("Real migrated grid level"), Level.Get()))
        {
            return false;
        }
        const bool bWasDirty = Level->GetOutermost()->IsDirty();

        for (const FGridLooseItemInstance& Placement : Level->LooseItemInstances)
        {
            if (!Placement.InstanceId.IsValid() ||
                PhysicalGuids.Contains(Placement.InstanceId) ||
                !Check(Placement.ItemId,
                    Placement.ItemLevel, Placement.Quantity))
            {
                AddError(TEXT("Invalid source, duplicate GUID or level in real loose placement."));
                return false;
            }
            PhysicalGuids.Add(Placement.InstanceId);
            ++LooseCount;
        }

        for (const FGridItemSpawnInstance& Spawn : Level->ItemSpawns)
        {
            if (!Spawn.SpawnId.IsValid() ||
                !Check(Spawn.ItemId,
                    Spawn.ItemLevel, Spawn.Quantity))
            {
                AddError(TEXT("Invalid source identity or level in authored spawn."));
                return false;
            }
            ++SpawnCount;
        }

        for (const FGridWorldObjectInstance& World : Level->WorldObjectInstances)
        {
            for (const FGridReceptacleInitialItemConfig& Initial :
                World.InstanceConfig.ReceptacleInitialContent)
            {
                if (!Check(Initial.ItemId,
                    Initial.ItemLevel, Initial.Quantity))
                {
                    AddError(TEXT("Invalid initial receptacle physical level."));
                    return false;
                }
                ++ReceptacleCount;
            }
        }

        if (!TestEqual(TEXT("Read-only preflight never dirties a level"),
            Level->GetOutermost()->IsDirty(), bWasDirty))
        {
            return false;
        }
    }

    TStrongObjectPtr<UGridObjectPaletteAsset> Palette(
        LoadObject<UGridObjectPaletteAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
                 "DA_ObjectPalette_Default.DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Real migrated item palette"), Palette.Get()))
    {
        return false;
    }
    const bool bPaletteWasDirty = Palette->GetOutermost()->IsDirty();
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.IsDirectItemEntry())
        {
            continue;
        }
        // X.5 materialized the physical default in the palette.
        if (!Check(Entry.ItemId,
                Entry.DefaultItemLevel, 1))
        {
            AddError(TEXT("Physical level for a direct palette item is unresolved."));
            return false;
        }
        ++PaletteCount;
    }

    if (!TestEqual(TEXT("Read-only preflight never dirties the palette"),
        Palette->GetOutermost()->IsDirty(), bPaletteWasDirty))
    {
        return false;
    }

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X4] READ_ONLY real refs=%d loose=%d spawns=%d "
        "receptacles=%d palette=%d arm=%d inheritedArmN0=%d nonArmN0=%d. "
        "No native packages, .sav, .umap or WBP were saved."),
        CanonicalCount, LooseCount, SpawnCount, ReceptacleCount,
        PaletteCount, ArmCount, InheritedArmCount, NonArmCount));

    return TestEqual(TEXT("Exact native loose placements"), LooseCount, 44) &&
        TestEqual(TEXT("Exact native generated item sources"), SpawnCount, 0) &&
        TestEqual(TEXT("Exact native initial receptacle items"), ReceptacleCount, 18) &&
        TestEqual(TEXT("Exact native direct palette items"), PaletteCount, 33) &&
        TestEqual(TEXT("All 95 persisted ItemIds have a resolvable physical level"),
            CanonicalCount, 95) &&
        TestEqual(TEXT("All 95 identities belong to a single family"),
            ArmCount + NonArmCount, 95) &&
        TestTrue(TEXT("Actual ARM families have physical N1-N20 levels"),
            ArmCount > 0) &&
        TestTrue(TEXT("Real non-armament N0 actually occurs"),
            NonArmCount > 0);
}

#endif
