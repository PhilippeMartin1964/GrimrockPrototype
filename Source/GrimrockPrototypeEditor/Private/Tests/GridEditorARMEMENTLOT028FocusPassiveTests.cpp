#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "Runtime/GridPartyInventoryComponent.h"

// Real canonical focus physical armor and GUID-persistent reserve transaction.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT028Focus,
    "Grimrock.RPG.ARMEMENT.LOT02.FocusPassiveEquipmentAndPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENTLOT028Focus::RunTest(const FString&)
{
    FGridWeaponPipelineRow Kind;
    Kind.BuildKind = TEXT("FocusPassive");
    Kind.EquipmentSlots = TEXT("OffHand");
    TestTrue(TEXT("Focus is supported and remains passive"),
        Kind.IsSupportedBuildKind() && Kind.IsPassive() && Kind.IsFocus() && !Kind.IsShield());
    TestFalse(TEXT("Focus never becomes ranged or thrown"),
        Kind.UsesRangedAction() || Kind.IsThrowable());
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("World with native ItemSystem configured"), Fixture.Ready()))
        return false;
    FGridResolvedItem Focus;
    if (!TestTrue(TEXT("Actual Arcane Orb ARM070 N1 profile"),
        Fixture.Registry->ResolveItem(TEXT("Weapon_ArcaneOrb_01"), 1, Focus)))
        return false;
    TestTrue(TEXT("Focus is a real passive OffHand item"),
        Focus.bIsArmament && Focus.ArchetypeId == FName(TEXT("ARM070")) &&
        Focus.Weapon.Kind == EGridArmamentKind::Focus &&
        Focus.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand) &&
        !Focus.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand) &&
        !Focus.Weapon.DamageDice.bEnabled &&
        !Focus.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand) &&
        Focus.AvailableActions.IsEmpty());

    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("Actual party inventory"), Inventory)) return false;
    FGridItemInstance Item;
    Item.ItemDefinitionId = Focus.ItemId;
    Item.ItemLevel = 1;
    Item.RuntimeObjectId = FGuid::NewGuid();
    Item.Quantity = 1;
    if (!TestTrue(TEXT("Canonical focus materializes as physical instance"),
        Inventory->ApplyItemDefinitionToInstance(Item)) ||
        !TestTrue(TEXT("Added to bag without historical pointer"),
            Inventory->AddItemToCharacterInventory(0, Item)))
        return false;
    int32 BagIndex = INDEX_NONE;
    const TArray<FGridInventorySlot>& Bag =
        Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
    for (int32 I = 0; I < Bag.Num(); ++I)
    {
        if (!Bag[I].IsEmpty() && Bag[I].Item.RuntimeObjectId == Item.RuntimeObjectId)
        { BagIndex = I; break; }
    }
    if (!TestTrue(TEXT("Physical ArcaneOrb in bag"), BagIndex != INDEX_NONE) ||
        !TestTrue(TEXT("Real OffHand equip"), Inventory->EquipItemFromInventorySlot(
            0, BagIndex, EGridEquipmentSlot::OffHand)))
        return false;
    FGridInventoryCharacterSummary Summary;
    if (!TestTrue(TEXT("Equipped armor summary"), Inventory->GetCharacterSummary(0, Summary)))
        return false;
    TestEqual(TEXT("Canonical magical armor equipped"),
        Summary.EquipmentStatBonus.MagicalArmorBonus, Focus.Weapon.MagicalArmor);
    const int32 Base = Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor;
    TestEqual(TEXT("Equipped reserve adds canonical magical armor"),
        Inventory->GetEffectiveCurrentMagicalArmor(0), Base + Focus.Weapon.MagicalArmor);
    if (Focus.Weapon.MagicalArmor > 0)
    {
        const int32 Damage = FMath::Min(2, Focus.Weapon.MagicalArmor);
        TestEqual(TEXT("Magical armor is depleted from item before player"),
            Inventory->ApplyMagicalArmorDamage(0, Damage), Damage);
        TestEqual(TEXT("Physical OffHand stores exact depleted reserve"),
            Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentMagicalArmor,
            Damage);
        TestTrue(TEXT("Unequip retains exact remaining depletion"),
            Inventory->UnequipItemToInventory(0, EGridEquipmentSlot::OffHand));
        TestEqual(TEXT("Bag grants no equipped armor reserve"),
            Inventory->GetEffectiveCurrentMagicalArmor(0), Base);
        const FGridPartyInventoryState Snapshot = Inventory->PartyInventoryState;
        bool bFound = false;
        for (const FGridInventorySlot& Slot : Snapshot.ActiveCharacters[0].InventorySlots)
            if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Item.RuntimeObjectId)
                bFound = Slot.Item.AbsorbedEquipmentMagicalArmor == Damage &&
                    Slot.Item.ItemLevel == 1;
        TestTrue(TEXT("Snapshot preserves exact item GUID and depletion"), bFound);
    }
    return !HasAnyErrors();
}
#endif
