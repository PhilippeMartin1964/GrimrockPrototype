#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/TextBlock.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory01SelectedBagProjectionTest, "Grimrock.UI.Inventory01.SelectedBagProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory01SelectedBagProjectionTest::RunTest(const FString& Parameters)
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

	Inventory->DefaultInventorySlotCountPerCharacter = 6;
	Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

	FGridCharacterInventoryState& First = Inventory->PartyInventoryState.ActiveCharacters[0];
	First.DisplayName = FText::FromString(TEXT("Ariadne"));
	First.InventorySlots.SetNum(6);

	FGridCharacterInventoryState& Second = Inventory->PartyInventoryState.ActiveCharacters[1];
	Second.DisplayName = FText::FromString(TEXT("Borin"));
	Second.InventorySlots.SetNum(6);

	Widget->InventoryComponent = Inventory;
	Widget->Text_InventoryBagOwner = NewObject<UTextBlock>(Widget);
	Widget->Text_InventoryBagWeight = NewObject<UTextBlock>(Widget);

	Widget->RefreshInventory();
	TestEqual(TEXT("Bag owner follows selected character 0"), Widget->Text_InventoryBagOwner->GetText().ToString(), FString(TEXT("Sac de : Ariadne")));
	TestTrue(TEXT("Bag weight uses compact Poids prefix for character 0"),
		Widget->Text_InventoryBagWeight->GetText().ToString().StartsWith(TEXT("Poids : ")));
	TestEqual(TEXT("Resolved slot count follows party-wide capacity"), Widget->ResolveInventorySlotWidgetCount(), 6);
	TestEqual(TEXT("Inventory slot count reads party-wide capacity"), Widget->GetInventorySlotCount(), 6);

	TestTrue(TEXT("Selecting character 1 succeeds"), Widget->SelectCharacter(1));
	TestEqual(TEXT("Bag owner switches to character 1"), Widget->Text_InventoryBagOwner->GetText().ToString(), FString(TEXT("Sac de : Borin")));
	TestTrue(TEXT("Bag weight uses compact Poids prefix for character 1"),
		Widget->Text_InventoryBagWeight->GetText().ToString().StartsWith(TEXT("Poids : ")));
	TestEqual(TEXT("Resolved slot count switches to same party-wide capacity"), Widget->ResolveInventorySlotWidgetCount(), 6);
	TestEqual(TEXT("Inventory slot count now reads same party-wide capacity"), Widget->GetInventorySlotCount(), 6);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory01SingleBagAuthorityTest, "Grimrock.UI.Inventory01.SingleBagAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory01SingleBagAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Widget = NewObject<UGridInventoryWidget>();
	if (!TestNotNull(TEXT("Party inventory exists"), Inventory) || !TestNotNull(TEXT("Inventory widget exists"), Widget))
	{
		return false;
	}

	Inventory->DefaultInventorySlotCountPerCharacter = 12;
	Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(12);
	Inventory->PartyInventoryState.ActiveCharacters[1].InventorySlots.SetNum(12);
	Widget->InventoryComponent = Inventory;

	Widget->InventorySlotColumnCount = 4;
	TestEqual(TEXT("Twelve slots remain authoritative with four columns"), Widget->ResolveInventorySlotWidgetCount(), 12);

	Widget->InventorySlotColumnCount = 6;
	TestEqual(TEXT("Changing columns never changes total slot capacity"), Widget->ResolveInventorySlotWidgetCount(), 12);

	TestTrue(TEXT("Authoritative selection changes"), Inventory->SetSelectedCharacterIndex(1));
	TestEqual(TEXT("Changing character never changes total slot capacity"), Widget->ResolveInventorySlotWidgetCount(), 12);

	return true;
}

#endif
