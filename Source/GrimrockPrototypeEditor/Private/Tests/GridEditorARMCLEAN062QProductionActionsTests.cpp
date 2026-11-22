#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Permanent read-only regression of the installed canonical ItemSystem.
// The obsolete transient production candidate builders were one-shot tools.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062QProductionActions,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionCatalogues22",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062QProductionActions::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Single production ItemSystem configured"), Registry.bReady)) return false;
    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("22 ARM manifest identities read"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
        return false;
    int32 Offensive = 0, OffHandSources = 0, Passive = 0;
    TSet<FName> ActionIds;
    for (const FGridWeaponPipelineRow& Row : Manifest)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Real N1 source resolves"),
            Registry.Resolve(FName(Row.ItemDefinitionId), 1, Item))) continue;
        if (Row.IsPassive())
        {
            ++Passive;
            TestTrue(TEXT("Passive ARM must not invent attacks"), Item.AvailableActions.IsEmpty());
            continue;
        }
        TestEqual(TEXT("Exactly one authored ARM action binding"), Item.AvailableActions.Num(), 1);
        if (Item.AvailableActions.Num() != 1) continue;
        ++Offensive;
        const FName ActionId = Item.AvailableActions[0].ActionId;
        TestEqual(TEXT("ARM action keeps same ID"), ActionId, FName(Row.AttackId));
        ActionIds.Add(ActionId);
        FGridResolvedBoundAction Action;
        TestTrue(TEXT("MainHand bound action and presentation are valid"),
            Registry.Registry->ResolveBoundAttackForHand(
                Item.ItemId, 1, ActionId, EGridEquipmentSlot::MainHand, Action) &&
            Action.IsValid() && Action.Action.Kind == EGridActionKind::Attack &&
            !Action.PresentationId.IsNone());
        if (Item.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand))
        {
            ++OffHandSources;
            FGridResolvedBoundAction Off;
            TestTrue(TEXT("Explicit OffHand attack source resolves"),
                Registry.Registry->ResolveBoundAttackForHand(
                    Item.ItemId, 1, ActionId, EGridEquipmentSlot::OffHand, Off));
        }
    }
    TestEqual(TEXT("19 physically offensive ARM"), Offensive, 19);
    TestEqual(TEXT("Three passive ARM with no attack"), Passive, 3);
    TestEqual(TEXT("Two explicitly approved OffHand attack sources"), OffHandSources, 2);
    // Identical action identifiers can be shared across weapons, but each
    // physical weapon must carry its own explicit binding.
    TestTrue(TEXT("No ARM action group is empty"), !ActionIds.IsEmpty());
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
