#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062LProductionItems,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionItemIdentityProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062LProductionItems::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production canonical ItemSystem configured"), Fixture.bReady))
        return false;
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Production ARM identity manifest read"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
        return false;
    TSet<FName> IDs, Archetypes;
    int32 Checked = 0;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        const FName ItemId(Row.ItemDefinitionId), ArmId(Row.ArchetypeId);
        TestFalse(TEXT("No duplicate canonical ItemId"), IDs.Contains(ItemId));
        TestFalse(TEXT("No duplicate ARM archetype"), Archetypes.Contains(ArmId));
        IDs.Add(ItemId);
        Archetypes.Add(ArmId);
        FGridResolvedItem Resolved;
        if (!TestTrue(TEXT("Real N1 identity resolves"), Fixture.Resolve(ItemId, 1, Resolved)))
            continue;
        ++Checked;
        TestTrue(TEXT("Canonical weapon N1 carries its exact identity"),
            Resolved.bIsArmament && Resolved.ItemId == ItemId && Resolved.ArchetypeId == ArmId);
        TestTrue(TEXT("Display name exists independently of the old DA"),
            !Resolved.DisplayName.IsEmpty());
        TestTrue(TEXT("Weight is a finite authored nonnegative value"),
            FMath::IsFinite(Resolved.WeightKg) && Resolved.WeightKg >= 0.f);
        TestEqual(TEXT("Weapon is one physical item"), Resolved.MaxStack, 1);
        FGridResolvedItem Highest;
        TestTrue(TEXT("Identity survives the full range to N20"),
            Fixture.Resolve(ItemId, 20, Highest) && Highest.ArchetypeId == ArmId);
    }
    TestEqual(TEXT("22 canonical ARM identities"), Checked, 22);
    TestEqual(TEXT("22 unique item IDs"), IDs.Num(), 22);
    TestEqual(TEXT("22 unique archetypes"), Archetypes.Num(), 22);
    TArray<FName> All;
    Fixture.Registry->GetCanonicalItemIds(All);
    TestEqual(TEXT("Root remains the single 48-item source"), All.Num(), 48);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062LIdentitySafety,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ItemIdentityProjectionSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062LIdentitySafety::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical registry"), Fixture.bReady)) return false;
    FGridResolvedItem Rejected;
    TestFalse(TEXT("No empty ItemId"), Fixture.Registry->ResolveItem(NAME_None, 1, Rejected));
    TestFalse(TEXT("No unknown item synthesized from old DA naming"),
        Fixture.Registry->ResolveItem(TEXT("DA_Weapon_Dagger_01"), 1, Rejected));
    TestFalse(TEXT("N0 cannot masquerade as ARM"),
        Fixture.Registry->ResolveItem(TEXT("Weapon_Dagger_01"), 0, Rejected));
    TestFalse(TEXT("N21 cannot masquerade as ARM"),
        Fixture.Registry->ResolveItem(TEXT("Weapon_Dagger_01"), 21, Rejected));
    TestFalse(TEXT("NonARM N0 cannot gain a weapon progression"),
        Fixture.Registry->ResolveItem(TEXT("Item_Arrows_01"), 1, Rejected));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
