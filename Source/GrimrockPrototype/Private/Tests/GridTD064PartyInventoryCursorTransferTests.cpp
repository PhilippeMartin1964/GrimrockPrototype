#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UGridPartyInventoryComponent* GridTD064CreateInventory()
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		Component->InitializeDefaultPartyIfNeeded();
		return Component;
	}

	UGridItemDefinitionAsset* GridTD064CreateItemDefinition(
		UObject* Outer, FName ItemDefinitionId, EGridItemType ItemType, bool bStackable, EGridEquipmentSlot CompatibleSlot = EGridEquipmentSlot::None)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = ItemDefinitionId;
		Definition->DisplayName = FText::FromName(ItemDefinitionId);
		Definition->ItemType = ItemType;
		Definition->Weight = 1.0f;
		Definition->bStackable = bStackable;
		Definition->MaxStackSize = bStackable ? 10 : 1;
		if (CompatibleSlot != EGridEquipmentSlot::None)
		{
			Definition->CompatibleEquipmentSlots.Add(CompatibleSlot);
		}
		return Definition;
	}

	FGridItemInstance GridTD064CreateItem(FName ItemDefinitionId, int32 Quantity = 1)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemDefinitionId;
		Item.DisplayName = FText::FromName(ItemDefinitionId);
		Item.Quantity = Quantity;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}

	bool GridTD064ValidateOwnership(FAutomationTestBase& Test, UGridPartyInventoryComponent* Component, const TCHAR* Stage)
	{
		FString OwnershipError;
		const bool bValid = Component && Component->ValidateInventoryOwnership(OwnershipError);
		Test.TestTrue(FString::Printf(TEXT("%s preserves exclusive ownership: %s"), Stage, *OwnershipError), bValid);
		return bValid;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD064PartyInventoryCursorTransferContractTest,
	"Grimrock.TechnicalDebt.TD06_4.PartyInventoryCursorTransfer.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD064PartyInventoryCursorTransferContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Component = GridTD064CreateInventory();
	if (!TestNotNull(TEXT("The characterized party inventory component is created"), Component))
	{
		return false;
	}

	const FName ReflectedFunctions[] = {
		TEXT("SetCursorItem"),
		TEXT("ClearCursorItem"),
		TEXT("HasCursorItem"),
		TEXT("TryTakeInventorySlotToCursor"),
		TEXT("TryTakeInventorySlotQuantityToCursor"),
		TEXT("TryPlaceCursorItemInCharacterInventorySlot"),
		TEXT("TryMoveCharacterInventorySlot"),
		TEXT("TryPlaceCursorItemInCharacterInventory"),
		TEXT("TryPlaceCursorItemInSelectedCharacterInventory"),
		TEXT("TryClearCursorToSelectedCharacterInventory"),
		TEXT("TryDropCursorItem"),
		TEXT("CanEquipCursorItemToCharacterSlot"),
		TEXT("TryEquipCursorItemToCharacterSlot"),
		TEXT("TryEquipCursorItemToSelectedCharacterSlot"),
		TEXT("TryTakeEquipmentSlotToCursor"),
		TEXT("TryTakeSelectedCharacterEquipmentSlotToCursor")
	};
	for (const FName FunctionName : ReflectedFunctions)
	{
		const UFunction* Function = Component->FindFunction(FunctionName);
		TestNotNull(FString::Printf(TEXT("%s remains reflected"), *FunctionName.ToString()), Function);
		TestTrue(FString::Printf(TEXT("%s remains BlueprintCallable"), *FunctionName.ToString()),
			Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	}

	TestFalse(TEXT("The cursor starts empty"), Component->HasCursorItem());
	TestFalse(TEXT("Clearing an already empty cursor returns false without mutation"), Component->ClearCursorItem());

	UGridItemDefinitionAsset* StackDefinition =
		GridTD064CreateItemDefinition(Component, TEXT("Potion_TD064"), EGridItemType::Potion, true);
	UGridItemDefinitionAsset* WeaponADefinition =
		GridTD064CreateItemDefinition(Component, TEXT("WeaponA_TD064"), EGridItemType::Weapon, false, EGridEquipmentSlot::MainHand);
	UGridItemDefinitionAsset* WeaponBDefinition =
		GridTD064CreateItemDefinition(Component, TEXT("WeaponB_TD064"), EGridItemType::Weapon, false, EGridEquipmentSlot::MainHand);
	if (!TestNotNull(TEXT("The stack definition is created"), StackDefinition) ||
		!TestNotNull(TEXT("Weapon A definition is created"), WeaponADefinition) ||
		!TestNotNull(TEXT("Weapon B definition is created"), WeaponBDefinition))
	{
		return false;
	}

	TestTrue(TEXT("The stack definition is registered"), Component->RegisterItemDefinition(StackDefinition));
	TestTrue(TEXT("Weapon A definition is registered"), Component->RegisterItemDefinition(WeaponADefinition));
	TestTrue(TEXT("Weapon B definition is registered"), Component->RegisterItemDefinition(WeaponBDefinition));

	const FGridItemInstance StackItem = GridTD064CreateItem(StackDefinition->ItemDefinitionId, 5);
	const FGridItemInstance WeaponA = GridTD064CreateItem(WeaponADefinition->ItemDefinitionId);
	const FGridItemInstance WeaponB = GridTD064CreateItem(WeaponBDefinition->ItemDefinitionId);
	const FGuid OriginalStackRuntimeId = StackItem.RuntimeObjectId;
	const FGuid WeaponARuntimeId = WeaponA.RuntimeObjectId;
	const FGuid WeaponBRuntimeId = WeaponB.RuntimeObjectId;

	TestTrue(TEXT("The five-unit stack enters inventory"), Component->AddItemToCharacterInventory(0, StackItem));
	TestTrue(TEXT("Weapon A enters inventory"), Component->AddItemToCharacterInventory(0, WeaponA));
	TestTrue(TEXT("Weapon B enters inventory"), Component->AddItemToCharacterInventory(0, WeaponB));
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Initial inventory setup")))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("The stack is stored in the first inventory slot"), Character.InventorySlots[0].Item.Quantity, 5);
	TestTrue(TEXT("The original stack keeps its runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == OriginalStackRuntimeId);
	TestTrue(TEXT("Weapon A occupies the second inventory slot"), Character.InventorySlots[1].Item.RuntimeObjectId == WeaponARuntimeId);
	TestTrue(TEXT("Weapon B occupies the third inventory slot"), Character.InventorySlots[2].Item.RuntimeObjectId == WeaponBRuntimeId);

	TestTrue(TEXT("Taking two units from a stack to the cursor succeeds"), Component->TryTakeInventorySlotQuantityToCursor(0, 0, 2));
	TestTrue(TEXT("The cursor becomes occupied after a stack split"), Component->HasCursorItem());
	const FGuid SplitRuntimeId = Component->GetCursorItem().RuntimeObjectId;
	TestEqual(TEXT("The cursor owns exactly the requested split quantity"), Component->GetCursorItem().Quantity, 2);
	TestEqual(TEXT("The source stack keeps the remaining quantity"), Character.InventorySlots[0].Item.Quantity, 3);
	TestTrue(TEXT("The source stack keeps its original runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == OriginalStackRuntimeId);
	TestTrue(TEXT("A split stack receives a distinct runtime identity"), SplitRuntimeId.IsValid() && SplitRuntimeId != OriginalStackRuntimeId);
	TestTrue(TEXT("A split item is normalized to cursor ownership"), Component->GetCursorItem().OwnerType == EGridItemOwnerType::Cursor);
	TestEqual(TEXT("A cursor item has no character owner"), Component->GetCursorItem().OwnerCharacterIndex, INDEX_NONE);
	TestTrue(TEXT("A cursor item has no equipment slot"), Component->GetCursorItem().EquipmentSlot == EGridEquipmentSlot::None);
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Inventory to cursor stack split")))
	{
		return false;
	}

	TestTrue(TEXT("The split cursor item can be placed into a chosen empty inventory slot"),
		Component->TryPlaceCursorItemInCharacterInventorySlot(0, 5));
	TestFalse(TEXT("Placing into an empty inventory slot clears the cursor"), Component->HasCursorItem());
	TestTrue(TEXT("The placed split item keeps its runtime identity"), Character.InventorySlots[5].Item.RuntimeObjectId == SplitRuntimeId);
	TestEqual(TEXT("The placed split item keeps its quantity"), Character.InventorySlots[5].Item.Quantity, 2);
	TestTrue(TEXT("The placed item is normalized to inventory ownership"),
		Character.InventorySlots[5].Item.OwnerType == EGridItemOwnerType::CharacterInventory);
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Cursor to empty inventory slot")))
	{
		return false;
	}

	TestTrue(TEXT("Taking a complete inventory slot to the cursor succeeds"), Component->TryTakeInventorySlotToCursor(0, 5));
	TestTrue(TEXT("Taking a complete slot clears the source inventory slot"), Character.InventorySlots[5].IsEmpty());
	TestTrue(TEXT("A complete transfer preserves the runtime identity"), Component->GetCursorItem().RuntimeObjectId == SplitRuntimeId);

	TestTrue(TEXT("Dropping the cursor onto an occupied inventory slot swaps atomically"),
		Component->TryPlaceCursorItemInCharacterInventorySlot(0, 1));
	TestTrue(TEXT("The former cursor item now occupies the target inventory slot"), Character.InventorySlots[1].Item.RuntimeObjectId == SplitRuntimeId);
	TestTrue(TEXT("The displaced inventory item becomes the cursor item"), Component->GetCursorItem().RuntimeObjectId == WeaponARuntimeId);
	TestTrue(TEXT("The swapped target is normalized to inventory ownership"),
		Character.InventorySlots[1].Item.OwnerType == EGridItemOwnerType::CharacterInventory);
	TestTrue(TEXT("The displaced item is normalized to cursor ownership"), Component->GetCursorItem().OwnerType == EGridItemOwnerType::Cursor);
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Cursor and inventory occupied-slot swap")))
	{
		return false;
	}

	TestTrue(TEXT("A cursor weapon is compatible with its declared main-hand slot"),
		Component->CanEquipCursorItemToCharacterSlot(0, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("A cursor weapon equips to an empty main hand"),
		Component->TryEquipCursorItemToCharacterSlot(0, EGridEquipmentSlot::MainHand));
	TestFalse(TEXT("Equipping into an empty equipment slot clears the cursor"), Component->HasCursorItem());

	FGridItemInstance EquippedItem;
	TestTrue(TEXT("The equipped weapon can be read"), Component->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem));
	TestTrue(TEXT("Cursor to equipment preserves the runtime identity"), EquippedItem.RuntimeObjectId == WeaponARuntimeId);
	TestTrue(TEXT("Equipment owns the transferred item"), EquippedItem.OwnerType == EGridItemOwnerType::EquipmentSlot);
	TestEqual(TEXT("The equipped item keeps character zero as owner"), EquippedItem.OwnerCharacterIndex, 0);
	TestTrue(TEXT("The equipped item records the main-hand slot"), EquippedItem.EquipmentSlot == EGridEquipmentSlot::MainHand);
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Cursor to empty equipment slot")))
	{
		return false;
	}

	TestTrue(TEXT("The selected-character equipment wrapper moves the main-hand item to the cursor"),
		Component->TryTakeSelectedCharacterEquipmentSlotToCursor(EGridEquipmentSlot::MainHand));
	TestFalse(TEXT("Equipment to cursor clears the source equipment slot"),
		Component->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Equipment to cursor preserves the runtime identity"), Component->GetCursorItem().RuntimeObjectId == WeaponARuntimeId);
	TestTrue(TEXT("Equipment to cursor normalizes cursor ownership"), Component->GetCursorItem().OwnerType == EGridItemOwnerType::Cursor);
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Equipment to cursor")))
	{
		return false;
	}

	TestTrue(TEXT("Weapon B can equip from inventory while Weapon A remains on the cursor"),
		Component->EquipItemFromInventorySlot(0, 2, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("The cursor still owns Weapon A after the independent equipment transaction"),
		Component->GetCursorItem().RuntimeObjectId == WeaponARuntimeId);
	TestTrue(TEXT("Weapon B occupies the main hand before the cursor/equipment swap"),
		Component->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem) && EquippedItem.RuntimeObjectId == WeaponBRuntimeId);

	TestTrue(TEXT("Equipping the cursor onto occupied equipment swaps atomically"),
		Component->TryEquipCursorItemToSelectedCharacterSlot(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Weapon A becomes the equipped item after the swap"),
		Component->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem) && EquippedItem.RuntimeObjectId == WeaponARuntimeId);
	TestTrue(TEXT("Weapon B becomes the cursor item after the swap"), Component->GetCursorItem().RuntimeObjectId == WeaponBRuntimeId);
	TestTrue(TEXT("The displaced equipment item is normalized to cursor ownership"), Component->GetCursorItem().OwnerType == EGridItemOwnerType::Cursor);
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Cursor and occupied-equipment swap")))
	{
		return false;
	}

	TestTrue(TEXT("The cursor item can be returned to the first free inventory slot"),
		Component->TryPlaceCursorItemInCharacterInventory(0));
	TestFalse(TEXT("Returning the cursor item to inventory clears the cursor"), Component->HasCursorItem());
	TestTrue(TEXT("The returned weapon occupies the first free slot"), Character.InventorySlots[2].Item.RuntimeObjectId == WeaponBRuntimeId);
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Cursor to first free inventory slot")))
	{
		return false;
	}

	TestTrue(TEXT("Moving an inventory item into an empty slot succeeds"),
		Component->TryMoveCharacterInventorySlot(0, 2, 3));
	TestTrue(TEXT("The inventory move clears its source slot"), Character.InventorySlots[2].IsEmpty());
	TestTrue(TEXT("The inventory move preserves the runtime identity"), Character.InventorySlots[3].Item.RuntimeObjectId == WeaponBRuntimeId);

	TestTrue(TEXT("Moving onto an occupied inventory slot swaps atomically"),
		Component->TryMoveCharacterInventorySlot(0, 3, 1));
	TestTrue(TEXT("The moved weapon reaches the occupied target"), Character.InventorySlots[1].Item.RuntimeObjectId == WeaponBRuntimeId);
	TestTrue(TEXT("The displaced split stack moves back to the source slot"), Character.InventorySlots[3].Item.RuntimeObjectId == SplitRuntimeId);
	if (!GridTD064ValidateOwnership(*this, Component, TEXT("Inventory slot move and swap")))
	{
		return false;
	}

	FGridItemInstance TemporaryCursorItem = GridTD064CreateItem(TEXT("TemporaryCursor_TD064"));
	const FGuid TemporaryRuntimeId = TemporaryCursorItem.RuntimeObjectId;
	TestTrue(TEXT("SetCursorItem accepts a standalone valid runtime item"), Component->SetCursorItem(TemporaryCursorItem));
	TestTrue(TEXT("SetCursorItem normalizes the item to cursor ownership"), Component->GetCursorItem().OwnerType == EGridItemOwnerType::Cursor);
	TestTrue(TEXT("SetCursorItem preserves runtime identity"), Component->GetCursorItem().RuntimeObjectId == TemporaryRuntimeId);
	TestTrue(TEXT("ClearCursorItem clears an occupied cursor"), Component->ClearCursorItem());
	TestFalse(TEXT("The cursor is empty after ClearCursorItem"), Component->HasCursorItem());
	TestFalse(TEXT("SetCursorItem rejects an invalid item"), Component->SetCursorItem(FGridItemInstance()));

	return true;
}

#endif
