#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
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
    FGridMonsterLootEntry Entry;
    Entry.DropChance = 1.0f;
    Entry.MinQuantity = 1;
    Entry.MaxQuantity = 4;
    int32 FixedLevel = INDEX_NONE;
    const FName Arms[] = {
        TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
        TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01") };
    for (const FName Id : Arms)
    {
        Entry.ItemDefinitionId = Id;
        for (const int32 Level : {1, 20})
        {
            Entry.ItemLevel = Level;
            if (!TestTrue(TEXT("Authoring valid"), Entry.IsValidDefinition())) return false;
            TestTrue(TEXT("Canonical ARM resolves without a definition pointer"),
                FGridMonsterLootResolver::TryResolveItemLevel(Entry, FixedLevel, Registry.Get()));
            TestEqual(TEXT("Authored fixed level retained"), FixedLevel, Level);
        }
        Entry.ItemLevel = 0;
        TestFalse(TEXT("ARM N0 cannot inherit a default"),
            FGridMonsterLootResolver::TryResolveItemLevel(Entry, FixedLevel, Registry.Get()));
        TestEqual(TEXT("Refused level resets output"), FixedLevel, 0);
    }
    Entry.ItemDefinitionId = TEXT("Item_Arrows_01");
    Entry.ItemLevel = 0;
    TestTrue(TEXT("Canonical arrows remain N0"),
        FGridMonsterLootResolver::TryResolveItemLevel(Entry, FixedLevel, Registry.Get()));
    TestEqual(TEXT("Arrows remain unlevelled"), FixedLevel, 0);
    for (const int32 Level : {1, 20, 21, -1})
    {
        Entry.ItemLevel = Level;
        TestFalse(TEXT("Invalid non-ARM level rejected"),
            FGridMonsterLootResolver::TryResolveItemLevel(Entry, FixedLevel, Registry.Get()));
    }
    Entry.ItemLevel = 0;
    Entry.ItemDefinitionId = TEXT("UnknownLoot_AH");
    TestFalse(TEXT("Unknown ID cannot create physical loot"),
        FGridMonsterLootResolver::TryResolveItemLevel(Entry, FixedLevel, Registry.Get()));
    Entry.ItemDefinitionId = TEXT("Stone");
    TestFalse(TEXT("No registry cannot create physical loot"),
        FGridMonsterLootResolver::TryResolveItemLevel(Entry, FixedLevel, nullptr));
    Entry.ItemDefinitionId = NAME_None;
    TestFalse(TEXT("No explicit identity is invalid"), Entry.IsValidDefinition());

    // RNG remains independent of ItemLevel; this is a pure post-roll read.
    Entry.ItemDefinitionId = TEXT("Weapon_StraightSword_01");
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
