#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUICharacter02SheetProjectionTest, "Grimrock.UI.Character02.SheetProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUICharacter02SheetProjectionTest::RunTest(const FString& Parameters)
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

	Inventory->PartyInventoryState.ActiveCharacters.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.DisplayName = FText::FromString(TEXT("Ariadne"));
	Character.Attributes = FRPGAttributes{ 12, 13, 14, 15, 16, 17 };
	Character.DerivedStats.MaxHealth = 100;
	Character.DerivedStats.MaxMana = 50;
	Character.DerivedStats.Initiative = 3;
	Character.DerivedStats.Accuracy = 4;
	Character.DerivedStats.Evasion = 5;
	Character.Resources.CurrentHealth = 75;
	Character.Resources.CurrentMana = 20;
	Character.Resources.CurrentPhysicalArmor = 12;
	Character.Resources.CurrentMagicalArmor = 7;
	Character.InventorySlots.SetNum(8);

	Widget->InventoryComponent = Inventory;
	Widget->Text_CharacterHealth = NewObject<UTextBlock>(Widget);
	Widget->Text_CharacterMana = NewObject<UTextBlock>(Widget);
	Widget->Text_CharacterInventorySlots = NewObject<UTextBlock>(Widget);
	Widget->Text_CharacterPhysicalArmor = NewObject<UTextBlock>(Widget);
	Widget->Text_CharacterMagicalArmor = NewObject<UTextBlock>(Widget);
	Widget->Text_CharacterInitiative = NewObject<UTextBlock>(Widget);
	Widget->Text_CharacterAccuracy = NewObject<UTextBlock>(Widget);
	Widget->Text_CharacterEvasion = NewObject<UTextBlock>(Widget);
	Widget->Text_ResistancePhysical = NewObject<UTextBlock>(Widget);
	Widget->ProgressBar_CharacterHealth = NewObject<UProgressBar>(Widget);
	Widget->ProgressBar_CharacterMana = NewObject<UProgressBar>(Widget);

	Widget->RefreshSelectedCharacterDetails();

	TestEqual(TEXT("Health text uses current/max projection"), Widget->Text_CharacterHealth->GetText().ToString(), FString(TEXT("75 / 100")));
	TestEqual(TEXT("Mana text uses current/max projection"), Widget->Text_CharacterMana->GetText().ToString(), FString(TEXT("20 / 50")));
	TestTrue(TEXT("Health progress is 75 percent"), FMath::IsNearlyEqual(Widget->ProgressBar_CharacterHealth->GetPercent(), 0.75f));
	TestTrue(TEXT("Mana progress is 40 percent"), FMath::IsNearlyEqual(Widget->ProgressBar_CharacterMana->GetPercent(), 0.4f));
	TestEqual(TEXT("Inventory slot usage is projected"), Widget->Text_CharacterInventorySlots->GetText().ToString(), FString(TEXT("0 / 8")));
	TestEqual(TEXT("Physical armor is projected"), Widget->Text_CharacterPhysicalArmor->GetText().ToString(), FString(TEXT("12")));
	TestEqual(TEXT("Magical armor is projected"), Widget->Text_CharacterMagicalArmor->GetText().ToString(), FString(TEXT("7")));
	TestEqual(TEXT("Initiative is projected"), Widget->Text_CharacterInitiative->GetText().ToString(), FString(TEXT("3")));
	TestEqual(TEXT("Accuracy is projected"), Widget->Text_CharacterAccuracy->GetText().ToString(), FString(TEXT("4")));
	TestEqual(TEXT("Evasion is projected"), Widget->Text_CharacterEvasion->GetText().ToString(), FString(TEXT("5")));
	TestEqual(TEXT("Physical resistance is projected from the canonical summary"), Widget->Text_ResistancePhysical->GetText().ToString(), FString(TEXT("0")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUICharacter02ProgressClampTest, "Grimrock.UI.Character02.ProgressClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUICharacter02ProgressClampTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Widget = NewObject<UGridInventoryWidget>();
	if (!TestNotNull(TEXT("Party inventory exists"), Inventory) || !TestNotNull(TEXT("Inventory widget exists"), Widget))
	{
		return false;
	}

	Inventory->PartyInventoryState.ActiveCharacters.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.DerivedStats.MaxHealth = 10;
	Character.Resources.CurrentHealth = 50;
	Character.DerivedStats.MaxMana = 0;
	Character.Resources.CurrentMana = 10;

	Widget->InventoryComponent = Inventory;
	Widget->ProgressBar_CharacterHealth = NewObject<UProgressBar>(Widget);
	Widget->ProgressBar_CharacterMana = NewObject<UProgressBar>(Widget);
	Widget->RefreshSelectedCharacterDetails();

	// GetCharacterSummary clamps mutable resources to their canonical maxima.
	TestTrue(TEXT("Health progress never exceeds one"), FMath::IsNearlyEqual(Widget->ProgressBar_CharacterHealth->GetPercent(), 1.0f));
	TestTrue(TEXT("Zero-capacity mana produces zero progress"), FMath::IsNearlyEqual(Widget->ProgressBar_CharacterMana->GetPercent(), 0.0f));
	return true;
}

#endif
