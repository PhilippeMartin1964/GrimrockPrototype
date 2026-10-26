#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace Shield022
{
	UGridItemDefinitionAsset* RealShield()
	{
		return LoadObject<UGridItemDefinitionAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
	}

	URPGClassAsset* MakeClass(UObject* Outer, FName Id, int32 Physical, int32 Magical)
	{
		URPGClassAsset* Result = NewObject<URPGClassAsset>(Outer);
		Result->ClassId = Id;
		Result->DisplayName = FText::FromName(Id);
		Result->HealthAtLevelOne = 20;
		Result->BasePhysicalArmor = Physical;
		Result->BaseMagicalArmor = Magical;
		return Result;
	}

	FGridCharacterInventoryState MakeCharacter(URPGClassAsset* Definition, int32 Health)
	{
		FGridCharacterInventoryState Result;
		Result.CharacterId = FGuid::NewGuid();
		Result.ClassId = Definition->ClassId;
		Result.ClassDefinition = Definition;
		Result.Level = 1;
		Result.Resources.CurrentHealth = Health;
		Result.Resources.CurrentMana = 7;
		Result.Resources.CurrentPhysicalArmor = 0;
		Result.Resources.CurrentMagicalArmor = 1;
		Result.DerivedStats.MaxHealth = 20;
		Result.DerivedStats.MaxMana = 12;
		Result.InventorySlots.SetNum(2);
		return Result;
	}

	FGridItemInstance MakeItem(const UGridItemDefinitionAsset* Definition, int32 Spent)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.Quantity = 1;
		Item.ItemLevel = 1;
		Item.AbsorbedEquipmentPhysicalArmor = Spent;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield022VictoryRefillTest,
	"Grimrock.RPG.SHIELD02.2.VictoryRefillsPhysicalAndMagical",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield022VictoryRefillTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Shield = Shield022::RealShield();
	if (!TestNotNull(TEXT("Real ARM075 N1 round shield"), Shield))
		return false;
	TestEqual(TEXT("Round shield grants five physical armor"), Shield->EquipmentStatBonus.ArmorBonus, 5);
	const bool bShieldDirty = Shield->GetOutermost()->IsDirty();

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* Class = Shield022::MakeClass(Inventory, TEXT("Shield022VictoryClass"), 2, 3);
	if (!TestTrue(TEXT("Synthetic class valid"), Class->IsValidDefinition()) ||
		!TestTrue(TEXT("Shield definition registered"), Inventory->RegisterItemDefinition(Shield)))
		return false;

	Inventory->PartyInventoryState.ActiveCharacters.Add(Shield022::MakeCharacter(Class, 14));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	auto& State = Inventory->PartyInventoryState;
	State.ActiveEquipment[0].OffHand = Shield022::MakeItem(Shield, 5);
	State.ActiveEquipment[0].OffHand.OwnerType = EGridItemOwnerType::EquipmentSlot;
	State.ActiveEquipment[0].OffHand.OwnerCharacterIndex = 0;
	State.ActiveEquipment[0].OffHand.EquipmentSlot = EGridEquipmentSlot::OffHand;
	State.ActiveCharacters[0].InventorySlots[0].bOccupied = true;
	State.ActiveCharacters[0].InventorySlots[0].Item = Shield022::MakeItem(Shield, 4);
	State.ActiveCharacters[0].InventorySlots[0].Item.OwnerType = EGridItemOwnerType::CharacterInventory;
	State.ActiveCharacters[0].InventorySlots[0].Item.OwnerCharacterIndex = 0;
	State.bHasCursorItem = true;
	State.CursorItem = Shield022::MakeItem(Shield, 2);
	State.CursorItem.OwnerType = EGridItemOwnerType::Cursor;

	const FGuid EquippedId = State.ActiveEquipment[0].OffHand.RuntimeObjectId;
	const FGuid BagId = State.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId;
	const FGuid CursorId = State.CursorItem.RuntimeObjectId;
	TestEqual(TEXT("Physical armor depleted before Victory"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);

	TestEqual(TEXT("One living character changed by refill"), Inventory->RefillArmorReservesAfterVictory(), 1);
	TestEqual(TEXT("Equipped armor fully restored"), State.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Unworn shield in bag fully restored"), State.ActiveCharacters[0].InventorySlots[0].Item.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Party-held cursor item fully restored"), State.CursorItem.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Class base physical reserve restored"), State.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 2);
	TestEqual(TEXT("Magical armor restored to class reference"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Physical armor sums class 2 and equipped shield 5"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Victory does not heal HP"), State.ActiveCharacters[0].Resources.CurrentHealth, 14);
	TestEqual(TEXT("Victory does not restore mana"), State.ActiveCharacters[0].Resources.CurrentMana, 7);
	TestEqual(TEXT("Equipped GUID remains"), State.ActiveEquipment[0].OffHand.RuntimeObjectId, EquippedId);
	TestEqual(TEXT("Bag GUID remains"), State.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId, BagId);
	TestEqual(TEXT("Cursor GUID remains"), State.CursorItem.RuntimeObjectId, CursorId);

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("UI summary reflects restored armor"), Inventory->GetCharacterSummary(0, Summary));
	TestEqual(TEXT("UI and combat agree on current physical armor"), Summary.Resources.CurrentPhysicalArmor, 7);
	TestEqual(TEXT("UI and combat agree on current magical armor"), Summary.Resources.CurrentMagicalArmor, 3);

	TestEqual(TEXT("Re-applying full refill changes nobody"), Inventory->RefillArmorReservesAfterVictory(), 0);
	TestEqual(TEXT("Repeated refill cannot exceed physical reference"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Repeated refill cannot exceed magical reference"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);

	// Second encounter: no automatic refill until the next authorized Victory.
	Inventory->ApplyPhysicalArmorDamage(0, 4);
	State.ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	TestEqual(TEXT("Second encounter spends physical armor"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 3);
	TestEqual(TEXT("Next Victory returns physical armor"), Inventory->RefillArmorReservesAfterVictory(), 1);
	TestEqual(TEXT("Second Victory restores full physical pool"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Second Victory restores magic"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Real DataAsset not modified"), Shield->GetOutermost()->IsDirty(), bShieldDirty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield022DownedNoRefillTest,
	"Grimrock.RPG.SHIELD02.2.DownedCharactersAreNotRefilled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield022DownedNoRefillTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Shield = Shield022::RealShield();
	if (!TestNotNull(TEXT("Real ARM075 shield"), Shield))
		return false;
	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* Class = Shield022::MakeClass(Inventory, TEXT("Shield022DefeatClass"), 1, 4);
	if (!TestTrue(TEXT("Synthetic class valid"), Class->IsValidDefinition()) ||
		!TestTrue(TEXT("Shield registered"), Inventory->RegisterItemDefinition(Shield)))
		return false;

	Inventory->PartyInventoryState.ActiveCharacters.Add(Shield022::MakeCharacter(Class, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	auto& State = Inventory->PartyInventoryState;
	State.ActiveEquipment[0].OffHand = Shield022::MakeItem(Shield, 5);
	State.ActiveCharacters[0].Resources.CurrentPhysicalArmor = 0;
	State.ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	State.bHasCursorItem = true;
	State.CursorItem = Shield022::MakeItem(Shield, 4);

	TestEqual(TEXT("No living member means no refill"), Inventory->RefillArmorReservesAfterVictory(), 0);
	TestEqual(TEXT("Downed shield remains depleted"), State.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 5);
	TestEqual(TEXT("Downed magic remains depleted"), State.ActiveCharacters[0].Resources.CurrentMagicalArmor, 0);
	TestEqual(TEXT("Cursor not recharged without survivor"), State.CursorItem.AbsorbedEquipmentPhysicalArmor, 4);
	TestEqual(TEXT("Refill does not resurrect"), State.ActiveCharacters[0].Resources.CurrentHealth, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
