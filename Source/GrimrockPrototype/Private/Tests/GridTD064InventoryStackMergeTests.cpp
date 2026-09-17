#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD064InventoryStackMergeTest, "Grimrock.TechnicalDebt.TD06_4.InventoryStackMerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD064InventoryStackMergeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
	if (!TestNotNull(TEXT("The inventory component is created"), Component))
	{
		return false;
	}
	Component->InitializeDefaultPartyIfNeeded();

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Component);
	Definition->ItemDefinitionId = TEXT("Stone_TD064_Merge");
	Definition->DisplayName = FText::FromString(TEXT("Pierre TD06.4 merge"));
	Definition->ItemType = EGridItemType::Misc;
	Definition->Weight = 1.0f;
	Definition->bStackable = true;
	Definition->MaxStackSize = 10;
	if (!TestTrue(TEXT("The stackable definition is registered"), Component->RegisterItemDefinition(Definition)))
	{
		return false;
	}

	FGridItemInstance Stack;
	Stack.RuntimeObjectId = FGuid::NewGuid();
	Stack.ItemDefinitionId = Definition->ItemDefinitionId;
	Stack.DisplayName = Definition->DisplayName;
	Stack.Quantity = 5;
	Stack.OwnerType = EGridItemOwnerType::World;
	const FGuid OriginalRuntimeId = Stack.RuntimeObjectId;
	TestTrue(TEXT("The five-unit stack enters inventory"), Component->AddItemToCharacterInventory(0, Stack));

	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("The initial stack contains five units"), Character.InventorySlots[0].Item.Quantity, 5);

	TestTrue(TEXT("Ctrl-style split takes two units to the cursor"), Component->TryTakeInventorySlotQuantityToCursor(0, 0, 2));
	TestEqual(TEXT("The source stack keeps three units after split"), Character.InventorySlots[0].Item.Quantity, 3);
	TestEqual(TEXT("The cursor owns two split units"), Component->GetCursorItem().Quantity, 2);
	TestTrue(TEXT("Dropping the split cursor stack back onto its source merges it"), Component->TryPlaceCursorItemInCharacterInventorySlot(0, 0));
	TestFalse(TEXT("A complete cursor merge clears the cursor"), Component->HasCursorItem());
	TestEqual(TEXT("The reunited stack returns to five units"), Character.InventorySlots[0].Item.Quantity, 5);
	TestTrue(TEXT("The destination stack keeps its original runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == OriginalRuntimeId);

	TestTrue(TEXT("A second split can be placed in another inventory slot"), Component->TryTakeInventorySlotQuantityToCursor(0, 0, 2));
	TestTrue(TEXT("The split stack can be placed in slot one"), Component->TryPlaceCursorItemInCharacterInventorySlot(0, 1));
	TestEqual(TEXT("The source stack contains three units before slot merge"), Character.InventorySlots[0].Item.Quantity, 3);
	TestEqual(TEXT("The secondary stack contains two units before slot merge"), Character.InventorySlots[1].Item.Quantity, 2);
	TestTrue(TEXT("Moving a matching stack onto the original stack merges it"), Component->TryMoveCharacterInventorySlot(0, 1, 0));
	TestTrue(TEXT("A complete slot merge clears the source slot"), Character.InventorySlots[1].IsEmpty());
	TestEqual(TEXT("The direct slot merge restores five units"), Character.InventorySlots[0].Item.Quantity, 5);
	TestTrue(TEXT("The direct slot merge keeps the destination runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == OriginalRuntimeId);

	Character.InventorySlots[0].Item.Quantity = 9;
	FGridItemInstance Overflow;
	Overflow.RuntimeObjectId = FGuid::NewGuid();
	Overflow.ItemDefinitionId = Definition->ItemDefinitionId;
	Overflow.DisplayName = Definition->DisplayName;
	Overflow.Quantity = 3;
	Overflow.OwnerType = EGridItemOwnerType::World;
	TestTrue(TEXT("A three-unit matching stack can be put on the cursor"), Component->SetCursorItem(Overflow));
	TestTrue(TEXT("Merging into a nearly full stack transfers only available capacity"), Component->TryPlaceCursorItemInCharacterInventorySlot(0, 0));
	TestEqual(TEXT("The destination stack is capped at MaxStackSize"), Character.InventorySlots[0].Item.Quantity, 10);
	TestTrue(TEXT("The overflow remains on the cursor"), Component->HasCursorItem());
	TestEqual(TEXT("Exactly two overflow units remain on the cursor"), Component->GetCursorItem().Quantity, 2);
	TestFalse(TEXT("Dropping onto an already full matching stack is rejected without swapping"), Component->TryPlaceCursorItemInCharacterInventorySlot(0, 0));
	TestEqual(TEXT("The full destination remains unchanged"), Character.InventorySlots[0].Item.Quantity, 10);
	TestEqual(TEXT("The rejected full-stack merge preserves the cursor remainder"), Component->GetCursorItem().Quantity, 2);

	FString OwnershipError;
	TestTrue(TEXT("All merge paths preserve exclusive inventory ownership"), Component->ValidateInventoryOwnership(OwnershipError));
	return true;
}

#endif
