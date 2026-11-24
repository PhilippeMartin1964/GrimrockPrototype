#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "GridCanonicalWeightTestWorld.h"

namespace ARMEMENT0166Tests
{
    struct FSpec
    {
        const TCHAR* ItemId;
        const TCHAR* Archetype;
        const TCHAR* VisualId;
        EGridEquipmentSlot Slot;
        bool bAttack;
    };

    // These are canonical physical identities, never obsolete .uasset paths.
    static const FSpec Specs[] =
    {
        {TEXT("Weapon_Dagger_01"), TEXT("ARM001"), TEXT("VIS_Dagger_01"),
         EGridEquipmentSlot::MainHand, true},
        {TEXT("Weapon_ShortBow_01"), TEXT("ARM045"), TEXT("VIS_ShortBow_01"),
         EGridEquipmentSlot::MainHand, true},
        {TEXT("Weapon_RoundShield_01"), TEXT("ARM075"), TEXT("VIS_RoundShield_01"),
         EGridEquipmentSlot::OffHand, false}
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0166AuthoredDefinitionsTest,
    "Grimrock.RPG.ARMEMENT01.6.Items.AuthoredCataloguesAndMeshes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0166AuthoredDefinitionsTest::RunTest(const FString&)
{
    // Validate actual shipped catalogue projection. Retired DA_Weapon
    // definitions and the two old weapon catalogues are no longer inputs.
    FGridCanonicalWeightTestWorld Fixture({});
    const UGridItemSubsystem* Registry = Fixture.Scope.Registry;
    if (!TestNotNull(TEXT("Configured production ItemSystem"), Registry))
        return false;

    for (const ARMEMENT0166Tests::FSpec& Spec : ARMEMENT0166Tests::Specs)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Canonical pilot resolves at N1"),
            Registry->ResolveItem(FName(Spec.ItemId), 1, Item)))
        {
            continue;
        }
        TestTrue(TEXT("Resolved pilot is a valid ARM"), Item.IsValid() && Item.bIsArmament);
        TestEqual(TEXT("Stable canonical ItemId"), Item.ItemId, FName(Spec.ItemId));
        TestEqual(TEXT("Correct physical N1"), Item.ItemLevel, 1);
        TestEqual(TEXT("Correct canonical archetype"), Item.ArchetypeId, FName(Spec.Archetype));
        TestEqual(TEXT("Correct canonical visual identity"), Item.VisualId, FName(Spec.VisualId));
        TestTrue(TEXT("No weapon stack"), Item.MaxStack == 1);
        TestTrue(TEXT("N1 mass is finite and nonnegative"),
            FMath::IsFinite(Item.WeightKg) && Item.WeightKg >= 0.f);
        TestTrue(TEXT("Approved hand remains equippable"), Item.Weapon.CanEquipInHand(Spec.Slot));
        TestTrue(TEXT("Real world mesh loads from canonical visual catalogue"),
            Item.WorldMesh.LoadSynchronous() != nullptr);
        TestTrue(TEXT("Real held mesh loads from canonical visual catalogue"),
            Item.EquippedMesh.LoadSynchronous() != nullptr);
        TestTrue(TEXT("Canonical world transform has no invalid values"),
            !Item.WorldVisualTransform.ContainsNaN());
        TestEqual(TEXT("Attack permission is governed by canonical ARM type"),
            Item.Weapon.HasAttackSourceInHand(Spec.Slot), Spec.bAttack);
        TestEqual(TEXT("Shield versus attacking weapon distinction"),
            Item.Weapon.Kind == EGridArmamentKind::Shield, !Spec.bAttack);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0166EquipmentAttackTest,
    "Grimrock.RPG.ARMEMENT01.6.Items.EquipAndFixedLevelCombat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0166EquipmentAttackTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    const UGridItemSubsystem* Registry = Fixture.Scope.Registry;
    if (!TestNotNull(TEXT("Configured production ItemSystem"), Registry))
        return false;
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("World-bound inventory for canonical pilot weapons"), Inventory))
        return false;
    Inventory->InitializeDefaultPartyIfNeeded();

    for (const ARMEMENT0166Tests::FSpec& Spec : ARMEMENT0166Tests::Specs)
    {
        const FName ItemId(Spec.ItemId);
        FGridResolvedItem Resolved;
        if (!TestTrue(TEXT("Real pilot ARM resolves canonically at N1"),
                Registry->ResolveItem(ItemId, 1, Resolved)) ||
            !TestTrue(TEXT("Resolved item and archetype match the pilot manifest"),
                Resolved.IsValid() && Resolved.bIsArmament &&
                Resolved.ItemId == ItemId && Resolved.ItemLevel == 1 &&
                Resolved.ArchetypeId == FName(Spec.Archetype)))
            return false;

        TestTrue(TEXT("Resolved canonical mass is finite and nonnegative"),
            FMath::IsFinite(Resolved.WeightKg) && Resolved.WeightKg >= 0.f);
        TestTrue(TEXT("Canonical pilot supports its authored equipment hand"),
            Resolved.Weapon.CanEquipInHand(Spec.Slot));
        FGridItemInstance Instance;
        Instance.RuntimeObjectId = FGuid::NewGuid();
        Instance.ItemDefinitionId = ItemId;
        Instance.ItemLevel = 1;
        Instance.Quantity = 1;
        if (!TestTrue(TEXT("Canonical fixed-level instance materializes"),
                Inventory->ApplyItemDefinitionToInstance(Instance)))
            return false;
        TestEqual(TEXT("Physical item identity is stable"), Instance.ItemDefinitionId, ItemId);
        TestEqual(TEXT("Physical item keeps explicit N1"), Instance.ItemLevel, 1);
        TestNull(TEXT("No redundant Weight field on physical instance"),
            FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));

        TestEqual(TEXT("Canonical combat attack permission"),
            Resolved.Weapon.HasAttackSourceInHand(Spec.Slot), Spec.bAttack);
        if (Spec.bAttack)
        {
            TestTrue(TEXT("Pilot attack uses the canonical weapon kind"),
                Resolved.Weapon.Kind == EGridArmamentKind::Weapon);
            TestTrue(TEXT("Pilot N1 attack costs action points"),
                Resolved.Weapon.ActionPointCost > 0);
            TestTrue(TEXT("Pilot attack range is authored"),
                Resolved.Weapon.RangeCells >= 1);
        }
        else
        {
            TestTrue(TEXT("Shield is a passive canonical ARM"),
                Resolved.Weapon.Kind == EGridArmamentKind::Shield);
            TestFalse(TEXT("Passive shield does not attack from MainHand"),
                Resolved.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
            TestFalse(TEXT("Passive shield is not equippable MainHand"),
                Resolved.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand));
            TestEqual(TEXT("Round Shield N1 has five physical armor points"),
                Resolved.Weapon.PhysicalArmor, 5);
        }
    }
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
