#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemTransferService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"

namespace
{
	UGridItemDefinitionAsset* MakeDefinition(UGridPartyInventoryComponent* Inventory, FName Id, bool bStackable, int32 MaxStackSize)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Inventory);
		Definition->ItemDefinitionId = Id;
		Definition->DisplayName = FText::FromName(Id);
		Definition->ItemType = EGridItemType::Misc;
		Definition->bStackable = bStackable;
		Definition->MaxStackSize = MaxStackSize;
		Inventory->RegisterItemDefinition(Definition);
		return Definition;
	}

	void PrepareTwoCharacters(UGridPartyInventoryComponent* Inventory, int32 SourceSlots = 4, int32 TargetSlots = 4)
	{
		Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

		FGridCharacterInventoryState& Source = Inventory->PartyInventoryState.ActiveCharacters[0];
		Source.CharacterId = FGuid::NewGuid();
		Source.DisplayName = FText::FromString(TEXT("Source"));
		Source.InventorySlots.SetNum(SourceSlots);

		FGridCharacterInventoryState& Target = Inventory->PartyInventoryState.ActiveCharacters[1];
		Target.CharacterId = FGuid::NewGuid();
		Target.DisplayName = FText::FromString(TEXT("Target"));
		Target.InventorySlots.SetNum(TargetSlots);
	}

	FGridItemInstance MakeOwnedItem(
		const UGridItemDefinitionAsset* Definition, const FGridCharacterInventoryState& Owner, int32 CharacterIndex, int32 Quantity)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.DisplayName = Definition->DisplayName;
		Item.Quantity = Quantity;
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerGuid = Owner.CharacterId;
		Item.OwnerCharacterIndex = CharacterIndex;
		Item.EquipmentSlot = EGridEquipmentSlot::None;
		return Item;
	}

	void PutItem(FGridCharacterInventoryState& Character, int32 CharacterIndex, int32 SlotIndex, const UGridItemDefinitionAsset* Definition, int32 Quantity)
	{
		FGridItemInstance Item = MakeOwnedItem(Definition, Character, CharacterIndex, Quantity);
		Character.InventorySlots[SlotIndex].bOccupied = true;
		Character.InventorySlots[SlotIndex].Item = Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory02PartyTransferServiceTest, "Grimrock.UI.Inventory02.PartyTransferService",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory02PartyTransferServiceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	TestNotNull(TEXT("Party inventory exists"), Inventory);
	if (!Inventory)
	{
		return false;
	}

	PrepareTwoCharacters(Inventory);
	UGridItemDefinitionAsset* StackDefinition = MakeDefinition(Inventory, TEXT("UIINV02_Stack"), true, 10);
	UGridItemDefinitionAsset* OtherDefinition = MakeDefinition(Inventory, TEXT("UIINV02_Other"), false, 1);

	FGridCharacterInventoryState& Source = Inventory->PartyInventoryState.ActiveCharacters[0];
	FGridCharacterInventoryState& Target = Inventory->PartyInventoryState.ActiveCharacters[1];

	PutItem(Source, 0, 0, StackDefinition, 3);
	const FGuid WholeSourceId = Source.InventorySlots[0].Item.RuntimeObjectId;
	const FGridItemTransferResult WholeResult = UGridItemTransferService::TransferInventorySlotToCharacter(Inventory, 0, 0, 1, 0);
	TestTrue(TEXT("Whole stack transfer succeeds"), WholeResult.bSuccess);
	TestTrue(TEXT("Whole transfer clears source slot"), Source.InventorySlots[0].IsEmpty());
	TestFalse(TEXT("Whole transfer creates target item"), Target.InventorySlots[0].IsEmpty());
	TestEqual(TEXT("Whole transfer keeps quantity"), Target.InventorySlots[0].Item.Quantity, 3);
	TestTrue(TEXT("Whole transfer preserves runtime identity"), Target.InventorySlots[0].Item.RuntimeObjectId == WholeSourceId);
	TestEqual(TEXT("Whole transfer rewrites owner character"), Target.InventorySlots[0].Item.OwnerCharacterIndex, 1);
	TestTrue(TEXT("Whole transfer rewrites owner guid"), Target.InventorySlots[0].Item.OwnerGuid == Target.CharacterId);

	for (FGridInventorySlot& Slot : Source.InventorySlots) Slot = FGridInventorySlot();
	for (FGridInventorySlot& Slot : Target.InventorySlots) Slot = FGridInventorySlot();

	PutItem(Source, 0, 0, StackDefinition, 3);
	const FGuid SplitSourceId = Source.InventorySlots[0].Item.RuntimeObjectId;
	const FGridItemTransferResult SplitResult = UGridItemTransferService::TransferInventorySlotToCharacter(Inventory, 0, 0, 1, 1);
	TestTrue(TEXT("Split transfer succeeds"), SplitResult.bSuccess);
	TestEqual(TEXT("Split leaves source remainder"), Source.InventorySlots[0].Item.Quantity, 2);
	TestTrue(TEXT("Split preserves source remainder identity"), Source.InventorySlots[0].Item.RuntimeObjectId == SplitSourceId);
	TestEqual(TEXT("Split target receives one"), Target.InventorySlots[0].Item.Quantity, 1);
	TestTrue(TEXT("Split target receives distinct runtime identity"), Target.InventorySlots[0].Item.RuntimeObjectId != SplitSourceId);
	TestEqual(TEXT("Split target owner is rewritten"), Target.InventorySlots[0].Item.OwnerCharacterIndex, 1);

	for (FGridInventorySlot& Slot : Source.InventorySlots) Slot = FGridInventorySlot();
	Target.InventorySlots.SetNum(1);
	Target.InventorySlots[0] = FGridInventorySlot();
	PutItem(Source, 0, 0, OtherDefinition, 1);
	PutItem(Target, 1, 0, OtherDefinition, 1);
	const FGuid RejectedSourceId = Source.InventorySlots[0].Item.RuntimeObjectId;
	const FGridItemTransferResult RejectResult = UGridItemTransferService::TransferInventorySlotToCharacter(Inventory, 0, 0, 1, 0);
	TestFalse(TEXT("Full target rejects transfer"), RejectResult.bSuccess);
	TestEqual(TEXT("Full target reports inventory full"), RejectResult.Result, EGridItemTransferResult::InventoryFull);
	TestFalse(TEXT("Rejected transfer preserves source"), Source.InventorySlots[0].IsEmpty());
	TestTrue(TEXT("Rejected transfer preserves source identity"), Source.InventorySlots[0].Item.RuntimeObjectId == RejectedSourceId);

	FString OwnershipError;
	TestTrue(TEXT("Ownership remains valid after transfers"), Inventory->ValidateInventoryOwnership(OwnershipError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory02PortraitDropRoutingTest, "Grimrock.UI.Inventory02.PortraitDropRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory02PortraitDropRoutingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Widget = NewObject<UGridInventoryWidget>();
	TestNotNull(TEXT("Party inventory exists"), Inventory);
	TestNotNull(TEXT("Inventory widget exists"), Widget);
	if (!Inventory || !Widget)
	{
		return false;
	}

	PrepareTwoCharacters(Inventory);
	UGridItemDefinitionAsset* Definition = MakeDefinition(Inventory, TEXT("UIINV02_Drag"), true, 10);
	FGridCharacterInventoryState& Source = Inventory->PartyInventoryState.ActiveCharacters[0];
	FGridCharacterInventoryState& Target = Inventory->PartyInventoryState.ActiveCharacters[1];
	PutItem(Source, 0, 0, Definition, 2);

	Widget->InventoryComponent = Inventory;

	UGridInventorySlotWidget* SourceSlotWidget = NewObject<UGridInventorySlotWidget>();
	SourceSlotWidget->SetOwnerInventoryWidget(Widget);
	SourceSlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);
	SourceSlotWidget->SetItem(Source.InventorySlots[0].Item);
	UGridInventoryDragDropOperation* Operation = SourceSlotWidget->CreateDragDropOperation();
	TestNotNull(TEXT("Real slot drag operation is created"), Operation);
	if (!Operation)
	{
		return false;
	}
	TestEqual(TEXT("Drag captures selected source character"), Operation->SourceCharacterIndex, 0);

	TestTrue(TEXT("Portrait drop transfers the complete stack"), Widget->HandlePartyMemberItemDrop(Operation, 1));
	TestEqual(TEXT("Portrait drop keeps source character selected"), Inventory->GetSelectedCharacterIndex(), 0);
	TestTrue(TEXT("Portrait drop clears the source stack"), Source.InventorySlots[0].IsEmpty());
	TestEqual(TEXT("Portrait drop moves both items to target"), Target.InventorySlots[0].Item.Quantity, 2);

	PutItem(Source, 0, 0, Definition, 1);
	UGridInventoryDragDropOperation* StaleOperation = NewObject<UGridInventoryDragDropOperation>();
	FGridItemInstance StaleItem = Source.InventorySlots[0].Item;
	StaleOperation->InitializeFromSlot(EGridInventoryUiSlotType::Inventory, 0, StaleItem);
	StaleOperation->SourceCharacterIndex = 0;
	StaleOperation->SourceRuntimeObjectId = FGuid::NewGuid();

	TestFalse(TEXT("Stale drag identity is rejected"), Widget->HandlePartyMemberItemDrop(StaleOperation, 1));
	TestEqual(TEXT("Rejected stale drag leaves source unchanged"), Source.InventorySlots[0].Item.Quantity, 1);
	TestEqual(TEXT("Rejected stale drag leaves target unchanged"), Target.InventorySlots[0].Item.Quantity, 2);

	TestFalse(TEXT("Dropping on the same portrait is not a transfer"), Widget->HandlePartyMemberItemDrop(Operation, 0));
	return true;
}

#endif
