#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UGridPartyInventoryComponent* CreateInitializedInventoryComponent()
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		Component->InitializeDefaultPartyIfNeeded();
		return Component;
	}

	FGridItemInstance CreateTestItem(const TCHAR* DefinitionId, float Weight)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = FName(DefinitionId);
		Item.DisplayName = FText::FromString(DefinitionId);
		Item.Quantity = 1;
		Item.Weight = Weight;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyInventoryDefaultInitializationTest, "Grimrock.CharacterCreation.CC0.DefaultPartyInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyInventoryDefaultInitializationTest::RunTest(const FString& Parameters)
{
	UGridPartyInventoryComponent* Component = CreateInitializedInventoryComponent();

	TestNotNull(TEXT("Inventory component is created"), Component);
	if (!Component)
	{
		return false;
	}

	TestEqual(TEXT("One active character is created"), Component->GetActiveCharacterCount(), 1);
	TestEqual(TEXT("The first character is selected"), Component->GetSelectedCharacterIndex(), 0);
	TestEqual(TEXT("Equipment state stays aligned with active characters"), Component->PartyInventoryState.ActiveEquipment.Num(),
		Component->PartyInventoryState.ActiveCharacters.Num());

	if (!Component->PartyInventoryState.ActiveCharacters.IsValidIndex(0))
	{
		AddError(TEXT("The default character is missing."));
		return false;
	}

	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	TestTrue(TEXT("The default character has a stable runtime id"), Character.CharacterId.IsValid());
	TestEqual(TEXT("The default inventory contains 40 slots"), Character.InventorySlots.Num(), 40);
	TestTrue(TEXT("The cursor starts empty"), !Component->HasCursorItem());

	FString OwnershipError;
	TestTrue(FString::Printf(TEXT("Initial ownership is valid: %s"), *OwnershipError), Component->ValidateInventoryOwnership(OwnershipError));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyInventorySelectedCharacterPickupTest, "Grimrock.CharacterCreation.CC0.SelectedCharacterPickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyInventorySelectedCharacterPickupTest::RunTest(const FString& Parameters)
{
	UGridPartyInventoryComponent* Component = CreateInitializedInventoryComponent();
	if (!Component)
	{
		AddError(TEXT("Failed to create the inventory component."));
		return false;
	}

	const FGridItemInstance Item = CreateTestItem(TEXT("CC0_TestTorch"), 2.5f);
	TestTrue(TEXT("An item can be added to the selected character"), Component->AddItemToSelectedCharacterInventory(Item));

	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	const FGridInventorySlot& FirstSlot = Character.InventorySlots[0];
	TestTrue(TEXT("The first inventory slot is occupied"), !FirstSlot.IsEmpty());
	TestTrue(TEXT("The item is owned by a character inventory"), FirstSlot.Item.OwnerType == EGridItemOwnerType::CharacterInventory);
	TestEqual(TEXT("The selected character owns the item"), FirstSlot.Item.OwnerCharacterIndex, 0);
	TestTrue(TEXT("The owner guid is the character guid"), FirstSlot.Item.OwnerGuid == Character.CharacterId);

	FString OwnershipError;
	const bool bOwnershipValid = Component->ValidateInventoryOwnership(OwnershipError);
	TestTrue(FString::Printf(TEXT("Pickup ownership is valid: %s"), *OwnershipError), bOwnershipValid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyInventoryCarryWeightTest, "Grimrock.CharacterCreation.CC0.CarryWeightAndOverload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyInventoryCarryWeightTest::RunTest(const FString& Parameters)
{
	UGridPartyInventoryComponent* Component = CreateInitializedInventoryComponent();
	if (!Component)
	{
		AddError(TEXT("Failed to create the inventory component."));
		return false;
	}

	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.Attributes.Strength = 10;
	Component->RecalculateCharacterWeight(0);

	TestTrue(TEXT("Carry capacity uses Strength x 5"), FMath::IsNearlyEqual(Character.MaxCarryWeight, 50.0f));
	TestTrue(TEXT("An empty inventory is not overloaded"), !Character.IsOverloaded());

	const FGridItemInstance HeavyItem = CreateTestItem(TEXT("CC0_HeavyItem"), 51.0f);
	TestTrue(TEXT("The heavy item can still be stored"), Component->AddItemToSelectedCharacterInventory(HeavyItem));
	TestTrue(TEXT("Current weight includes the stored item"), FMath::IsNearlyEqual(Character.CurrentWeight, 51.0f));
	TestTrue(TEXT("Weight above capacity marks the character overloaded"), Character.IsOverloaded());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyInventoryExclusiveOwnershipTest, "Grimrock.CharacterCreation.CC0.ExclusiveOwnershipFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyInventoryExclusiveOwnershipTest::RunTest(const FString& Parameters)
{
	UGridPartyInventoryComponent* Component = CreateInitializedInventoryComponent();
	if (!Component)
	{
		AddError(TEXT("Failed to create the inventory component."));
		return false;
	}

	const FGridItemInstance Item = CreateTestItem(TEXT("CC0_TestWeapon"), 3.0f);
	const FGuid RuntimeObjectId = Item.RuntimeObjectId;
	TestTrue(TEXT("The test item enters inventory"), Component->AddItemToSelectedCharacterInventory(Item));

	FString OwnershipError;
	TestTrue(FString::Printf(TEXT("Inventory ownership is valid: %s"), *OwnershipError), Component->ValidateInventoryOwnership(OwnershipError));

	TestTrue(TEXT("The item moves from inventory to cursor"), Component->TryTakeInventorySlotToCursor(0, 0));
	TestTrue(TEXT("The source inventory slot is empty"), Component->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].IsEmpty());
	TestTrue(TEXT("The cursor contains the same runtime item"), Component->GetCursorItem().RuntimeObjectId == RuntimeObjectId);
	TestTrue(TEXT("The cursor is the exclusive owner"), Component->GetCursorItem().OwnerType == EGridItemOwnerType::Cursor);

	OwnershipError.Empty();
	TestTrue(FString::Printf(TEXT("Cursor ownership is valid: %s"), *OwnershipError), Component->ValidateInventoryOwnership(OwnershipError));

	TestTrue(
		TEXT("The cursor item equips to the selected character main hand"), Component->TryEquipCursorItemToSelectedCharacterSlot(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("The cursor is empty after equip"), !Component->HasCursorItem());

	FGridItemInstance EquippedItem;
	TestTrue(TEXT("The main hand contains an item"), Component->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem));
	TestTrue(TEXT("The equipped runtime item is unchanged"), EquippedItem.RuntimeObjectId == RuntimeObjectId);
	TestTrue(TEXT("Equipment is the exclusive owner"), EquippedItem.OwnerType == EGridItemOwnerType::EquipmentSlot);
	TestEqual(TEXT("The equipped item belongs to character 0"), EquippedItem.OwnerCharacterIndex, 0);
	TestTrue(TEXT("The equipment slot is MainHand"), EquippedItem.EquipmentSlot == EGridEquipmentSlot::MainHand);

	OwnershipError.Empty();
	TestTrue(FString::Printf(TEXT("Equipment ownership is valid: %s"), *OwnershipError), Component->ValidateInventoryOwnership(OwnershipError));

	return true;
}

#endif
