#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UGridPartyInventoryComponent* GridTD066CreateInventory()
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		Component->InitializeDefaultPartyIfNeeded();
		return Component;
	}

	UGridItemDefinitionAsset* GridTD066CreateItemDefinition(
		UObject* Outer, FName ItemDefinitionId, EGridItemType ItemType, EGridEquipmentSlot CompatibleSlot, float Weight = 1.0f, bool bStackable = false)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = ItemDefinitionId;
		Definition->DisplayName = FText::FromName(ItemDefinitionId);
		Definition->ItemType = ItemType;
		Definition->Weight = Weight;
		Definition->bStackable = bStackable;
		Definition->MaxStackSize = bStackable ? 10 : 1;
		if (CompatibleSlot != EGridEquipmentSlot::None)
		{
			Definition->CompatibleEquipmentSlots.Add(CompatibleSlot);
		}
		return Definition;
	}

	FGridItemInstance GridTD066CreateItem(FName ItemDefinitionId, int32 Quantity = 1, float Weight = 1.0f)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemDefinitionId;
		Item.DisplayName = FText::FromName(ItemDefinitionId);
		Item.Quantity = Quantity;
		Item.Weight = Weight;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}

	bool GridTD066ValidateOwnership(FAutomationTestBase& Test, UGridPartyInventoryComponent* Component, const TCHAR* Stage)
	{
		FString OwnershipError;
		const bool bValid = Component && Component->ValidateInventoryOwnership(OwnershipError);
		Test.TestTrue(FString::Printf(TEXT("%s preserves exclusive ownership: %s"), Stage, *OwnershipError), bValid);
		return bValid;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD066PartyInventoryEquipmentCoreContractTest,
	"Grimrock.TechnicalDebt.TD06_6.PartyInventoryEquipmentCore.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD066PartyInventoryEquipmentCoreContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Component = GridTD066CreateInventory();
	if (!TestNotNull(TEXT("The characterized party inventory component is created"), Component))
	{
		return false;
	}

	const FName ReflectedFunctions[] = {
		TEXT("CanEquipItemToSlot"),
		TEXT("EquipItemFromInventorySlot"),
		TEXT("UnequipItemToInventory"),
		TEXT("GetEquippedItem"),
		TEXT("IsEquipmentSlotOccupied"),
		TEXT("ComputeCharacterEquipmentStatBonus"),
		TEXT("ComputeCharacterEquipmentResistances")
	};
	for (const FName FunctionName : ReflectedFunctions)
	{
		const UFunction* Function = Component->FindFunction(FunctionName);
		TestNotNull(FString::Printf(TEXT("%s remains reflected"), *FunctionName.ToString()), Function);
		TestTrue(FString::Printf(TEXT("%s remains BlueprintCallable"), *FunctionName.ToString()),
			Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	}

	TestFalse(TEXT("An invalid character cannot equip"),
		Component->CanEquipItemToSlot(INDEX_NONE, GridTD066CreateItem(TEXT("InvalidCharacter_TD066")), EGridEquipmentSlot::MainHand));
	TestFalse(TEXT("An invalid runtime item cannot equip"), Component->CanEquipItemToSlot(0, FGridItemInstance(), EGridEquipmentSlot::MainHand));

	const FGridItemInstance FallbackItem = GridTD066CreateItem(TEXT("UnregisteredFallback_TD066"));
	TestTrue(TEXT("The historical no-definition fallback accepts a valid item in a supported slot"),
		Component->CanEquipItemToSlot(0, FallbackItem, EGridEquipmentSlot::MainHand));
	TestFalse(TEXT("The historical no-definition fallback still rejects the None equipment slot"),
		Component->CanEquipItemToSlot(0, FallbackItem, EGridEquipmentSlot::None));

	UGridItemDefinitionAsset* SwordDefinition =
		GridTD066CreateItemDefinition(Component, TEXT("Sword_TD066"), EGridItemType::Weapon, EGridEquipmentSlot::MainHand, 2.0f);
	UGridItemDefinitionAsset* AxeDefinition =
		GridTD066CreateItemDefinition(Component, TEXT("Axe_TD066"), EGridItemType::Weapon, EGridEquipmentSlot::MainHand, 3.0f);
	UGridItemDefinitionAsset* ShieldDefinition =
		GridTD066CreateItemDefinition(Component, TEXT("Shield_TD066"), EGridItemType::Shield, EGridEquipmentSlot::OffHand, 4.0f);
	if (!TestNotNull(TEXT("Sword definition is created"), SwordDefinition) || !TestNotNull(TEXT("Axe definition is created"), AxeDefinition) ||
		!TestNotNull(TEXT("Shield definition is created"), ShieldDefinition))
	{
		return false;
	}

	SwordDefinition->EquipmentStatBonus.StrengthBonus = 2;
	SwordDefinition->EquipmentStatBonus.ArmorBonus = 1;
	SwordDefinition->EquipmentResistanceBonus.FireResistance = 3;

	AxeDefinition->EquipmentStatBonus.StrengthBonus = 5;
	AxeDefinition->EquipmentResistanceBonus.LightningResistance = 2;

	ShieldDefinition->EquipmentStatBonus.ConstitutionBonus = 1;
	ShieldDefinition->EquipmentStatBonus.CarryWeightBonus = 4.0f;
	ShieldDefinition->EquipmentResistanceBonus.PhysicalResistance = 2;
	ShieldDefinition->EquipmentResistanceBonus.FireResistance = 1;

	TestTrue(TEXT("Sword definition is registered"), Component->RegisterItemDefinition(SwordDefinition));
	TestTrue(TEXT("Axe definition is registered"), Component->RegisterItemDefinition(AxeDefinition));
	TestTrue(TEXT("Shield definition is registered"), Component->RegisterItemDefinition(ShieldDefinition));

	const FGridItemInstance Sword = GridTD066CreateItem(SwordDefinition->ItemDefinitionId, 1, SwordDefinition->Weight);
	const FGridItemInstance Axe = GridTD066CreateItem(AxeDefinition->ItemDefinitionId, 1, AxeDefinition->Weight);
	const FGridItemInstance Shield = GridTD066CreateItem(ShieldDefinition->ItemDefinitionId, 1, ShieldDefinition->Weight);
	const FGuid SwordRuntimeId = Sword.RuntimeObjectId;
	const FGuid AxeRuntimeId = Axe.RuntimeObjectId;
	const FGuid ShieldRuntimeId = Shield.RuntimeObjectId;

	TestTrue(TEXT("Sword enters inventory"), Component->AddItemToCharacterInventory(0, Sword));
	TestTrue(TEXT("Axe enters inventory"), Component->AddItemToCharacterInventory(0, Axe));
	TestTrue(TEXT("Shield enters inventory"), Component->AddItemToCharacterInventory(0, Shield));
	if (!GridTD066ValidateOwnership(*this, Component, TEXT("Initial equipment setup")))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	TestTrue(TEXT("Registered compatibility accepts the sword in MainHand"),
		Component->CanEquipItemToSlot(0, Character.InventorySlots[0].Item, EGridEquipmentSlot::MainHand));
	TestFalse(TEXT("Registered compatibility rejects the sword in OffHand"),
		Component->CanEquipItemToSlot(0, Character.InventorySlots[0].Item, EGridEquipmentSlot::OffHand));

	TestTrue(TEXT("Equipping from inventory into an empty MainHand succeeds"),
		Component->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("The source inventory slot is cleared after an empty-slot equip"), Character.InventorySlots[0].IsEmpty());
	TestTrue(TEXT("MainHand reports occupied after equip"), Component->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));

	FGridItemInstance EquippedItem;
	TestTrue(TEXT("The equipped sword can be read"), Component->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem));
	TestTrue(TEXT("Equip preserves the sword runtime identity"), EquippedItem.RuntimeObjectId == SwordRuntimeId);
	TestTrue(TEXT("Equip normalizes equipment ownership"), EquippedItem.OwnerType == EGridItemOwnerType::EquipmentSlot);
	TestEqual(TEXT("Equip records the owning character"), EquippedItem.OwnerCharacterIndex, 0);
	TestTrue(TEXT("Equip records the actual equipment slot"), EquippedItem.EquipmentSlot == EGridEquipmentSlot::MainHand);

	AddExpectedError(TEXT("GridInventory Equip Failed Character=0 Slot=MainHand Reason=UnsupportedSlot"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("An incompatible Shield -> MainHand equip is rejected"),
		Component->EquipItemFromInventorySlot(0, 2, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("A rejected incompatible equip leaves the shield in its source slot"),
		Character.InventorySlots[2].Item.RuntimeObjectId == ShieldRuntimeId);
	TestTrue(TEXT("A rejected incompatible equip leaves the existing MainHand unchanged"),
		Component->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem) && EquippedItem.RuntimeObjectId == SwordRuntimeId);

	TestTrue(TEXT("The shield equips to its declared OffHand slot"), Component->EquipItemFromInventorySlot(0, 2, EGridEquipmentSlot::OffHand));
	TestTrue(TEXT("OffHand reports occupied after shield equip"), Component->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));

	const FGridEquipmentStatBonus InitialBonus = Component->ComputeCharacterEquipmentStatBonus(0);
	TestEqual(TEXT("Equipped definitions aggregate Strength"), InitialBonus.StrengthBonus, 2);
	TestEqual(TEXT("Equipped definitions aggregate Constitution"), InitialBonus.ConstitutionBonus, 1);
	TestEqual(TEXT("Equipped definitions aggregate Armor"), InitialBonus.ArmorBonus, 1);
	TestTrue(TEXT("Equipped definitions aggregate carry-weight bonus"), FMath::IsNearlyEqual(InitialBonus.CarryWeightBonus, 4.0f));

	const FGridDamageResistanceSet InitialResistances = Component->ComputeCharacterEquipmentResistances(0);
	TestEqual(TEXT("Equipment resistances aggregate Physical"), InitialResistances.PhysicalResistance, 2);
	TestEqual(TEXT("Equipment resistances aggregate Fire"), InitialResistances.FireResistance, 4);
	TestEqual(TEXT("Equipment resistances leave Lightning at zero before the swap"), InitialResistances.LightningResistance, 0);

	TestTrue(TEXT("Equipping Axe onto occupied MainHand swaps atomically"),
		Component->EquipItemFromInventorySlot(0, 1, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Axe becomes the equipped MainHand item"),
		Component->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem) && EquippedItem.RuntimeObjectId == AxeRuntimeId);
	TestTrue(TEXT("The displaced sword moves into the Axe source inventory slot"), Character.InventorySlots[1].Item.RuntimeObjectId == SwordRuntimeId);
	TestTrue(TEXT("The displaced sword is normalized to inventory ownership"),
		Character.InventorySlots[1].Item.OwnerType == EGridItemOwnerType::CharacterInventory);
	TestTrue(TEXT("The displaced sword no longer records an equipment slot"), Character.InventorySlots[1].Item.EquipmentSlot == EGridEquipmentSlot::None);

	const FGridEquipmentStatBonus SwappedBonus = Component->ComputeCharacterEquipmentStatBonus(0);
	TestEqual(TEXT("Stat aggregation immediately reflects the equipped Axe"), SwappedBonus.StrengthBonus, 5);
	TestEqual(TEXT("Shield Constitution remains after the MainHand swap"), SwappedBonus.ConstitutionBonus, 1);
	TestEqual(TEXT("Sword Armor disappears after the MainHand swap"), SwappedBonus.ArmorBonus, 0);

	const FGridDamageResistanceSet SwappedResistances = Component->ComputeCharacterEquipmentResistances(0);
	TestEqual(TEXT("Shield Physical resistance remains after the MainHand swap"), SwappedResistances.PhysicalResistance, 2);
	TestEqual(TEXT("Sword Fire resistance disappears after the MainHand swap"), SwappedResistances.FireResistance, 1);
	TestEqual(TEXT("Axe Lightning resistance appears after the MainHand swap"), SwappedResistances.LightningResistance, 2);

	TestTrue(TEXT("Unequipping MainHand returns the Axe to the first free inventory slot"),
		Component->UnequipItemToInventory(0, EGridEquipmentSlot::MainHand));
	TestFalse(TEXT("MainHand is empty after unequip"), Component->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Unequip preserves the Axe runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == AxeRuntimeId);
	TestTrue(TEXT("Unequip normalizes inventory ownership"), Character.InventorySlots[0].Item.OwnerType == EGridItemOwnerType::CharacterInventory);
	TestTrue(TEXT("Unequip clears the equipment-slot marker"), Character.InventorySlots[0].Item.EquipmentSlot == EGridEquipmentSlot::None);
	if (!GridTD066ValidateOwnership(*this, Component, TEXT("Equip, swap and unequip transactions")))
	{
		return false;
	}

	TestFalse(TEXT("Invalid-character equipment reads fail"), Component->GetEquippedItem(INDEX_NONE, EGridEquipmentSlot::MainHand, EquippedItem));
	TestFalse(TEXT("Invalid-character occupancy reads fail"), Component->IsEquipmentSlotOccupied(INDEX_NONE, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Invalid-character stat aggregation returns an empty set"), !Component->ComputeCharacterEquipmentStatBonus(INDEX_NONE).HasAnyBonus());
	TestTrue(TEXT("Invalid-character resistance aggregation returns an empty set"), Component->ComputeCharacterEquipmentResistances(INDEX_NONE).IsEmpty());

	UGridPartyInventoryComponent* ConsumeComponent = GridTD066CreateInventory();
	if (!TestNotNull(TEXT("Combat-consumption component is created"), ConsumeComponent))
	{
		return false;
	}
	UGridItemDefinitionAsset* StackWeaponDefinition = GridTD066CreateItemDefinition(
		ConsumeComponent, TEXT("StackWeapon_TD066"), EGridItemType::Weapon, EGridEquipmentSlot::MainHand, 1.0f, true);
	if (!TestNotNull(TEXT("Stack weapon definition is created"), StackWeaponDefinition))
	{
		return false;
	}
	TestTrue(TEXT("Stack weapon definition is registered"), ConsumeComponent->RegisterItemDefinition(StackWeaponDefinition));
	const FGridItemInstance StackWeapon = GridTD066CreateItem(StackWeaponDefinition->ItemDefinitionId, 3, StackWeaponDefinition->Weight);
	const FGuid StackRuntimeId = StackWeapon.RuntimeObjectId;
	TestTrue(TEXT("Three-unit weapon stack enters inventory"), ConsumeComponent->AddItemToCharacterInventory(0, StackWeapon));
	TestTrue(TEXT("Three-unit weapon stack equips to MainHand"), ConsumeComponent->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand));

	TestFalse(TEXT("Combat consumption rejects the wrong hand"), ConsumeComponent->TryConsumeEquippedItemQuantityForCombatAction(
		0, EGridEquipmentSlot::OffHand, StackWeaponDefinition->ItemDefinitionId, StackRuntimeId, 1));
	TestFalse(TEXT("Combat consumption rejects the wrong definition"), ConsumeComponent->TryConsumeEquippedItemQuantityForCombatAction(
		0, EGridEquipmentSlot::MainHand, TEXT("WrongDefinition_TD066"), StackRuntimeId, 1));
	TestFalse(TEXT("Combat consumption rejects the wrong runtime identity"), ConsumeComponent->TryConsumeEquippedItemQuantityForCombatAction(
		0, EGridEquipmentSlot::MainHand, StackWeaponDefinition->ItemDefinitionId, FGuid::NewGuid(), 1));
	TestFalse(TEXT("Combat consumption rejects zero quantity"), ConsumeComponent->TryConsumeEquippedItemQuantityForCombatAction(
		0, EGridEquipmentSlot::MainHand, StackWeaponDefinition->ItemDefinitionId, StackRuntimeId, 0));
	TestFalse(TEXT("Combat consumption rejects a quantity larger than the equipped stack"), ConsumeComponent->TryConsumeEquippedItemQuantityForCombatAction(
		0, EGridEquipmentSlot::MainHand, StackWeaponDefinition->ItemDefinitionId, StackRuntimeId, 4));

	TestTrue(TEXT("Combat consumption decrements a valid equipped stack"), ConsumeComponent->TryConsumeEquippedItemQuantityForCombatAction(
		0, EGridEquipmentSlot::MainHand, StackWeaponDefinition->ItemDefinitionId, StackRuntimeId, 1));
	TestTrue(TEXT("The partially consumed equipped stack remains readable"),
		ConsumeComponent->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem));
	TestEqual(TEXT("Partial combat consumption decrements quantity exactly"), EquippedItem.Quantity, 2);
	TestTrue(TEXT("Partial combat consumption preserves runtime identity"), EquippedItem.RuntimeObjectId == StackRuntimeId);

	TestTrue(TEXT("Consuming the remaining stack clears the equipment slot"), ConsumeComponent->TryConsumeEquippedItemQuantityForCombatAction(
		0, EGridEquipmentSlot::MainHand, StackWeaponDefinition->ItemDefinitionId, StackRuntimeId, 2));
	TestFalse(TEXT("The fully consumed equipment stack is no longer readable"),
		ConsumeComponent->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem));
	TestFalse(TEXT("The fully consumed equipment slot reports empty"), ConsumeComponent->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
	if (!GridTD066ValidateOwnership(*this, ConsumeComponent, TEXT("Combat equipment consumption")))
	{
		return false;
	}

	UGridPartyInventoryComponent* FullInventoryComponent = GridTD066CreateInventory();
	if (!TestNotNull(TEXT("Full-inventory component is created"), FullInventoryComponent))
	{
		return false;
	}
	UGridItemDefinitionAsset* FullInventoryWeaponDefinition = GridTD066CreateItemDefinition(
		FullInventoryComponent, TEXT("FullInventoryWeapon_TD066"), EGridItemType::Weapon, EGridEquipmentSlot::MainHand, 1.0f);
	UGridItemDefinitionAsset* FillerDefinition = GridTD066CreateItemDefinition(
		FullInventoryComponent, TEXT("Filler_TD066"), EGridItemType::Misc, EGridEquipmentSlot::None, 0.1f);
	if (!TestNotNull(TEXT("Full-inventory weapon definition is created"), FullInventoryWeaponDefinition) ||
		!TestNotNull(TEXT("Filler definition is created"), FillerDefinition))
	{
		return false;
	}
	TestTrue(TEXT("Full-inventory weapon definition is registered"), FullInventoryComponent->RegisterItemDefinition(FullInventoryWeaponDefinition));
	TestTrue(TEXT("Filler definition is registered"), FullInventoryComponent->RegisterItemDefinition(FillerDefinition));

	const FGridItemInstance FullInventoryWeapon =
		GridTD066CreateItem(FullInventoryWeaponDefinition->ItemDefinitionId, 1, FullInventoryWeaponDefinition->Weight);
	const FGuid FullInventoryWeaponRuntimeId = FullInventoryWeapon.RuntimeObjectId;
	TestTrue(TEXT("Full-inventory test weapon enters inventory"), FullInventoryComponent->AddItemToCharacterInventory(0, FullInventoryWeapon));
	TestTrue(TEXT("Full-inventory test weapon equips"), FullInventoryComponent->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand));

	const int32 SlotCount = FullInventoryComponent->PartyInventoryState.ActiveCharacters[0].InventorySlots.Num();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const FGridItemInstance Filler = GridTD066CreateItem(FillerDefinition->ItemDefinitionId, 1, FillerDefinition->Weight);
		TestTrue(FString::Printf(TEXT("Filler item %d fills one inventory slot"), SlotIndex),
			FullInventoryComponent->AddItemToCharacterInventory(0, Filler));
	}

	AddExpectedError(TEXT("GridInventory Unequip Failed Character=0 Slot=MainHand Reason=InventoryFull"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Unequip fails atomically when no inventory slot is free"),
		FullInventoryComponent->UnequipItemToInventory(0, EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Failed full-inventory unequip keeps the source equipment occupied"),
		FullInventoryComponent->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem) &&
			EquippedItem.RuntimeObjectId == FullInventoryWeaponRuntimeId);
	if (!GridTD066ValidateOwnership(*this, FullInventoryComponent, TEXT("Failed full-inventory unequip")))
	{
		return false;
	}

	return true;
}

#endif
