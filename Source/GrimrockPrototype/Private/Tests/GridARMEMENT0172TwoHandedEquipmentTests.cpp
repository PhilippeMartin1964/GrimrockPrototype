#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace ARMEMENT0172
{
    UGridPartyInventoryComponent* CreateInventory()
    {
        UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
        Inventory->InitializeDefaultPartyIfNeeded();
        return Inventory;
    }

    UGridItemDefinitionAsset* AddDefinition(
        UGridPartyInventoryComponent* Inventory, FName Id, EGridItemHandUsage Usage, EGridEquipmentSlot CompatibleSlot)
    {
        UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>(Inventory);
        Item->ItemDefinitionId = Id;
        Item->DisplayName = FText::FromName(Id);
        Item->ItemType = EGridItemType::Weapon;
        Item->Weight = 1.0f;
        Item->HandUsage = Usage;
        Item->CompatibleEquipmentSlots.Add(CompatibleSlot);
        Inventory->RegisterItemDefinition(Item);
        return Item;
    }

    FGridItemInstance CreateItem(FName Id)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Id;
        Item.DisplayName = FText::FromName(Id);
        Item.Quantity = 1;
        Item.Weight = 1.0f;
        return Item;
    }

    int32 CountPhysical(const UGridPartyInventoryComponent* Inventory, const FGuid& Id)
    {
        int32 Count = 0;
        for (const FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Id) ++Count;
        }
        const FGridCharacterEquipmentState& Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
        if (Equipment.MainHand.IsValid() && Equipment.MainHand.RuntimeObjectId == Id) ++Count;
        if (Equipment.OffHand.IsValid() && Equipment.OffHand.RuntimeObjectId == Id) ++Count;
        if (Inventory->HasCursorItem() && Inventory->GetCursorItem().RuntimeObjectId == Id) ++Count;
        return Count;
    }

    bool Setup(UGridPartyInventoryComponent* Inventory, UGridItemDefinitionAsset*& Sword,
        UGridItemDefinitionAsset*& Shield, UGridItemDefinitionAsset*& Bow)
    {
        Sword = AddDefinition(Inventory, TEXT("ARM0172_Sword"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::MainHand);
        Shield = AddDefinition(Inventory, TEXT("ARM0172_Shield"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::OffHand);
        Bow = AddDefinition(Inventory, TEXT("ARM0172_Bow"), EGridItemHandUsage::TwoHanded, EGridEquipmentSlot::MainHand);
        Bow->EquipmentStatBonus.StrengthBonus = 7;
        return Sword && Shield && Bow &&
            Inventory->FindItemDefinition(Sword->ItemDefinitionId) == Sword &&
            Inventory->FindItemDefinition(Shield->ItemDefinitionId) == Shield &&
            Inventory->FindItemDefinition(Bow->ItemDefinitionId) == Bow;
    }

    bool EquipPair(UGridPartyInventoryComponent* Inventory, FGridItemInstance& SwordItem, FGridItemInstance& ShieldItem)
    {
        SwordItem = CreateItem(TEXT("ARM0172_Sword"));
        ShieldItem = CreateItem(TEXT("ARM0172_Shield"));
        return Inventory->AddItemToCharacterInventory(0, SwordItem) &&
            Inventory->AddItemToCharacterInventory(0, ShieldItem) &&
            Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand) &&
            Inventory->EquipItemFromInventorySlot(0, 1, EGridEquipmentSlot::OffHand);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0172InventoryTest,
    "Grimrock.RPG.ARMEMENT01.7.TwoHands.InventoryEquipAndOccupation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0172InventoryTest::RunTest(const FString&)
{
    UGridPartyInventoryComponent* Inventory = ARMEMENT0172::CreateInventory();
    UGridItemDefinitionAsset *Sword = nullptr, *Shield = nullptr, *Bow = nullptr;
    if (!TestTrue(TEXT("Hand definitions registered"), ARMEMENT0172::Setup(Inventory, Sword, Shield, Bow))) return false;
    TestTrue(TEXT("Bow accepts MainHand"), Bow->CanEquipToSlot(EGridEquipmentSlot::MainHand));
    TestTrue(TEXT("Bow accepts OffHand placement request"), Bow->CanEquipToSlot(EGridEquipmentSlot::OffHand));

    FGridItemInstance SwordItem, ShieldItem;
    if (!TestTrue(TEXT("Initial sword and shield equipped"), ARMEMENT0172::EquipPair(Inventory, SwordItem, ShieldItem))) return false;
    const FGridItemInstance BowItem = ARMEMENT0172::CreateItem(Bow->ItemDefinitionId);
    if (!TestTrue(TEXT("Bow put in bag"), Inventory->AddItemToCharacterInventory(0, BowItem))) return false;
    if (!TestTrue(TEXT("Bow displaces both hands atomically"), Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::OffHand))) return false;

    const FGridCharacterEquipmentState& Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
    TestEqual(TEXT("Physical bow resides only in MainHand"), Equipment.MainHand.RuntimeObjectId, BowItem.RuntimeObjectId);
    TestFalse(TEXT("OffHand contains no duplicate serialized item"), Equipment.OffHand.IsValid());
    TestTrue(TEXT("Main hand is occupied"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
    TestTrue(TEXT("Off hand is virtually occupied"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
    TestTrue(TEXT("Two-handed predicate is true"), Inventory->IsTwoHandedEquipped(0));

    FGridItemInstance VisualMain, VisualOff;
    TestTrue(TEXT("Main hand reads bow"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, VisualMain));
    TestTrue(TEXT("Off hand reads the same bow"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::OffHand, VisualOff));
    TestEqual(TEXT("Both UI slots resolve to one runtime object"), VisualMain.RuntimeObjectId, VisualOff.RuntimeObjectId);
    TestEqual(TEXT("OffHand projection identifies MainHand as physical owner"), VisualOff.EquipmentSlot, EGridEquipmentSlot::MainHand);
    TestEqual(TEXT("Bonuses apply only once"), Inventory->ComputeCharacterEquipmentStatBonus(0).StrengthBonus, 7);
    TestEqual(TEXT("Bow physical count is exactly one"), ARMEMENT0172::CountPhysical(Inventory, BowItem.RuntimeObjectId), 1);
    TestEqual(TEXT("Sword physical count is exactly one"), ARMEMENT0172::CountPhysical(Inventory, SwordItem.RuntimeObjectId), 1);
    TestEqual(TEXT("Shield physical count is exactly one"), ARMEMENT0172::CountPhysical(Inventory, ShieldItem.RuntimeObjectId), 1);
    TestFalse(TEXT("Sword and shield were not left equipped"), Equipment.OffHand.IsValid());

    const FGridPartyInventoryState Saved = Inventory->PartyInventoryState;
    TestTrue(TEXT("Serialized MainHand keeps the bow"), Saved.ActiveEquipment[0].MainHand.RuntimeObjectId == BowItem.RuntimeObjectId);
    TestFalse(TEXT("Serialized OffHand stays empty"), Saved.ActiveEquipment[0].OffHand.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0172CursorTest,
    "Grimrock.RPG.ARMEMENT01.7.TwoHands.CursorAndUnequip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0172CursorTest::RunTest(const FString&)
{
    UGridPartyInventoryComponent* Inventory = ARMEMENT0172::CreateInventory();
    UGridItemDefinitionAsset *Sword = nullptr, *Shield = nullptr, *Bow = nullptr;
    if (!TestTrue(TEXT("Definitions registered"), ARMEMENT0172::Setup(Inventory, Sword, Shield, Bow))) return false;
    FGridItemInstance SwordItem, ShieldItem;
    if (!TestTrue(TEXT("Initial sword/shield equip"), ARMEMENT0172::EquipPair(Inventory, SwordItem, ShieldItem))) return false;

    const FGridItemInstance BowItem = ARMEMENT0172::CreateItem(Bow->ItemDefinitionId);
    TestTrue(TEXT("Cursor accepts bow"), Inventory->SetCursorItem(BowItem));
    TestTrue(TEXT("Two-hand cursor equip succeeds"), Inventory->TryEquipCursorItemToCharacterSlot(0, EGridEquipmentSlot::OffHand));
    TestFalse(TEXT("Cursor emptied"), Inventory->HasCursorItem());
    TestTrue(TEXT("Two hands occupied"), Inventory->IsTwoHandedEquipped(0));
    TestEqual(TEXT("Bow not duplicated"), ARMEMENT0172::CountPhysical(Inventory, BowItem.RuntimeObjectId), 1);
    TestEqual(TEXT("Sword returned to inventory"), ARMEMENT0172::CountPhysical(Inventory, SwordItem.RuntimeObjectId), 1);
    TestEqual(TEXT("Shield returned to inventory"), ARMEMENT0172::CountPhysical(Inventory, ShieldItem.RuntimeObjectId), 1);

    TestTrue(TEXT("Take from virtual OffHand succeeds"), Inventory->TryTakeEquipmentSlotToCursor(0, EGridEquipmentSlot::OffHand));
    TestFalse(TEXT("MainHand freed"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
    TestFalse(TEXT("OffHand freed"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
    TestEqual(TEXT("Cursor contains original bow"), Inventory->GetCursorItem().RuntimeObjectId, BowItem.RuntimeObjectId);

    TestTrue(TEXT("Re-equip from cursor"), Inventory->TryEquipCursorItemToCharacterSlot(0, EGridEquipmentSlot::MainHand));
    TestTrue(TEXT("Unequip from virtual OffHand goes to inventory"), Inventory->UnequipItemToInventory(0, EGridEquipmentSlot::OffHand));
    TestFalse(TEXT("Both hands free after unequip"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
    TestFalse(TEXT("Both hands free after unequip"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
    TestEqual(TEXT("Bow preserved in inventory exactly once"), ARMEMENT0172::CountPhysical(Inventory, BowItem.RuntimeObjectId), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0172CapacityTest,
    "Grimrock.RPG.ARMEMENT01.7.TwoHands.FullBagRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0172CapacityTest::RunTest(const FString&)
{
    UGridPartyInventoryComponent* Inventory = ARMEMENT0172::CreateInventory();
    UGridItemDefinitionAsset *Sword = nullptr, *Shield = nullptr, *Bow = nullptr;
    if (!TestTrue(TEXT("Definitions registered"), ARMEMENT0172::Setup(Inventory, Sword, Shield, Bow))) return false;
    FGridItemInstance SwordItem, ShieldItem;
    if (!TestTrue(TEXT("Initial sword/shield equip"), ARMEMENT0172::EquipPair(Inventory, SwordItem, ShieldItem))) return false;
    const FGridItemInstance BowItem = ARMEMENT0172::CreateItem(Bow->ItemDefinitionId);

    // Only source bag slot will be freed: two displaced hand items cannot fit.
    FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
    Character.InventorySlots.SetNum(1);
    Character.InventorySlots[0].bOccupied = true;
    Character.InventorySlots[0].Item = BowItem;
    Character.InventorySlots[0].Item.OwnerType = EGridItemOwnerType::CharacterInventory;
    Character.InventorySlots[0].Item.OwnerGuid = Character.CharacterId;
    Character.InventorySlots[0].Item.OwnerCharacterIndex = 0;
    AddExpectedError(TEXT("InventoryFullForDisplacedItems"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Inventory equip fails without two free destinations"), Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand));
    TestEqual(TEXT("Sword preserved"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId, SwordItem.RuntimeObjectId);
    TestEqual(TEXT("Shield preserved"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.RuntimeObjectId, ShieldItem.RuntimeObjectId);
    TestEqual(TEXT("Bow stays in bag"), Character.InventorySlots[0].Item.RuntimeObjectId, BowItem.RuntimeObjectId);

    // No free inventory slots while cursor carries a two-handed bow.
    Character.InventorySlots[0] = FGridInventorySlot();
    TestTrue(TEXT("Cursor takes bow"), Inventory->SetCursorItem(BowItem));
    Character.InventorySlots[0].bOccupied = true;
    Character.InventorySlots[0].Item = ARMEMENT0172::CreateItem(TEXT("ARM0172_Filler"));
    AddExpectedError(TEXT("InventoryFullForDisplacedItems"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Cursor equip fails without bag capacity"), Inventory->TryEquipCursorItemToCharacterSlot(0, EGridEquipmentSlot::OffHand));
    TestTrue(TEXT("Cursor unchanged after rejection"), Inventory->HasCursorItem() && Inventory->GetCursorItem().RuntimeObjectId == BowItem.RuntimeObjectId);
    TestEqual(TEXT("Sword still equipped"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId, SwordItem.RuntimeObjectId);
    TestEqual(TEXT("Shield still equipped"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.RuntimeObjectId, ShieldItem.RuntimeObjectId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0172OneHandReplacementTest,
    "Grimrock.RPG.ARMEMENT01.7.TwoHands.ReplaceWithOneHand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0172OneHandReplacementTest::RunTest(const FString&)
{
    UGridPartyInventoryComponent* Inventory = ARMEMENT0172::CreateInventory();
    UGridItemDefinitionAsset *Sword = nullptr, *Shield = nullptr, *Bow = nullptr;
    if (!TestTrue(TEXT("Definitions registered"), ARMEMENT0172::Setup(Inventory, Sword, Shield, Bow))) return false;
    const FGridItemInstance BowItem = ARMEMENT0172::CreateItem(Bow->ItemDefinitionId);
    const FGridItemInstance SwordItem = ARMEMENT0172::CreateItem(Sword->ItemDefinitionId);
    TestTrue(TEXT("Bow added to inventory"), Inventory->AddItemToCharacterInventory(0, BowItem));
    TestTrue(TEXT("Bow initially equipped"), Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand));
    TestTrue(TEXT("Sword added to inventory"), Inventory->AddItemToCharacterInventory(0, SwordItem));
    TestTrue(TEXT("Sword equips OffHand and releases bow"), Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::OffHand));
    TestFalse(TEXT("Main hand freed"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.IsValid());
    TestEqual(TEXT("OffHand contains sword"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.RuntimeObjectId, SwordItem.RuntimeObjectId);
    TestFalse(TEXT("Two-handed reservation ended"), Inventory->IsTwoHandedEquipped(0));
    TestEqual(TEXT("Bow returned to inventory once"), ARMEMENT0172::CountPhysical(Inventory, BowItem.RuntimeObjectId), 1);
    TestEqual(TEXT("Sword owned once"), ARMEMENT0172::CountPhysical(Inventory, SwordItem.RuntimeObjectId), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0172WorldTransferTest,
    "Grimrock.RPG.ARMEMENT01.7.TwoHands.WorldTransferAndRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0172WorldTransferTest::RunTest(const FString&)
{
    UGridPartyInventoryComponent* Inventory = ARMEMENT0172::CreateInventory();
    UGridItemDefinitionAsset *Sword = nullptr, *Shield = nullptr, *Bow = nullptr;
    if (!TestTrue(TEXT("Definitions registered"), ARMEMENT0172::Setup(Inventory, Sword, Shield, Bow))) return false;
    const FGridItemInstance BowItem = ARMEMENT0172::CreateItem(Bow->ItemDefinitionId);
    TestTrue(TEXT("Bow added to inventory"), Inventory->AddItemToCharacterInventory(0, BowItem));
    TestTrue(TEXT("Bow equipped"), Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand));

    FGridItemInstance WorldItem;
    TestTrue(TEXT("Extract from virtual OffHand takes physical MainHand"), Inventory->TryExtractOneEquippedItemForWorldTransfer(
        0, EGridEquipmentSlot::OffHand, BowItem.ItemDefinitionId, WorldItem));
    TestEqual(TEXT("Extracted world item identity"), WorldItem.RuntimeObjectId, BowItem.RuntimeObjectId);
    TestFalse(TEXT("Main/Off cleared after extraction"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
    TestTrue(TEXT("Restore into either hand uses MainHand as physical owner"), Inventory->TryRestoreExtractedItemToEquipment(
        0, EGridEquipmentSlot::OffHand, WorldItem));
    TestEqual(TEXT("Restored bow in MainHand"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId, BowItem.RuntimeObjectId);
    TestFalse(TEXT("OffHand remains a virtual reservation"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0172ProductionPilotTest,
    "Grimrock.RPG.ARMEMENT01.7.TwoHands.ProductionShortBowDefinition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0172ProductionPilotTest::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Bow = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.DA_Weapon_ShortBow_01"));
    if (!TestNotNull(TEXT("Production ShortBow pilot exists"), Bow)) return false;
    TestEqual(TEXT("ShortBow authoring is two-handed"), Bow->GetEffectiveHandUsage(), EGridItemHandUsage::TwoHanded);
    TestTrue(TEXT("ShortBow can be placed from either hand slot"), Bow->CanEquipToSlot(EGridEquipmentSlot::MainHand) && Bow->CanEquipToSlot(EGridEquipmentSlot::OffHand));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
