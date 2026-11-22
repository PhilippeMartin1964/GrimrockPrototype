#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062HGeneratedActionProjectionTest,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionGeneratedActionProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062HGeneratedActionProjectionTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Installed action system configured"), Fixture.bReady)) return false;
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Production ARM manifest reads"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
        return false;
    int32 Actions = 0, Passive = 0, Ranged = 0, Consuming = 0;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Canonical ARM N1 resolves"), Fixture.Resolve(FName(Row.ItemDefinitionId), 1, Item)))
            continue;
        if (Row.IsPassive())
        {
            ++Passive;
            TestTrue(TEXT("Passive ARM binds no attack"), Item.AvailableActions.IsEmpty());
            continue;
        }
        FGridResolvedBoundAction Bound;
        if (!TestTrue(TEXT("Authored attack is executable from MainHand"),
            Fixture.Registry->ResolveBoundAttackForHand(
                Item.ItemId, 1, FName(Row.AttackId), EGridEquipmentSlot::MainHand, Bound)))
            continue;
        ++Actions;
        TestEqual(TEXT("Canonical action ID matches original identity"),
            Bound.ActionId, FName(Row.AttackId));
        TestEqual(TEXT("Detailed action kind remains Attack"),
            Bound.Action.Kind, EGridActionKind::Attack);
        TestTrue(TEXT("Resource cost is never negative"),
            Bound.Action.ManaCost >= 0 && Bound.Action.SourceItemQuantityCost >= 0);
        if (Bound.Action.bRangedAttack) ++Ranged;
        if (Bound.Action.SourceItemQuantityCost > 0) ++Consuming;
        if (Row.IsThrowable())
        {
            TestTrue(TEXT("Physical projectile is also authored canonically"),
                Item.Weapon.Projectile.bEnabled);
        }
    }
    TestEqual(TEXT("Nineteen physically bound ARM actions"), Actions, 19);
    TestEqual(TEXT("Three passive items"), Passive, 3);
    TestTrue(TEXT("Ranged attacks still represented"), Ranged > 0);
    TestEqual(TEXT("Three dedicated thrown weapons consume physical source"),
        Consuming, 3);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062HRejectModifiedActionTest,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.RejectModifiedActionProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062HRejectModifiedActionTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Registered canonical action source"), Fixture.bReady)) return false;
    FGridResolvedItem Dagger;
    if (!TestTrue(TEXT("Real canonical dagger exists"),
        Fixture.Resolve(TEXT("Weapon_Dagger_01"), 1, Dagger))) return false;
    if (!TestTrue(TEXT("Dagger has one authored attack"), Dagger.AvailableActions.Num() == 1))
        return false;
    FGridResolvedBoundAction Bound;
    const FName RealAction = Dagger.AvailableActions[0].ActionId;
    TestTrue(TEXT("Real bound MainHand attack remains available"),
        Fixture.Registry->ResolveBoundAttackForHand(
            Dagger.ItemId, 1, RealAction, EGridEquipmentSlot::MainHand, Bound));
    TestFalse(TEXT("Unknown substituted action ID cannot be borrowed"),
        Fixture.Registry->ResolveBoundAttackForHand(
            Dagger.ItemId, 1, TEXT("Counterfeit_Dagger_Attack"), EGridEquipmentSlot::MainHand, Bound));
    TestFalse(TEXT("Invalid N21 level cannot borrow a real action"),
        Fixture.Registry->ResolveBoundAttackForHand(
            Dagger.ItemId, 21, RealAction, EGridEquipmentSlot::MainHand, Bound));
    TestFalse(TEXT("Empty action identity is never an attack"),
        Fixture.Registry->ResolveBoundAttackForHand(
            Dagger.ItemId, 1, NAME_None, EGridEquipmentSlot::MainHand, Bound));
    TestFalse(TEXT("Rejected action has no valid stale binding"), Bound.IsValid());
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
