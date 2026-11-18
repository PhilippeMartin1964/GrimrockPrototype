#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Package.h"

/** X.48/X.58: exact native baseline after X.57B.
 *  44 loose, four shared rules and 33 palette rows are pointerless.
 *  Exactly 18 initial-content rows in levels 00/01 retain all or no
 *  old pointers; the three dormant overrides and five loots stay strict.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X48PostNativeRefs,
    "Grimrock.RPG.ARM.CLEAN06_4X48.PostNative63ReferenceBaseline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X48PostNativeRefs::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Native item system root"), Root.Get()) ||
        !TestEqual(TEXT("34 canonical definitions"), Root->ItemDefinitions.Num(), 34))
    {
        return false;
    }

    TSet<FName> Known;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Known.Contains(Definition->ItemId))
        {
            AddError(TEXT("Invalid/duplicate canonical ItemId in real registry."));
            return false;
        }
        Known.Add(Definition->ItemId);
    }
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    int32 LooseTotal = 0, LooseOld = 0, InitialTotal = 0, InitialOld = 0;
    int32 ReceptacleOverrideOld = 0, LockOverrideOld = 0;
    int32 ActiveAcceptanceOverrides = 0;
    int32 ItemSpawns = 0;
    TSet<FGuid> SeenLoose;
    const TCHAR* Levels[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };
    for (int32 LevelIndex = 0; LevelIndex < 4; ++LevelIndex)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(
            LoadObject<UGridLevelAsset>(nullptr, Levels[LevelIndex]));
        if (!TestNotNull(TEXT("Real native level"), Level.Get())) return false;
        const bool bWasDirty = Level->GetOutermost()->IsDirty();
        ItemSpawns += Level->ItemSpawns.Num();
        for (const FGridLooseItemInstance& Item : Level->LooseItemInstances)
        {
            ++LooseTotal;
            if (Item.ItemDefinition) ++LooseOld;
            if (LevelIndex != 0 || Item.ItemDefinition ||
                !Item.InstanceId.IsValid() || SeenLoose.Contains(Item.InstanceId) ||
                Item.CanonicalItemId.IsNone() || !Known.Contains(Item.CanonicalItemId) ||
                Item.Quantity < 1 || !Level->IsValidCoord(Item.CellX, Item.CellY) ||
                Item.ItemLevel < 0 || Item.ItemLevel > 20)
            {
                AddError(TEXT("Migrated loose row is no longer canonical pointerless."));
                return false;
            }
            SeenLoose.Add(Item.InstanceId);
        }
        const int32 BeforeInitial = InitialTotal;
        for (const FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
        {
            for (const FGridReceptacleInitialItemConfig& Item :
                Object.InstanceConfig.ReceptacleInitialContent)
            {
                ++InitialTotal;
                if (Item.ItemDefinition) ++InitialOld;
                if (Item.CanonicalItemId.IsNone() || !Known.Contains(Item.CanonicalItemId) ||
                    (Item.ItemDefinition && (!IsValid(Item.ItemDefinition.Get()) ||
                        Item.ItemDefinition->ItemDefinitionId != Item.CanonicalItemId)) ||
                    Item.Quantity < 1 || Item.ItemLevel < 0 || Item.ItemLevel > 20)
                {
                    AddError(TEXT("Initial receptacle content identity changed unexpectedly."));
                    return false;
                }
            }
            const FGridWorldObjectInteractionOverrides& Overrides =
                Object.InstanceConfig.InteractionOverrides;
            if (Overrides.bOverrideReceptacleRules || Overrides.bOverrideAcceptedKeys)
            {
                ++ActiveAcceptanceOverrides;
            }
            for (const FGridReceptacleAcceptedItemConfig& Rule :
                Overrides.ReceptacleRules.AcceptedItems)
            {
                if (!IsValid(Rule.ItemDefinition.Get()))
                {
                    AddError(TEXT("Dormant receptacle override lost its old reference."));
                    return false;
                }
                ++ReceptacleOverrideOld;
            }
            for (const TObjectPtr<UGridItemDefinitionAsset>& Key :
                Overrides.AcceptedKeys.AcceptedKeyItems)
            {
                if (!IsValid(Key.Get()))
                {
                    AddError(TEXT("Dormant lock override lost its old reference."));
                    return false;
                }
                ++LockOverrideOld;
            }
        }
        const int32 ExpectedInitial = LevelIndex == 0 ? 17 : LevelIndex == 1 ? 1 : 0;
        if (!TestEqual(TEXT("Exact original per-level distribution"), InitialTotal - BeforeInitial, ExpectedInitial))
            return false;
        if (!TestEqual(TEXT("Real level remains unmodified"), Level->GetOutermost()->IsDirty(), bWasDirty))
        {
            return false;
        }
    }

    TStrongObjectPtr<UGridObjectPaletteAsset> Palette(
        LoadObject<UGridObjectPaletteAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
                 "DA_ObjectPalette_Default.DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Real direct item palette"), Palette.Get())) return false;
    const bool bPaletteDirty = Palette->GetOutermost()->IsDirty();
    int32 PaletteRows = 0, PaletteOld = 0;
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.IsDirectItemEntry()) continue;
        ++PaletteRows;
        if (Entry.CanonicalItemId.IsNone() || !Known.Contains(Entry.CanonicalItemId) ||
            !Entry.IsValidEntry() || (Entry.DefaultItemDefinition &&
                (!IsValid(Entry.DefaultItemDefinition.Get()) ||
                 Entry.DefaultItemDefinition->ItemDefinitionId != Entry.CanonicalItemId)))
        {
            AddError(TEXT("Canonical palette entry lost identity or retained contradictory source."));
            return false;
        }
        if (Entry.DefaultItemDefinition) ++PaletteOld;
    }
    if (!TestEqual(TEXT("Palette remains unmodified"), Palette->GetOutermost()->IsDirty(), bPaletteDirty))
        return false;

    FAssetRegistryModule& AssetRegistry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype/Core/DataAssets")));
    Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> Definitions;
    AssetRegistry.Get().GetAssets(Filter, Definitions);
    if (!TestEqual(TEXT("44 real world-object definitions inspected"), Definitions.Num(), 44))
        return false;
    int32 AcceptanceRules = 0, LockKeys = 0, RuleAssets = 0, LockAssets = 0;
    for (const FAssetData& Data : Definitions)
    {
        UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(Data.GetAsset());
        if (!TestNotNull(TEXT("World definition asset loaded"), Definition)) return false;
        const bool bWasDirty = Definition->GetOutermost()->IsDirty();
        const FGridReceptacleBehaviorParams& Receptacle = Definition->DefaultBehavior.Receptacle;
        if (!Receptacle.AcceptedItems.IsEmpty()) ++RuleAssets;
        const FString Package = Definition->GetOutermost()->GetName();
        for (const FGridReceptacleAcceptedItemConfig& Rule : Receptacle.AcceptedItems)
        {
            const FName Required = Package ==
                TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder")
                ? FName(TEXT("Item_Torch")) :
                (Package ==
                    TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian")
                    ? FName(TEXT("Gem_Blue")) : NAME_None);
            const UGridItemDefinitionAsset* Old = Rule.ItemDefinition.Get();
            if (Required.IsNone() || Rule.CanonicalItemId != Required ||
                !Known.Contains(Required) ||
                (Rule.ItemDefinition && (!IsValid(Old) ||
                    Old->ItemDefinitionId != Required)))
            {
                AddError(TEXT("Shared receptacle rule is neither exact legacy-backed nor exact canonical-only."));
                return false;
            }
            if (Old) ++AcceptanceRules;
        }
        const FGridLockBehaviorParams& Lock = Definition->DefaultBehavior.Lock;
        if (!Lock.AcceptedKeyIds.IsEmpty()) ++LockAssets;
        if (!Lock.AcceptedKeyItems.IsEmpty() || !Lock.AcceptedKeyIds.IsEmpty())
        {
            const FName Required = Package ==
                TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall")
                ? FName(TEXT("Key_Copper")) :
                (Package == TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall")
                    ? FName(TEXT("Key_Iron")) : NAME_None);
            if (Required.IsNone() || Lock.AcceptedKeyIds.Num() != 1 ||
                Lock.AcceptedKeyIds[0] != Required ||
                !Known.Contains(Required) || Lock.AcceptedKeyItems.Num() > 1)
            {
                AddError(TEXT("Shared lock does not have the exact approved canonical key."));
                return false;
            }
            for (const TObjectPtr<UGridItemDefinitionAsset>& Key : Lock.AcceptedKeyItems)
            {
                if (!IsValid(Key.Get()) || Key->ItemDefinitionId != Required)
                {
                    AddError(TEXT("Existing historical key does not match the canonical key."));
                    return false;
                }
                ++LockKeys;
            }
        }
        if (!TestEqual(TEXT("World definition remains unmodified"),
            Definition->GetOutermost()->IsDirty(), bWasDirty)) return false;
    }

    const TCHAR* MonsterPaths[] = {
        TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/"
             "DA_MON_GoblinThrower.DA_MON_GoblinThrower"),
        TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/"
             "DA_MON_RatGiant.DA_MON_RatGiant")
    };
    int32 MonsterOld = 0, MonsterRows = 0, MonsterRegistered = 0;
    for (const TCHAR* Path : MonsterPaths)
    {
        TStrongObjectPtr<UGridMonsterDefinitionAsset> Monster(
            LoadObject<UGridMonsterDefinitionAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("Real historic monster definition"), Monster.Get()))
            return false;
        const bool bWasDirty = Monster->GetOutermost()->IsDirty();
        for (const FGridMonsterLootEntry& Loot : Monster->LootTable)
        {
            ++MonsterRows;
            if (Loot.ItemDefinitionAsset) ++MonsterOld;
            if (!IsValid(Loot.ItemDefinitionAsset.Get()) ||
                !Loot.ItemDefinitionId.IsNone() || Loot.ItemLevel != 0 ||
                Loot.MinQuantity < 1 || Loot.MaxQuantity < Loot.MinQuantity)
            {
                AddError(TEXT("Historical monster loot unexpectedly modified."));
                return false;
            }
            if (Known.Contains(Loot.ItemDefinitionAsset->ItemDefinitionId))
                ++MonsterRegistered;
        }
        if (!TestEqual(TEXT("Monster package remains unmodified"),
            Monster->GetOutermost()->IsDirty(), bWasDirty)) return false;
    }

    const int32 Remaining = LooseOld + InitialOld + ReceptacleOverrideOld +
        LockOverrideOld + PaletteOld + AcceptanceRules + LockKeys + MonsterOld;
    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X48] REAL POST-NATIVE scan: loose %d/%d, "
        "receptacle initial %d/%d, dormant overrides receptacle %d lock %d, "
        "palette %d/%d, world acceptance %d/%d assets, world locks %d/%d assets, "
        "monster loot %d/%d (%d canonical IDs), REMAINING=%d; "
        "no assets or SaveGames saved."),
        LooseOld, LooseTotal, InitialOld, InitialTotal,
        ReceptacleOverrideOld, LockOverrideOld, PaletteOld, PaletteRows,
        AcceptanceRules, RuleAssets, LockKeys, LockAssets,
        MonsterOld, MonsterRows, MonsterRegistered, Remaining));

    return TestEqual(TEXT("44 native loose instances remain pointerless"), LooseTotal, 44) &&
        TestEqual(TEXT("Zero old pointers among 44"), LooseOld, 0) &&
        TestEqual(TEXT("No historical item spawns"), ItemSpawns, 0) &&
        TestTrue(TEXT("All 18 initial pointers present or all absent"), InitialOld == 18 || InitialOld == 0) &&
        TestEqual(TEXT("18 total initial entries"), InitialTotal, 18) &&
        TestEqual(TEXT("One dormant receptacle rule reference"), ReceptacleOverrideOld, 1) &&
        TestEqual(TEXT("Two dormant lock key references"), LockOverrideOld, 2) &&
        TestEqual(TEXT("No activated dormant overrides"), ActiveAcceptanceOverrides, 0) &&
        TestEqual(TEXT("33 direct palette rows remain"), PaletteRows, 33) &&
        TestEqual(TEXT("X.57B palette remains pointerless"), PaletteOld, 0) &&
        TestEqual(TEXT("X.54B shared receptacle pointers remain removed"), AcceptanceRules, 0) &&
        TestEqual(TEXT("X.54B shared lock pointers remain removed"), LockKeys, 0) &&
        TestEqual(TEXT("Two shared receptacle assets"), RuleAssets, 2) &&
        TestEqual(TEXT("Two shared lock assets"), LockAssets, 2) &&
        TestEqual(TEXT("Five historic monster loot sources"), MonsterOld, 5) &&
        TestEqual(TEXT("Five historic loot rows"), MonsterRows, 5) &&
        TestEqual(TEXT("One already registered monster loot identity"), MonsterRegistered, 1) &&
        TestEqual(TEXT("Exactly 26 before initial-content cutover or 8 after"),
            Remaining, InitialOld == 18 ? 26 : 8) &&
        TestEqual(TEXT("Canonical root remains unmodified"), Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
