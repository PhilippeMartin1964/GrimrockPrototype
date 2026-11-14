#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AHLootLevelResolution,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalMonsterLootLevelResolution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AHLootLevelResolution::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Registered real ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) AddError(Error);
        return false;
    }
    TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
    Old->DefaultWeaponItemLevel = 6;
    Old->bUseWeaponCatalogue = false; // Contradict canonical ARM deliberately.
    FGridMonsterLootEntry Entry;
    Entry.ItemDefinitionAsset = Old.Get();
    Entry.DropChance = 1.0f;
    Entry.MinQuantity = 1;
    Entry.MaxQuantity = 4;
    int32 FixedLevel = INDEX_NONE;

    const FName Arms[] = {
        TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
        TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01") };
    for (const FName Id : Arms)
    {
        Old->ItemDefinitionId = Id;
        Entry.ItemDefinitionId = Id;
        for (const int32 Level : {1, 20})
        {
            Entry.ItemLevel = Level;
            if (!TestTrue(TEXT("Authoring valid"), Entry.IsValidDefinition())) return false;
            TestTrue(TEXT("Canonical ARM level does not need legacy profile"),
                FGridMonsterLootResolver::TryResolveItemLevel(
                    Entry, Old.Get(), FixedLevel, Registry.Get()));
            TestEqual(TEXT("Authored fixed level retained"), FixedLevel, Level);
        }
        Entry.ItemLevel = 0;
        TestTrue(TEXT("Fresh ARM loot inherits authored default once"),
            FGridMonsterLootResolver::TryResolveItemLevel(
                Entry, Old.Get(), FixedLevel, Registry.Get()));
        TestEqual(TEXT("Inherited default is N6, not assumed N1"), FixedLevel, 6);

        // An explicit fixed level can resolve without a historical definition.
        Entry.ItemDefinitionAsset = nullptr;
        Entry.ItemLevel = 20;
        TestTrue(TEXT("Known canonical fixed level resolves by ItemId"),
            FGridMonsterLootResolver::TryResolveItemLevel(
                Entry, nullptr, FixedLevel, Registry.Get()));
        TestEqual(TEXT("Id-only fixed loot retains N20"), FixedLevel, 20);
        Entry.ItemLevel = 0;
        TestFalse(TEXT("Cannot invent an inherited default without definition"),
            FGridMonsterLootResolver::TryResolveItemLevel(
                Entry, nullptr, FixedLevel, Registry.Get()));
        TestEqual(TEXT("Failed read resets output"), FixedLevel, 0);
        Entry.ItemDefinitionAsset = Old.Get();
    }

    Old->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
    Entry.ItemDefinitionId = Old->ItemDefinitionId;
    Entry.ItemLevel = 0;
    Old->DefaultWeaponItemLevel = 0;
    TestFalse(TEXT("ARM default N0 must fail closed"),
        FGridMonsterLootResolver::TryResolveItemLevel(
            Entry, Old.Get(), FixedLevel, Registry.Get()));
    TestEqual(TEXT("N0 rejection leaves level zero"), FixedLevel, 0);
    Old->DefaultWeaponItemLevel = 6;

    Old->ItemDefinitionId = TEXT("Item_Arrows_01");
    Old->bUseWeaponCatalogue = true; // Contradict canonical ammunition deliberately.
    Entry.ItemDefinitionId = Old->ItemDefinitionId;
    Entry.ItemLevel = 0;
    TestTrue(TEXT("Canonical arrows remain genuine N0"),
        FGridMonsterLootResolver::TryResolveItemLevel(
            Entry, Old.Get(), FixedLevel, Registry.Get()));
    TestEqual(TEXT("Arrows never inherit ARM default"), FixedLevel, 0);
    Entry.ItemLevel = 1;
    TestFalse(TEXT("Unlevelled ammo cannot be assigned a kit/loot weapon level"),
        FGridMonsterLootResolver::TryResolveItemLevel(
            Entry, Old.Get(), FixedLevel, Registry.Get()));

    Old->ItemDefinitionId = TEXT("LegacyLoot_AH");
    Old->bUseWeaponCatalogue = false;
    Entry.ItemDefinitionId = Old->ItemDefinitionId;
    Entry.ItemLevel = 0;
    TestTrue(TEXT("Unknown legacy loot N0 remains supported"),
        FGridMonsterLootResolver::TryResolveItemLevel(
            Entry, Old.Get(), FixedLevel, Registry.Get()));
    TestEqual(TEXT("Unknown legacy level unchanged"), FixedLevel, 0);
    Entry.ItemLevel = 9;
    TestFalse(TEXT("Unknown non-catalogue loot cannot invent N9"),
        FGridMonsterLootResolver::TryResolveItemLevel(
            Entry, Old.Get(), FixedLevel, Registry.Get()));
    Entry.ItemDefinitionAsset = nullptr;
    Entry.ItemLevel = 0;
    TestTrue(TEXT("Unknown id-only loot N0 remains supported"),
        FGridMonsterLootResolver::TryResolveItemLevel(
            Entry, nullptr, FixedLevel, Registry.Get()));

    // RNG remains independent of ItemLevel; this is a pure post-roll read.
    Entry.ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    Entry.ItemDefinitionAsset = nullptr;
    Entry.ItemLevel = 1;
    const TArray<FGridMonsterLootRollResult> Low =
        FGridMonsterLootResolver::ResolveLoot(TArray<FGridMonsterLootEntry>{Entry}, 631);
    Entry.ItemLevel = 20;
    const TArray<FGridMonsterLootRollResult> High =
        FGridMonsterLootResolver::ResolveLoot(TArray<FGridMonsterLootEntry>{Entry}, 631);
    if (!TestEqual(TEXT("Both loot tables yield one deterministic roll"), Low.Num(), 1) ||
        !TestEqual(TEXT("Both loot tables yield one deterministic roll"), High.Num(), 1))
        return false;
    TestEqual(TEXT("Level changes no RNG drop probability"), Low[0].DropRoll, High[0].DropRoll);
    TestEqual(TEXT("Level changes no RNG stack quantity"), Low[0].Quantity, High[0].Quantity);
    TestEqual(TEXT("Level changes no RNG drop decision"), Low[0].bDropped, High[0].bDropped);
    return true;
}

#endif
