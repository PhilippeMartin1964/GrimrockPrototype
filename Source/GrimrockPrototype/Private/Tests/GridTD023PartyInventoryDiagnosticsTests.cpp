#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridPartyInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD023PartyInventoryDiagnosticsContractTest, "Grimrock.TechnicalDebt.TD02_3.PartyInventoryDiagnosticsContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD023PartyInventoryDiagnosticsContractTest::RunTest(const FString& Parameters)
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
	Character.DisplayName = FText::FromString(TEXT("TD02 Diagnostics Tester"));
	Character.ClassId = TEXT("Warrior");
	Character.Level = 3;
	Character.InventorySlots.SetNum(1);

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = TEXT("TD02_Stone");
	Stone.DisplayName = FText::FromString(TEXT("Stone"));
	Stone.Quantity = 2;
	Stone.Weight = 1.0f;
	Stone.OwnerType = EGridItemOwnerType::CharacterInventory;
	Stone.OwnerGuid = Character.CharacterId;
	Stone.OwnerCharacterIndex = 0;
	Stone.EquipmentSlot = EGridEquipmentSlot::None;
	Character.InventorySlots[0].bOccupied = true;
	Character.InventorySlots[0].Item = Stone;

	Inventory->PartyInventoryState.ActiveCharacters = { Character };
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	Inventory->PartyInventoryState.MaxActiveCharacters = 6;

	FGridItemInstance Sword;
	Sword.RuntimeObjectId = FGuid::NewGuid();
	Sword.ItemDefinitionId = TEXT("TD02_Sword");
	Sword.DisplayName = FText::FromString(TEXT("Sword"));
	Sword.Quantity = 1;
	Sword.Weight = 3.0f;
	Sword.OwnerType = EGridItemOwnerType::EquipmentSlot;
	Sword.OwnerGuid = Character.CharacterId;
	Sword.OwnerCharacterIndex = 0;
	Sword.EquipmentSlot = EGridEquipmentSlot::MainHand;
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = Sword;

	const FString EquipmentDiagnostics = Inventory->GetEquipmentDiagnosticsForCharacter(0);
	TestTrue(TEXT("Equipment diagnostics expose the equipped main-hand item"), EquipmentDiagnostics.Contains(TEXT("MainHand=TD02_Sword")));
	TestEqual(TEXT("Invalid character diagnostics preserve the empty equipment contract"), Inventory->GetEquipmentDiagnosticsForCharacter(99),
		FString(TEXT("    Equipment: None")));

	const FString PartyDiagnostics = Inventory->GetPartyInventoryDiagnostics();
	TestTrue(TEXT("Party diagnostics keep their heading"), PartyDiagnostics.Contains(TEXT("GridPartyInventory Diagnostics")));
	TestTrue(TEXT("Party diagnostics expose the active character count"), PartyDiagnostics.Contains(TEXT("ActiveCharacters=1")));
	TestTrue(TEXT("Party diagnostics expose the selected character"), PartyDiagnostics.Contains(TEXT("SelectedCharacter=0")));
	TestTrue(TEXT("Party diagnostics expose the character identity"), PartyDiagnostics.Contains(TEXT("Name=TD02 Diagnostics Tester")));
	TestTrue(TEXT("Party diagnostics expose equipment"), PartyDiagnostics.Contains(TEXT("MainHand=TD02_Sword")));
	TestTrue(TEXT("Party diagnostics expose inventory quantity"), PartyDiagnostics.Contains(TEXT("Item=TD02_Stone Qty=2")));
	TestTrue(TEXT("Party diagnostics expose inventory ownership"), PartyDiagnostics.Contains(TEXT("Owner=CharacterInventory")));

	FString OwnershipError;
	TestTrue(TEXT("The characterized fixture preserves valid inventory ownership"), Inventory->ValidateInventoryOwnership(OwnershipError));
	TestTrue(TEXT("Valid ownership produces no error text"), OwnershipError.IsEmpty());

	return true;
}

#endif
