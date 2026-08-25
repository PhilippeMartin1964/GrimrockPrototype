#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
	FGridItemInstance MakeTD022EquippedItem(const FGuid& CharacterId, FName ItemDefinitionId, int32 Quantity, float UnitWeight)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemDefinitionId;
		Item.DisplayName = FText::FromName(ItemDefinitionId);
		Item.Quantity = Quantity;
		Item.Weight = UnitWeight;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = EGridEquipmentSlot::MainHand;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD022EquipmentWorldTransferContractTest, "Grimrock.TechnicalDebt.TD02_2.EquipmentWorldTransferContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD022EquipmentWorldTransferContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	TestNotNull(TEXT("The party inventory component is created"), Inventory);
	if (!Inventory)
	{
		return false;
	}

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("TD02 Transfer Tester"));
	Inventory->PartyInventoryState.ActiveCharacters = { Character };
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

	const FName ShurikenId(TEXT("TD02_Shuriken"));
	const FName OtherItemId(TEXT("TD02_Other"));
	FGridItemInstance& MainHand = Inventory->PartyInventoryState.ActiveEquipment[0].MainHand;
	MainHand = MakeTD022EquippedItem(Character.CharacterId, ShurikenId, 2, 0.5f);
	const FGuid OriginalStackRuntimeId = MainHand.RuntimeObjectId;

	FGridItemInstance ExtractedItem;
	TestFalse(TEXT("A world transfer is rejected from a non-hand equipment slot"),
		Inventory->TryExtractOneEquippedItemForWorldTransfer(0, EGridEquipmentSlot::Head, ShurikenId, ExtractedItem));
	TestEqual(TEXT("Rejected non-hand transfer leaves the stack untouched"), MainHand.Quantity, 2);

	TestFalse(TEXT("A world transfer is rejected when the expected definition differs"),
		Inventory->TryExtractOneEquippedItemForWorldTransfer(0, EGridEquipmentSlot::MainHand, OtherItemId, ExtractedItem));
	TestEqual(TEXT("Rejected definition mismatch leaves the stack untouched"), MainHand.Quantity, 2);

	TestTrue(TEXT("One unit can be extracted from a stacked hand item"),
		Inventory->TryExtractOneEquippedItemForWorldTransfer(0, EGridEquipmentSlot::MainHand, ShurikenId, ExtractedItem));
	TestEqual(TEXT("The equipped stack is decremented by one"), MainHand.Quantity, 1);
	TestEqual(TEXT("The equipped stack keeps its original runtime identity"), MainHand.RuntimeObjectId, OriginalStackRuntimeId);
	TestEqual(TEXT("The extracted world item contains one unit"), ExtractedItem.Quantity, 1);
	TestEqual(TEXT("The extracted item is owned by the world"), ExtractedItem.OwnerType, EGridItemOwnerType::World);
	TestFalse(TEXT("The extracted item has no owner guid"), ExtractedItem.OwnerGuid.IsValid());
	TestEqual(TEXT("The extracted item has no character owner index"), ExtractedItem.OwnerCharacterIndex, INDEX_NONE);
	TestEqual(TEXT("The extracted item has no equipment slot"), ExtractedItem.EquipmentSlot, EGridEquipmentSlot::None);
	TestNotEqual(TEXT("A unit split from a stack receives a distinct runtime identity"), ExtractedItem.RuntimeObjectId, OriginalStackRuntimeId);

	TestTrue(TEXT("The extracted unit can be rolled back into the matching hand stack"),
		Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, ExtractedItem));
	TestEqual(TEXT("Rollback restores the original stack quantity"), MainHand.Quantity, 2);
	TestEqual(TEXT("Rollback into a stack preserves the stack runtime identity"), MainHand.RuntimeObjectId, OriginalStackRuntimeId);

	FGridItemInstance FinalUnit;
	MainHand.Quantity = 1;
	const FGuid FinalUnitRuntimeId = MainHand.RuntimeObjectId;
	TestTrue(TEXT("The final equipped unit can be extracted"),
		Inventory->TryExtractOneEquippedItemForWorldTransfer(0, EGridEquipmentSlot::MainHand, ShurikenId, FinalUnit));
	TestFalse(TEXT("Extracting the final unit clears the equipment slot"), MainHand.IsValid());
	TestEqual(TEXT("A non-split final unit keeps its runtime identity in the world"), FinalUnit.RuntimeObjectId, FinalUnitRuntimeId);

	TestTrue(TEXT("A final extracted unit can be restored into the now-empty hand slot"),
		Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, FinalUnit));
	TestTrue(TEXT("Restoring into an empty hand slot recreates a valid equipped item"), MainHand.IsValid());
	TestEqual(TEXT("Restore into an empty slot keeps the world item's runtime identity"), MainHand.RuntimeObjectId, FinalUnitRuntimeId);
	TestEqual(TEXT("Restored item owner type is EquipmentSlot"), MainHand.OwnerType, EGridItemOwnerType::EquipmentSlot);
	TestEqual(TEXT("Restored item owner guid is the character"), MainHand.OwnerGuid, Character.CharacterId);
	TestEqual(TEXT("Restored item owner index is the character index"), MainHand.OwnerCharacterIndex, 0);
	TestEqual(TEXT("Restored item equipment slot is MainHand"), MainHand.EquipmentSlot, EGridEquipmentSlot::MainHand);

	FGridItemInstance OtherWorldItem = FinalUnit;
	OtherWorldItem.RuntimeObjectId = FGuid::NewGuid();
	OtherWorldItem.ItemDefinitionId = OtherItemId;
	OtherWorldItem.DisplayName = FText::FromName(OtherItemId);
	OtherWorldItem.OwnerType = EGridItemOwnerType::World;
	OtherWorldItem.OwnerGuid = FGuid();
	OtherWorldItem.OwnerCharacterIndex = INDEX_NONE;
	OtherWorldItem.EquipmentSlot = EGridEquipmentSlot::None;
	TestFalse(TEXT("Rollback refuses to merge a different item definition into an occupied hand slot"),
		Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, OtherWorldItem));
	TestEqual(TEXT("Rejected rollback leaves the equipped definition unchanged"), MainHand.ItemDefinitionId, ShurikenId);
	TestEqual(TEXT("Rejected rollback leaves the equipped quantity unchanged"), MainHand.Quantity, 1);

	return true;
}

#endif
