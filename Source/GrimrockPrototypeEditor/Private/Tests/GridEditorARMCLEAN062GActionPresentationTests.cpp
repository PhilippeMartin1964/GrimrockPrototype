#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062GProductionActionsPresentationsTest,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionPresentationParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062GProductionActionsPresentationsTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production action registry configured"), Fixture.bReady)) return false;
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("ARM production manifest loads"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
        return false;
    int32 Active = 0, Passive = 0, BoundPresentations = 0;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        ++Active;
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("One ARM N1 resolves"),
            Fixture.Resolve(FName(Row.ItemDefinitionId), 1, Item))) continue;
        if (Row.IsPassive())
        {
            ++Passive;
            TestTrue(TEXT("Passive weapon never receives an attack presentation"),
                Item.AvailableActions.IsEmpty());
            continue;
        }
        if (!TestEqual(TEXT("Exactly one authored action binding"),
            Item.AvailableActions.Num(), 1)) continue;
        FGridResolvedBoundAction Bound;
        if (!TestTrue(TEXT("Bound action and presentation resolve together"),
            Fixture.Registry->ResolveBoundAttackForHand(
                Item.ItemId, 1, Item.AvailableActions[0].ActionId,
                EGridEquipmentSlot::MainHand, Bound))) continue;
        ++BoundPresentations;
        TestTrue(TEXT("Unique action presentation is valid"),
            Bound.IsValid() && !Bound.PresentationId.IsNone() &&
            Bound.Presentation.PresentationId == Bound.PresentationId);
        TestEqual(TEXT("Exact action ID survives canonical join"),
            Bound.Action.ActionId, FName(Row.AttackId));
        TestEqual(TEXT("Presentation identity is explicit"),
            Bound.PresentationId, Item.AvailableActions[0].PresentationId);
        TestTrue(TEXT("Finite authored attack motion timing"),
            FMath::IsFinite(Bound.Presentation.MotionDurationSeconds) &&
            FMath::IsFinite(Bound.Presentation.ImpactDelaySeconds));
    }
    TestEqual(TEXT("22 physical ARM identities"), Active, 22);
    TestEqual(TEXT("Three passive weapons have no attack presentation"), Passive, 3);
    TestEqual(TEXT("Nineteen action-to-presentation joins"), BoundPresentations, 19);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062GProjectionSafetyTest,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.PresentationProjectionSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062GProjectionSafetyTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Registered ItemSystem for presentation safety"), Fixture.bReady))
        return false;
    FGridResolvedBoundAction Out;
    TestFalse(TEXT("Unknown action cannot invent a presentation"),
        Fixture.Registry->ResolveBoundAction(
            TEXT("Weapon_Dagger_01"), 1, TEXT("Unbound_Presentation_Test"), Out));
    TestFalse(TEXT("Unknown ItemId cannot borrow a real action presentation"),
        Fixture.Registry->ResolveBoundAction(
            TEXT("Invalid_Weapon_Id"), 1, TEXT("Attack_Dagger_01"), Out));
    TestFalse(TEXT("Wrong physical weapon level cannot borrow a presentation"),
        Fixture.Registry->ResolveBoundAction(
            TEXT("Weapon_Dagger_01"), 21, TEXT("Attack_Dagger_01"), Out));
    TestTrue(TEXT("Rejected canonical binding cannot be mistaken for success"), !Out.IsValid());
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
