#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X60StonePlan,
    "Grimrock.RPG.ARM.CLEAN06_4X60.NativeStoneLootPlan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X60StonePlan::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    UGridMonsterDefinitionAsset* Goblin = LoadObject<UGridMonsterDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower"));
    UGridMonsterDefinitionAsset* Rat = LoadObject<UGridMonsterDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));
    if (!TestTrue(TEXT("Installed 48-item ItemSystem"), Fixture.bReady) ||
        !TestNotNull(TEXT("Real Goblin authoring"), Goblin) ||
        !TestNotNull(TEXT("Real Rat authoring"), Rat))
        return false;
    const bool bGoblinDirty = Goblin->GetOutermost()->IsDirty();
    const bool bRatDirty = Rat->GetOutermost()->IsDirty();
    if (!TestEqual(TEXT("Goblin has three loot rows"), Goblin->LootTable.Num(), 3) ||
        !TestEqual(TEXT("Rat has two loot rows"), Rat->LootTable.Num(), 2))
        return false;
    const FName Expected[] = {
        TEXT("GoblinKnife"), TEXT("Stone"), TEXT("EmptyVial"),
        TEXT("Item_RatMeat"), TEXT("Item_RatTooth")
    };
    for (int32 I = 0; I < UE_ARRAY_COUNT(Expected); ++I)
    {
        const FGridMonsterLootEntry& Row = I < 3 ?
            Goblin->LootTable[I] : Rat->LootTable[I-3];
        TestEqual(TEXT("Exactly canonical loot ItemId"), Row.GetResolvedItemDefinitionId(), Expected[I]);
        TestTrue(TEXT("Direct N0 loot row validates"), Row.IsValidDefinition());
        TestEqual(TEXT("Physical level is N0"), Row.ItemLevel, 0);
        FGridResolvedItem Real;
        TestTrue(TEXT("Item source lives in ItemSystem"),
            Fixture.Resolve(Row.GetResolvedItemDefinitionId(), 0, Real) &&
            !Real.bIsArmament);
        int32 Level = INDEX_NONE;
        TestTrue(TEXT("Loot resolver uses physical level N0"),
            FGridMonsterLootResolver::TryResolveItemLevel(Row, Level, Fixture.Registry.Get()) &&
            Level == 0);
    }
    TestEqual(TEXT("Approved Goblin monster XP"), Goblin->ExperienceReward, 125);
    TestEqual(TEXT("Approved Rat monster XP"), Rat->ExperienceReward, 500);
    TestTrue(TEXT("Goblin Stone chance remains one half"),
        FMath::IsNearlyEqual(Goblin->LootTable[1].DropChance, 0.5f));
    FGridMonsterLootEntry BadStone = Goblin->LootTable[1];
    BadStone.ItemLevel = 1;
    int32 BadLevel = INDEX_NONE;
    TestFalse(TEXT("Stone cannot be promoted to ARM N1"),
        FGridMonsterLootResolver::TryResolveItemLevel(BadStone, BadLevel, Fixture.Registry.Get()));
    TestNull(TEXT("Historical loot pointer field absent"),
        FGridMonsterLootEntry::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
    TestEqual(TEXT("Goblin native package unchanged"),
        Goblin->GetOutermost()->IsDirty(), bGoblinDirty);
    TestEqual(TEXT("Rat native package unchanged"),
        Rat->GetOutermost()->IsDirty(), bRatDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
