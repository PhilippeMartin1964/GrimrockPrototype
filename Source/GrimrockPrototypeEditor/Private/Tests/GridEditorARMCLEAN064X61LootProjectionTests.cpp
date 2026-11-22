#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X61LootProjection,
    "Grimrock.RPG.ARM.CLEAN06_4X61.FourLootDefinitionProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X61LootProjection::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Installed canonical ItemSystem configured"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    UGridMonsterDefinitionAsset* Goblin = LoadObject<UGridMonsterDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower"));
    UGridMonsterDefinitionAsset* Rat = LoadObject<UGridMonsterDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));
    if (!TestNotNull(TEXT("Real ItemSystem root"), Root) ||
        !TestNotNull(TEXT("Real Goblin loot table"), Goblin) ||
        !TestNotNull(TEXT("Real Rat loot table"), Rat))
        return false;
    if (!TestEqual(TEXT("Goblin has exactly three native loot entries"), Goblin->LootTable.Num(), 3) ||
        !TestEqual(TEXT("Rat has exactly two native loot entries"), Rat->LootTable.Num(), 2))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bGoblinDirty = Goblin->GetOutermost()->IsDirty();
    const bool bRatDirty = Rat->GetOutermost()->IsDirty();
    TestTrue(TEXT("Exactly the installed 48 canonical definitions"),
        GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root));
    TestEqual(TEXT("Four global catalogue links remain installed"), 4,
        (Root->WeaponCatalogue ? 1 : 0) + (Root->WeaponVisualCatalogue ? 1 : 0) +
        (Root->ActionCatalogue ? 1 : 0) + (Root->ActionPresentationCatalogue ? 1 : 0));
    int32 NonArms = 0;
    for (const auto& Definition : Root->ItemDefinitions)
    {
        if (Cast<UGridNonArmamentDefinitionAsset>(Definition.Get())) ++NonArms;
    }
    TestEqual(TEXT("All 26 non-ARM definitions are native"), NonArms, 26);
    const FName CanonicalLoot[] =
    {
        TEXT("GoblinKnife"), TEXT("Stone"), TEXT("EmptyVial"),
        TEXT("Item_RatMeat"), TEXT("Item_RatTooth")
    };
    for (int32 I = 0; I < UE_ARRAY_COUNT(CanonicalLoot); ++I)
    {
        const FGridMonsterLootEntry& Loot =
            I < 3 ? Goblin->LootTable[I] : Rat->LootTable[I-3];
        FGridResolvedItem Item;
        TestTrue(TEXT("Real N0 loot source is not a historical definition"),
            Fixture.Resolve(CanonicalLoot[I], 0, Item) && !Item.bIsArmament);
        TestEqual(TEXT("Monster refers only to canonical ItemId"),
            Loot.ItemDefinitionId, CanonicalLoot[I]);
        TestEqual(TEXT("No counterfeit physical level"), Loot.ItemLevel, 0);
    }
    TestNull(TEXT("No serialized legacy loot pointer"),
        FGridMonsterLootEntry::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));

    // Both monster tables continue to use the same deterministic native
    // loot resolver. A private temporary copy cannot alter native packages.
    const TArray<FGridMonsterLootEntry> GoblinSnapshot = Goblin->LootTable;
    const TArray<FGridMonsterLootEntry> RatSnapshot = Rat->LootTable;
    for (int32 Seed = -64; Seed < 64; ++Seed)
    {
        const auto OriginalGoblin = FGridMonsterLootResolver::ResolveLoot(Goblin->LootTable, Seed);
        const auto CopiedGoblin = FGridMonsterLootResolver::ResolveLoot(GoblinSnapshot, Seed);
        const auto OriginalRat = FGridMonsterLootResolver::ResolveLoot(Rat->LootTable, Seed);
        const auto CopiedRat = FGridMonsterLootResolver::ResolveLoot(RatSnapshot, Seed);
        if (!TestEqual(TEXT("Goblin deterministic row count"), OriginalGoblin.Num(), CopiedGoblin.Num()) ||
            !TestEqual(TEXT("Rat deterministic row count"), OriginalRat.Num(), CopiedRat.Num()))
            continue;
        for (int32 I = 0; I < OriginalGoblin.Num(); ++I)
        {
            TestTrue(TEXT("Native Goblin deterministic roll identity"),
                OriginalGoblin[I].ItemDefinitionId == CopiedGoblin[I].ItemDefinitionId &&
                OriginalGoblin[I].bDropped == CopiedGoblin[I].bDropped &&
                OriginalGoblin[I].Quantity == CopiedGoblin[I].Quantity);
        }
        for (int32 I = 0; I < OriginalRat.Num(); ++I)
        {
            TestTrue(TEXT("Native Rat deterministic roll identity"),
                OriginalRat[I].ItemDefinitionId == CopiedRat[I].ItemDefinitionId &&
                OriginalRat[I].bDropped == CopiedRat[I].bDropped &&
                OriginalRat[I].Quantity == CopiedRat[I].Quantity);
        }
    }
    TestEqual(TEXT("ItemSystem remained read-only"), Root->GetOutermost()->IsDirty(), bRootDirty);
    TestEqual(TEXT("Goblin source remained read-only"), Goblin->GetOutermost()->IsDirty(), bGoblinDirty);
    TestEqual(TEXT("Rat source remained read-only"), Rat->GetOutermost()->IsDirty(), bRatDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
