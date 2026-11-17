#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean064X3AuthoredLevelMaterializer.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
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
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Native ItemSystem root"), Root.Get()) ||
        !TestEqual(TEXT("Original 34 canonical definitions"), Root->ItemDefinitions.Num(), 34))
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
    if (!TestEqual(TEXT("Exactly 34 unique registry identities"), Definitions.Num(), 34))
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
            const FName AuthoredItemId, const UGridItemDefinitionAsset* Source,
            int32 AuthoredLevel, int32 Quantity, bool bAllowPointerless) -> bool
    {
        // X.45: tolerate a completed native cutover ONLY for the 44
        // loose objects. All other old references remain required until
        // their own independent migration, and N-levels stay physical.
        if (!IsValid(Source) && bAllowPointerless)
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
        }
        if (!IsValid(Source) || AuthoredItemId.IsNone() ||
            Source->ItemDefinitionId != AuthoredItemId ||
            Quantity < 1)
        {
            AddError(TEXT("Authored ItemId, historical source or quantity is invalid."));
            return false;
        }

        const UGridCanonicalItemDefinitionAsset* const* Found =
            Definitions.Find(AuthoredItemId);
        if (!Found || !IsValid(*Found))
        {
            AddError(TEXT("An authored ItemId does not exist in the native registry."));
            return false;
        }

        int32 EffectiveLevel = INDEX_NONE;
        bool bInherited = false;
        if (!FGridArmClean064X3AuthoredLevelMaterializer::TryResolve(
            *Found, AuthoredLevel, Source->DefaultWeaponItemLevel,
            EffectiveLevel, bInherited))
        {
            AddError(TEXT("Real authored source cannot yield a physical ItemLevel."));
            return false;
        }

        const UGridArmamentDefinitionAsset* Arm =
            Cast<UGridArmamentDefinitionAsset>(*Found);
        if (Arm)
        {
            if (Arm->ArchetypeId.IsNone() ||
                Arm->ArchetypeId != Source->WeaponArchetypeId ||
                EffectiveLevel < 1 || EffectiveLevel > 20 ||
                bInherited != (AuthoredLevel == 0) ||
                (bInherited && EffectiveLevel != Source->DefaultWeaponItemLevel) ||
                (!bInherited && EffectiveLevel != AuthoredLevel))
            {
                AddError(TEXT("Physical ARM level or real weapon source differs."));
                return false;
            }
            ++ArmCount;
            if (bInherited)
            {
                ++InheritedArmCount;
            }
        }
        else if (!Cast<UGridNonArmamentDefinitionAsset>(*Found) ||
            !Source->WeaponArchetypeId.IsNone() ||
            AuthoredLevel != 0 || EffectiveLevel != 0 || bInherited)
        {
            AddError(TEXT("Unlevelled canonical non-ARM must remain physical N0."));
            return false;
        }
        else
        {
            ++NonArmCount;
        }

        FGridResolvedItem Resolved;
        if (!Registry->ResolveItem(AuthoredItemId, EffectiveLevel, Resolved) ||
            !Resolved.IsValid() ||
            Resolved.ItemId != AuthoredItemId ||
            Resolved.ItemLevel != EffectiveLevel ||
            Resolved.bIsArmament != (Arm != nullptr) ||
            (Arm && Resolved.ArchetypeId != Arm->ArchetypeId))
        {
            AddError(TEXT("Resulting physical level cannot resolve through ItemSystem."));
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
                !Check(Placement.CanonicalItemId, Placement.ItemDefinition,
                    Placement.ItemLevel, Placement.Quantity, true))
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
                !Check(Spawn.CanonicalItemId, Spawn.ItemDefinition,
                    Spawn.ItemLevel, Spawn.Quantity, false))
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
                if (!Check(Initial.CanonicalItemId, Initial.ItemDefinition,
                    Initial.ItemLevel, Initial.Quantity, false))
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
        if (!Entry.DefaultItemDefinition)
        {
            continue;
        }
        // Palette entries have no authored N-level; the source-defined
        // default is necessary when a NEW ARM placement is created.
        if (!Check(Entry.CanonicalItemId, Entry.DefaultItemDefinition, 0, 1, false))
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
