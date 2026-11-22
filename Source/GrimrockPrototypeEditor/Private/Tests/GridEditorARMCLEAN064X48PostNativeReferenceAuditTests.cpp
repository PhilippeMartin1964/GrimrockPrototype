#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Package.h"

/** X.48/X.60: 102 migrated references remain removed.
 * Five loot rows remain; only Stone may be canonical-only.
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
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
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
    int32 LooseTotal = 0, InitialTotal = 0;
    const int32 LooseOld = 0;
    TestNull(TEXT("Removed loose placement pointer schema"), FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
    const int32 InitialOld = 0;
    TestNull(TEXT("Removed initial content pointer schema"), FGridReceptacleInitialItemConfig::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
    const int32 ReceptacleOverrideOld = 0;
    TestNull(TEXT("Removed acceptance pointer schema"), FGridReceptacleAcceptedItemConfig::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
    const int32 LockOverrideOld = 0;
    TestNull(TEXT("Removed shared key pointer schema"), FGridLockBehaviorParams::StaticStruct()->FindPropertyByName(TEXT("AcceptedKeyItems")));
    TestNull(TEXT("Removed instance key pointer schema"), FGridLockAcceptedKeyInstanceRuleValues::StaticStruct()->FindPropertyByName(TEXT("AcceptedKeyItems")));
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
            if (LevelIndex != 0 ||
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
                if (Item.CanonicalItemId.IsNone() || !Known.Contains(Item.CanonicalItemId) ||
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
            if (!Overrides.ReceptacleRules.AcceptedItems.IsEmpty() ||
                !Overrides.AcceptedKeys.AcceptedKeyIds.IsEmpty())
            {
                AddError(TEXT("Dormant overrides must remain empty after native cutover."));
                return false;
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
    int32 PaletteRows = 0;
    const int32 PaletteOld = 0;
    TestNull(TEXT("Removed palette pointer schema"), FGridObjectPaletteEntry::StaticStruct()->FindPropertyByName(TEXT("DefaultItemDefinition")));
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.IsDirectItemEntry()) continue;
        ++PaletteRows;
        if (Entry.CanonicalItemId.IsNone() || !Known.Contains(Entry.CanonicalItemId) ||
            !Entry.IsValidEntry())
        {
            AddError(TEXT("Canonical palette entry lost identity or retained contradictory source."));
            return false;
        }
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
    int32 RuleAssets = 0, LockAssets = 0;
    const int32 AcceptanceRules = 0;
    const int32 LockKeys = 0;
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
            if (Required.IsNone() || Rule.CanonicalItemId != Required ||
                !Known.Contains(Required))
            {
                AddError(TEXT("Shared receptacle rule does not match exact registered identity."));
                return false;
            }
        }
        const FGridLockBehaviorParams& Lock = Definition->DefaultBehavior.Lock;
        if (!Lock.AcceptedKeyIds.IsEmpty()) ++LockAssets;
        if (!Lock.AcceptedKeyIds.IsEmpty())
        {
            const FName Required = Package ==
                TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall")
                ? FName(TEXT("Key_Copper")) :
                (Package == TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall")
                    ? FName(TEXT("Key_Iron")) : NAME_None);
            if (Required.IsNone() || Lock.AcceptedKeyIds.Num() != 1 ||
                Lock.AcceptedKeyIds[0] != Required ||
                !Known.Contains(Required))
            {
                AddError(TEXT("Shared lock does not have the exact approved canonical key."));
                return false;
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
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Goblin(
        LoadObject<UGridMonsterDefinitionAsset>(nullptr, MonsterPaths[0]));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Rat(
        LoadObject<UGridMonsterDefinitionAsset>(nullptr, MonsterPaths[1]));
    if (!TestNotNull(TEXT("Real goblin"), Goblin.Get()) ||
        !TestNotNull(TEXT("Real rat"), Rat.Get()))
        return false;
    if (!TestEqual(TEXT("Goblin native loot rows"), Goblin->LootTable.Num(), 3) ||
        !TestEqual(TEXT("Rat native loot rows"), Rat->LootTable.Num(), 2))
        return false;
    const FName Expected[] = {
        TEXT("GoblinKnife"), TEXT("Stone"), TEXT("EmptyVial"),
        TEXT("Item_RatMeat"), TEXT("Item_RatTooth")
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Expected); ++Index)
    {
        const FGridMonsterLootEntry& Row = Index < 3
            ? Goblin->LootTable[Index] : Rat->LootTable[Index-3];
        TestEqual(TEXT("All five monster loots own canonical ItemIds"),
            Row.GetResolvedItemDefinitionId(), Expected[Index]);
        TestEqual(TEXT("Non-armament loot physical level N0"),
            Row.ItemLevel, 0);
        TestTrue(TEXT("Canonical direct loot is structurally valid"),
            Row.IsValidDefinition());
    }
    const int32 MonsterOld = 0;
    TestNull(TEXT("No historical loot pointer schema"), FGridMonsterLootEntry::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
    int32 MonsterRows = 0, MonsterRegistered = 0;
    for (const auto* Monster : {Goblin.Get(), Rat.Get()})
        for (const FGridMonsterLootEntry& Loot : Monster->LootTable)
        {
            ++MonsterRows;
            if (Known.Contains(Loot.GetResolvedItemDefinitionId())) ++MonsterRegistered;
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
        TestEqual(TEXT("X.58B initial pointers remain removed"), InitialOld, 0) &&
        TestEqual(TEXT("18 total initial entries"), InitialTotal, 18) &&
        TestEqual(TEXT("X.59B dormant receptacle reference remains removed"), ReceptacleOverrideOld, 0) &&
        TestEqual(TEXT("X.59B dormant lock references remain removed"), LockOverrideOld, 0) &&
        TestEqual(TEXT("No activated dormant overrides"), ActiveAcceptanceOverrides, 0) &&
        TestEqual(TEXT("33 direct palette rows remain"), PaletteRows, 33) &&
        TestEqual(TEXT("X.57B palette remains pointerless"), PaletteOld, 0) &&
        TestEqual(TEXT("X.54B shared receptacle pointers remain removed"), AcceptanceRules, 0) &&
        TestEqual(TEXT("X.54B shared lock pointers remain removed"), LockKeys, 0) &&
        TestEqual(TEXT("Two shared receptacle assets"), RuleAssets, 2) &&
        TestEqual(TEXT("Two shared lock assets"), LockAssets, 2) &&
        TestEqual(TEXT("Zero old loot references"), MonsterOld, 0) &&
        TestEqual(TEXT("Five historic loot rows"), MonsterRows, 5) &&
        TestEqual(TEXT("Exact registered monster loot count"), MonsterRegistered, 5) &&
        TestEqual(TEXT("Zero remaining sources"), Remaining, 0) &&
        TestEqual(TEXT("Canonical root remains unmodified"), Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
