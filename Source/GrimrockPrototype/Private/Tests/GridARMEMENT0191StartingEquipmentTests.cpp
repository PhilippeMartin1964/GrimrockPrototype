#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT0191
{
	static constexpr const TCHAR* BowPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.DA_Weapon_ShortBow_01");
	static constexpr const TCHAR* DaggerPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01");
	static constexpr const TCHAR* ShieldPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01");

	URPGRaceAsset* MakeRace()
	{
		URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
		Race->RaceId = TEXT("Human");
		Race->DisplayName = FText::FromString(TEXT("Human"));
		Race->AttributeBonuses = FRPGAttributes{1, 1, 1, 1, 1, 1};
		return Race;
	}

	URPGClassAsset* MakeClass()
	{
		URPGClassAsset* Class = NewObject<URPGClassAsset>();
		Class->ClassId = TEXT("Ranger");
		Class->DisplayName = FText::FromString(TEXT("Ranger"));
		Class->BaseAttributes = FRPGAttributes{15, 11, 13, 9, 9, 9};
		Class->HealthAtLevelOne = 18;
		Class->HealthPerLevel = 8;
		return Class;
	}

	FRPGStartingEquipmentKit& AddKit(URPGClassAsset* Class, FName KitId, bool bMakeDefault)
	{
		FRPGStartingEquipmentKit& Kit = Class->StartingEquipmentKits.AddDefaulted_GetRef();
		Kit.KitId = KitId;
		Kit.DisplayName = FText::FromName(KitId);
		if (bMakeDefault)
		{
			Class->DefaultStartingEquipmentKitId = KitId;
		}
		return Kit;
	}

	void AddEntry(FRPGStartingEquipmentKit& Kit, UGridItemDefinitionAsset* Definition, int32 Level, EGridEquipmentSlot Slot)
	{
		FRPGStartingEquipmentEntry& Entry = Kit.Items.AddDefaulted_GetRef();
		Entry.ItemDefinition = Definition;
		Entry.ItemLevel = Level;
		Entry.TargetSlot = Slot;
		Entry.Quantity = 1;
	}

	FRPGCharacterCreationRequest MakeRequest(URPGClassAsset* Class, URPGRaceAsset* Race)
	{
		FRPGCharacterCreationRequest Request;
		Request.DisplayName = FText::FromString(TEXT("Elias"));
		Request.RaceDefinition = Race;
		Request.ClassDefinition = Class;
		return Request;
	}

	UGridPartyInventoryComponent* NewInventory()
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->InitializeDefaultPartyIfNeeded();
		return Inventory;
	}

	int32 CountPhysicalItem(const UGridPartyInventoryComponent* Inventory, FName DefinitionId)
	{
		int32 Count = 0;
		const FGridCharacterInventoryState& Char = Inventory->PartyInventoryState.ActiveCharacters[0];
		for (const FGridInventorySlot& Slot : Char.InventorySlots)
		{
			if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == DefinitionId) ++Count;
		}
		const FGridCharacterEquipmentState& Gear = Inventory->PartyInventoryState.ActiveEquipment[0];
		if (Gear.MainHand.IsValid() && Gear.MainHand.ItemDefinitionId == DefinitionId) ++Count;
		if (Gear.OffHand.IsValid() && Gear.OffHand.ItemDefinitionId == DefinitionId) ++Count;
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0191LegacyCreationTest,
	"Grimrock.RPG.ARMEMENT01.9.StartingKits.LegacyCreationUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0191LegacyCreationTest::RunTest(const FString&)
{
	URPGClassAsset* Class = ARMEMENT0191::MakeClass();
	URPGRaceAsset* Race = ARMEMENT0191::MakeRace();
	UGridPartyInventoryComponent* Inventory = ARMEMENT0191::NewInventory();
	const FProperty* KitId = FGridCharacterInventoryState::StaticStruct()->FindPropertyByName(TEXT("StartingEquipmentKitId"));
	TestTrue(TEXT("Starting kit selection is a durable character property"), KitId && !KitId->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Unconfigured class remains valid"), Class->IsValidDefinition());
	FText Error;
	if (!TestTrue(TEXT("Legacy creation still succeeds"), Inventory->CreateInitialCharacter(ARMEMENT0191::MakeRequest(Class, Race), Error))) return false;
	TestTrue(TEXT("Unconfigured class grants no starting kit"), Inventory->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId.IsNone());
	TestFalse(TEXT("Main hand remains empty"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
	TestFalse(TEXT("Off hand remains empty"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
	TestEqual(TEXT("First bag slot remains empty"), Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].IsEmpty(), true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0191TwoHandsKitTest,
	"Grimrock.RPG.ARMEMENT01.9.StartingKits.TwoHandedAndFixedLevels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0191TwoHandsKitTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Bow = LoadObject<UGridItemDefinitionAsset>(nullptr, ARMEMENT0191::BowPath);
	UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr, ARMEMENT0191::DaggerPath);
	if (!TestNotNull(TEXT("Pilot bow exists"), Bow) || !TestNotNull(TEXT("Pilot dagger exists"), Dagger)) return false;
	URPGClassAsset* Class = ARMEMENT0191::MakeClass();
	URPGRaceAsset* Race = ARMEMENT0191::MakeRace();
	FRPGStartingEquipmentKit& Kit = ARMEMENT0191::AddKit(Class, TEXT("Ranger_Archer"), true);
	ARMEMENT0191::AddEntry(Kit, Bow, 9, EGridEquipmentSlot::OffHand);
	ARMEMENT0191::AddEntry(Kit, Dagger, 0, EGridEquipmentSlot::None);
	UGridPartyInventoryComponent* Inventory = ARMEMENT0191::NewInventory();
	if (!TestTrue(TEXT("Class with archer kit is valid"), Class->IsValidDefinition())) return false;
	FText Error;
	if (!TestTrue(TEXT("Create hero and equip archer kit atomically"), Inventory->CreateInitialCharacter(ARMEMENT0191::MakeRequest(Class, Race), Error))) return false;

	TestEqual(TEXT("Selection saved on character"), Inventory->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, FName(TEXT("Ranger_Archer")));
	TestTrue(TEXT("Two-handed bow reserves both hands"), Inventory->IsTwoHandedEquipped(0));
	const FGridCharacterEquipmentState& Gear = Inventory->PartyInventoryState.ActiveEquipment[0];
	TestEqual(TEXT("Bow stored in canonical MainHand"), Gear.MainHand.ItemDefinitionId, Bow->ItemDefinitionId);
	TestEqual(TEXT("Bow is level 9"), Gear.MainHand.ItemLevel, 9);
	TestFalse(TEXT("Virtual OffHand has no duplicate physical object"), Gear.OffHand.IsValid());
	TestEqual(TEXT("Only one physical bow"), ARMEMENT0191::CountPhysicalItem(Inventory, Bow->ItemDefinitionId), 1);
	TestEqual(TEXT("Dagger granted once to bag"), ARMEMENT0191::CountPhysicalItem(Inventory, Dagger->ItemDefinitionId), 1);
	TestEqual(TEXT("Default dagger level materialized"),
		Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.ItemLevel, Dagger->DefaultWeaponItemLevel);
	FGridItemInstance VisualOffHand;
	TestTrue(TEXT("OffHand projects same equipped bow"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::OffHand, VisualOffHand));
	TestEqual(TEXT("Projected bow retains the physical GUID"), VisualOffHand.RuntimeObjectId, Gear.MainHand.RuntimeObjectId);
	FString OwnershipError;
	TestTrue(TEXT("Kit items have unique valid ownership"), Inventory->ValidateInventoryOwnership(OwnershipError));
	TestTrue(TEXT("Equipped bow is registered for combat"), Inventory->FindItemDefinition(Bow->ItemDefinitionId) == Bow);
	TestTrue(TEXT("Bag dagger is registered for combat"), Inventory->FindItemDefinition(Dagger->ItemDefinitionId) == Dagger);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0191OneHandKitTest,
	"Grimrock.RPG.ARMEMENT01.9.StartingKits.DaggerAndShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0191OneHandKitTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr, ARMEMENT0191::DaggerPath);
	UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr, ARMEMENT0191::ShieldPath);
	if (!TestNotNull(TEXT("Pilot dagger exists"), Dagger) || !TestNotNull(TEXT("Pilot shield exists"), Shield)) return false;
	URPGClassAsset* Class = ARMEMENT0191::MakeClass();
	FRPGStartingEquipmentKit& Kit = ARMEMENT0191::AddKit(Class, TEXT("Warrior_Defender"), true);
	ARMEMENT0191::AddEntry(Kit, Dagger, 4, EGridEquipmentSlot::MainHand);
	ARMEMENT0191::AddEntry(Kit, Shield, 0, EGridEquipmentSlot::OffHand);
	UGridPartyInventoryComponent* Inventory = ARMEMENT0191::NewInventory();
	FText Error;
	if (!TestTrue(TEXT("Two compatible one-hand items can be granted"), Inventory->CreateInitialCharacter(
		ARMEMENT0191::MakeRequest(Class, ARMEMENT0191::MakeRace()), Error))) return false;
	FGridItemInstance Main, Off;
	TestTrue(TEXT("MainHand has dagger"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, Main));
	TestTrue(TEXT("OffHand has shield"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::OffHand, Off));
	TestEqual(TEXT("Dagger fixed level preserved"), Main.ItemLevel, 4);
	TestEqual(TEXT("Shield default level preserved"), Off.ItemLevel, Shield->DefaultWeaponItemLevel);
	TestTrue(TEXT("Two hands have unique physical GUIDs"), Main.RuntimeObjectId != Off.RuntimeObjectId);
	TestFalse(TEXT("No virtual two-hand reservation"), Inventory->IsTwoHandedEquipped(0));
	FString OwnershipError;
	TestTrue(TEXT("One-hand kit ownership valid"), Inventory->ValidateInventoryOwnership(OwnershipError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0191RollbackTest,
	"Grimrock.RPG.ARMEMENT01.9.StartingKits.RejectionIsAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0191RollbackTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Bow = LoadObject<UGridItemDefinitionAsset>(nullptr, ARMEMENT0191::BowPath);
	UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr, ARMEMENT0191::ShieldPath);
	UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr, ARMEMENT0191::DaggerPath);
	if (!TestNotNull(TEXT("Pilot assets exist"), Bow) || !TestNotNull(TEXT("Pilot shield exists"), Shield) ||
		!TestNotNull(TEXT("Pilot dagger exists"), Dagger)) return false;
	URPGClassAsset* Class = ARMEMENT0191::MakeClass();
	FRPGStartingEquipmentKit& Kit = ARMEMENT0191::AddKit(Class, TEXT("Conflicting"), true);
	ARMEMENT0191::AddEntry(Kit, Bow, 9, EGridEquipmentSlot::MainHand);
	ARMEMENT0191::AddEntry(Kit, Shield, 0, EGridEquipmentSlot::OffHand);
	UGridPartyInventoryComponent* Inventory = ARMEMENT0191::NewInventory();
	const FGuid BeforeId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
	const FName BeforeClassId = Inventory->PartyInventoryState.ActiveCharacters[0].ClassId;
	FText Error;
	TestFalse(TEXT("Two-handed bow plus shield cannot be granted"), Inventory->CreateInitialCharacter(
		ARMEMENT0191::MakeRequest(Class, ARMEMENT0191::MakeRace()), Error));
	TestFalse(TEXT("Rejected creation has an explanation"), Error.IsEmpty());
	TestFalse(TEXT("Rejected creation does not mark completion"), Inventory->HasCompletedInitialCharacterCreation());
	TestEqual(TEXT("Previous character identity unchanged"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, BeforeId);
	TestEqual(TEXT("Previous character class unchanged"), Inventory->PartyInventoryState.ActiveCharacters[0].ClassId, BeforeClassId);
	TestFalse(TEXT("Failed kit did not register bow"), Inventory->FindItemDefinition(Bow->ItemDefinitionId) != nullptr);

	// Capacity failure after staging one item must likewise reject the entire kit.
	Kit.Items.Reset();
	ARMEMENT0191::AddEntry(Kit, Dagger, 0, EGridEquipmentSlot::None);
	ARMEMENT0191::AddEntry(Kit, Shield, 0, EGridEquipmentSlot::None);
	Inventory->DefaultInventorySlotCountPerCharacter = 1;
	Error = FText::GetEmpty();
	TestFalse(TEXT("Full bag rejects both staged grants"), Inventory->CreateInitialCharacter(
		ARMEMENT0191::MakeRequest(Class, ARMEMENT0191::MakeRace()), Error));
	TestFalse(TEXT("Capacity failure returns an explanation"), Error.IsEmpty());
	TestFalse(TEXT("Capacity failure does not mark completion"), Inventory->HasCompletedInitialCharacterCreation());
	TestEqual(TEXT("Capacity failure keeps original character"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, BeforeId);
	TestFalse(TEXT("Failed kit does not register dagger"), Inventory->FindItemDefinition(Dagger->ItemDefinitionId) != nullptr);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
