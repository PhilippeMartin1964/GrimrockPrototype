#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT015
{
    FGridItemInstance Make(FName Id, int32 Level)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Id;
        Item.ItemLevel = Level;
        Item.Quantity = 1;
        return Item;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015CatalogueBindingTest,
    "Grimrock.RPG.ARMEMENT01.5.CatalogueBinding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT015CatalogueBindingTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real canonical registry configured"), Fixture.bReady)) return false;
    FGridResolvedItem Item;
    if (!TestTrue(TEXT("Fixed level six Dagger profile resolves"),
        Fixture.Resolve(TEXT("Weapon_Dagger_01"), 6, Item))) return false;
    TestTrue(TEXT("N6 has authored weapon identity, dice and physical hand"),
        Item.IsValid() && Item.bIsArmament &&
        Item.ArchetypeId == FName(TEXT("ARM001")) &&
        Item.Weapon.DamageDice.bEnabled &&
        Item.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand));
    FGridResolvedBoundAction Attack;
    TestTrue(TEXT("Real N6 MainHand action remains bound to Dagger"),
        Fixture.Registry->ResolveBoundAttackForHand(
            Item.ItemId, 6, TEXT("Attack_Dagger_01"), EGridEquipmentSlot::MainHand, Attack) &&
        Attack.IsValid());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015InstanceLevelTest,
    "Grimrock.RPG.ARMEMENT01.5.FixedLevelAndInstanceMetadata",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT015InstanceLevelTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("Real world-bound inventory"), Inventory)) return false;
    Inventory->InitializeDefaultPartyIfNeeded();
    FGridItemInstance Item = ARMEMENT015::Make(TEXT("Weapon_Dagger_01"), 6);
    const FGuid Guid = Item.RuntimeObjectId;
    if (!TestTrue(TEXT("Apply canonical N6 physical instance"),
        Inventory->ApplyItemDefinitionToInstance(Item))) return false;
    TestEqual(TEXT("N6 does not revert to old default N1"), Item.ItemLevel, 6);
    TestEqual(TEXT("Physical GUID untouched"), Item.RuntimeObjectId, Guid);
    TestNull(TEXT("No static weight cache on mutable instance"),
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));
    const FProperty* LevelProperty =
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
    TestTrue(TEXT("Physical level is nontransient SaveGame metadata"),
        LevelProperty && !LevelProperty->HasAnyPropertyFlags(CPF_Transient));
    FGridItemInstance SavedCopy = Item;
    TestTrue(TEXT("Rehydrate from the same immutable canonical N6"),
        Inventory->ApplyItemDefinitionToInstance(SavedCopy));
    TestEqual(TEXT("Rehydrated N6 unchanged"), SavedCopy.ItemLevel, 6);
    TestEqual(TEXT("Rehydrated GUID unchanged"), SavedCopy.RuntimeObjectId, Guid);
    FGridPartyInventoryState Snapshot;
    Snapshot.ActiveEquipment.SetNum(1);
    Snapshot.ActiveEquipment[0].MainHand = SavedCopy;
    Snapshot.CursorItem = SavedCopy;
    Snapshot.bHasCursorItem = true;
    const FGridPartyInventoryState Copy = Snapshot;
    TestEqual(TEXT("Equipment retains N6"), Copy.ActiveEquipment[0].MainHand.ItemLevel, 6);
    TestEqual(TEXT("Cursor retains N6"), Copy.CursorItem.ItemLevel, 6);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015CombatTest,
    "Grimrock.RPG.ARMEMENT01.5.EquippedCombatProfile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT015CombatTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real canonical numeric profiles"), Fixture.bReady)) return false;
    FGridResolvedItem N7, N11;
    if (!TestTrue(TEXT("One ItemId independently resolves at N7 and N11"),
        Fixture.Resolve(TEXT("Weapon_Dagger_01"), 7, N7) &&
        Fixture.Resolve(TEXT("Weapon_Dagger_01"), 11, N11))) return false;
    TestTrue(TEXT("Both levels keep real attack dice"),
        N7.Weapon.DamageDice.bEnabled && N11.Weapon.DamageDice.bEnabled);
    TestTrue(TEXT("Both levels retain valid numeric range and scaling"),
        N7.Weapon.RangeCells > 0 && N11.Weapon.RangeCells > 0 &&
        N7.Weapon.ScalingAttribute == N11.Weapon.ScalingAttribute);
    FGridResolvedBoundAction A7, A11;
    TestTrue(TEXT("N7 MainHand attack is explicitly bound"),
        Fixture.Registry->ResolveBoundAttackForHand(
            N7.ItemId, 7, TEXT("Attack_Dagger_01"), EGridEquipmentSlot::MainHand, A7));
    TestTrue(TEXT("N11 MainHand attack is explicitly bound"),
        Fixture.Registry->ResolveBoundAttackForHand(
            N11.ItemId, 11, TEXT("Attack_Dagger_01"), EGridEquipmentSlot::MainHand, A11));
    TestTrue(TEXT("Shared ActionId does not copy or override weapon-level damage"),
        A7.ActionId == A11.ActionId &&
        N7.ItemLevel == 7 && N11.ItemLevel == 11);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015PassiveAndFailureTest,
    "Grimrock.RPG.ARMEMENT01.5.PassiveAndMissingReference",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT015PassiveAndFailureTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("One configured ItemSystem"), Fixture.bReady)) return false;
    FGridResolvedItem Focus, Rejected;
    if (!TestTrue(TEXT("N6 passive Orb exists"),
        Fixture.Resolve(TEXT("Weapon_ArcaneOrb_01"), 6, Focus))) return false;
    TestTrue(TEXT("Passive focus has OffHand permission without any attack"),
        Focus.bIsArmament && !Focus.Weapon.DamageDice.bEnabled &&
        Focus.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand) &&
        !Focus.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand) &&
        Focus.AvailableActions.IsEmpty());
    FGridResolvedBoundAction Attack;
    TestFalse(TEXT("Passive focus cannot borrow Dagger action"),
        Fixture.Registry->ResolveBoundAttackForHand(
            Focus.ItemId, 6, TEXT("Attack_Dagger_01"), EGridEquipmentSlot::OffHand, Attack));
    TestFalse(TEXT("No invented ARM099 archetype"),
        Fixture.Registry->ResolveItem(TEXT("Weapon_ARM099"), 6, Rejected));
    TestFalse(TEXT("N21 invalid, not clamped to N20"),
        Fixture.Registry->ResolveItem(Focus.ItemId, 21, Rejected));
    TestFalse(TEXT("N0 invalid for ARM, not promoted"),
        Fixture.Registry->ResolveItem(Focus.ItemId, 0, Rejected));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015InventoryEquipmentTest,
    "Grimrock.RPG.ARMEMENT01.5.InventoryEquipmentTransfer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT015InventoryEquipmentTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("Native world inventory"), Inventory)) return false;
    Inventory->InitializeDefaultPartyIfNeeded();
    FGridItemInstance Item = ARMEMENT015::Make(TEXT("Weapon_Dagger_01"), 9);
    if (!TestTrue(TEXT("N9 real physical item is valid"),
        Inventory->ApplyItemDefinitionToInstance(Item)) ||
        !TestTrue(TEXT("N9 item enters bag"),
            Inventory->AddItemToCharacterInventory(0, Item)))
        return false;
    int32 Index = INDEX_NONE;
    const auto& Bag = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
    for (int32 I = 0; I < Bag.Num(); ++I)
        if (!Bag[I].IsEmpty() && Bag[I].Item.RuntimeObjectId == Item.RuntimeObjectId)
        { Index = I; break; }
    if (!TestTrue(TEXT("N9 physical bag slot exists"), Index != INDEX_NONE))
        return false;
    const FGridItemInstance FromBag = Bag[Index].Item;
    TestEqual(TEXT("Bag preserves N9"), FromBag.ItemLevel, 9);
    if (!TestTrue(TEXT("Canonical MainHand equipped via real transaction"),
        Inventory->EquipItemFromInventorySlot(
            0, Index, EGridEquipmentSlot::MainHand))) return false;
    FGridItemInstance Held;
    if (!TestTrue(TEXT("Equipped N9 physical instance available"),
        Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, Held)))
        return false;
    TestEqual(TEXT("Equipment preserves original level"), Held.ItemLevel, 9);
    TestEqual(TEXT("Equipment preserves original GUID"), Held.RuntimeObjectId, Item.RuntimeObjectId);
    FGridResolvedItem N9;
    TestTrue(TEXT("Equipped damage resolves via physical N9, never character level"),
        Fixture.Scope.Registry->ResolveItem(Held.ItemDefinitionId, Held.ItemLevel, N9) &&
        N9.ItemLevel == 9 && N9.Weapon.DamageDice.bEnabled);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015LegacyTest,
    "Grimrock.RPG.ARMEMENT01.5.LegacyUnchanged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT015LegacyTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Native ItemSystem is the only definition authority"),
        Fixture.bReady)) return false;
    FGridResolvedItem N0, Wrong;
    if (!TestTrue(TEXT("N0 non-ARM remains valid after retirement"),
        Fixture.Resolve(TEXT("Stone"), 0, N0))) return false;
    TestTrue(TEXT("Non-ARM N0 is not silently promoted"),
        !N0.bIsArmament && N0.ItemLevel == 0);
    TestFalse(TEXT("Unknown historical definition does not acquire combat data"),
        Fixture.Registry->ResolveItem(TEXT("ARM015_Legacy"), 0, Wrong));
    TestFalse(TEXT("ARM weapon cannot borrow a Legacy N0 level"),
        Fixture.Registry->ResolveItem(TEXT("Weapon_Dagger_01"), 0, Wrong));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTCLEAN02LevelArmorAuthorityTest,
    "Grimrock.RPG.ARMEMENT.CLEAN02.FixedLevelArmorAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENTCLEAN02LevelArmorAuthorityTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real numeric armor authority configured"), Fixture.bReady))
        return false;
    FGridResolvedItem N1, N20, Invalid;
    if (!TestTrue(TEXT("N1 and N20 RoundShield exist in ItemSystem"),
        Fixture.Resolve(TEXT("Weapon_RoundShield_01"), 1, N1) &&
        Fixture.Resolve(TEXT("Weapon_RoundShield_01"), 20, N20)))
        return false;
    TestTrue(TEXT("Both levels remain passive and retain valid armor"),
        !N1.Weapon.DamageDice.bEnabled && !N20.Weapon.DamageDice.bEnabled &&
        N1.Weapon.PhysicalArmor >= 0 && N20.Weapon.PhysicalArmor >= 0 &&
        N1.Weapon.MagicalArmor >= 0 && N20.Weapon.MagicalArmor >= 0);
    FGridCanonicalArmorReserveView AtOne, AtTwenty;
    TestTrue(TEXT("Physical N1 reserve is computed, not an item cache"),
        FGridCanonicalArmorReserveResolver::TryCompute(N1.Weapon, 0, 0, AtOne));
    TestTrue(TEXT("Physical N20 reserve is computed from level growth"),
        FGridCanonicalArmorReserveResolver::TryCompute(N20.Weapon, 0, 0, AtTwenty));
    TestEqual(TEXT("Actual N1 physical armor"), AtOne.NominalPhysicalArmor, N1.Weapon.PhysicalArmor);
    TestEqual(TEXT("Actual N20 physical armor"), AtTwenty.NominalPhysicalArmor, N20.Weapon.PhysicalArmor);
    TestEqual(TEXT("Actual N1 magical armor"), AtOne.NominalMagicalArmor, N1.Weapon.MagicalArmor);
    TestEqual(TEXT("Actual N20 magical armor"), AtTwenty.NominalMagicalArmor, N20.Weapon.MagicalArmor);
    TestFalse(TEXT("Invalid N21 cannot infer N1 armor"),
        Fixture.Registry->ResolveItem(N1.ItemId, 21, Invalid));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
