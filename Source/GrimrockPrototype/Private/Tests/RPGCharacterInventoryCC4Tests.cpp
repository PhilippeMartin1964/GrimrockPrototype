#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGCharacterInventorySummaryCC4Test, "Grimrock.CharacterCreation.CC4.InventorySummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGCharacterInventorySummaryCC4Test::RunTest(const FString& Parameters)
{
	URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
	Race->RaceId = TEXT("Human");
	Race->DisplayName = FText::FromString(TEXT("Humain"));
	Race->AttributeBonuses = FRPGAttributes{ 1, 1, 1, 1, 1, 1 };

	URPGClassAsset* CharacterClass = NewObject<URPGClassAsset>();
	CharacterClass->ClassId = TEXT("Warrior");
	CharacterClass->DisplayName = FText::FromString(TEXT("Guerrier"));
	CharacterClass->BaseAttributes = FRPGAttributes{ 15, 11, 13, 9, 9, 9 };
	CharacterClass->HealthAtLevelOne = 18;
	CharacterClass->HealthPerLevel = 8;

	FRPGCharacterCreationRequest Request;
	Request.DisplayName = FText::FromString(TEXT("Elias"));
	Request.RaceDefinition = Race;
	Request.ClassDefinition = CharacterClass;

	UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
	Component->InitializeDefaultPartyIfNeeded();

	FText Error;
	TestTrue(TEXT("Character creation succeeds"), Component->CreateInitialCharacter(Request, Error));

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("The selected character summary is available"), Component->GetCharacterSummary(0, Summary));
	TestEqual(TEXT("The summary exposes the character name"), Summary.DisplayName.ToString(), FString(TEXT("Elias")));
	TestEqual(TEXT("The summary exposes the localized race"), Summary.RaceDisplayName.ToString(), FString(TEXT("Humain")));
	TestEqual(TEXT("The summary exposes the localized class"), Summary.ClassDisplayName.ToString(), FString(TEXT("Guerrier")));
	TestEqual(TEXT("The summary exposes level one"), Summary.Level, 1);
	TestEqual(TEXT("The summary exposes zero experience"), Summary.Experience, 0);
	TestEqual(TEXT("The summary exposes Strength 16"), Summary.Attributes.Strength, 16);
	TestEqual(TEXT("The summary exposes Dexterity 12"), Summary.Attributes.Dexterity, 12);
	TestEqual(TEXT("The summary exposes Constitution 14"), Summary.Attributes.Constitution, 14);
	TestEqual(TEXT("The summary exposes Intelligence 10"), Summary.Attributes.Intelligence, 10);
	TestEqual(TEXT("The summary exposes Wisdom 10"), Summary.Attributes.Wisdom, 10);
	TestEqual(TEXT("The summary exposes Charisma 10"), Summary.Attributes.Charisma, 10);
	TestEqual(TEXT("The summary exposes current health"), Summary.Resources.CurrentHealth, 20);
	TestEqual(TEXT("The summary exposes maximum health"), Summary.DerivedStats.MaxHealth, 20);
	TestEqual(TEXT("The summary exposes maximum mana"), Summary.DerivedStats.MaxMana, 0);
	TestTrue(TEXT("The summary exposes maximum carry weight 80"), FMath::IsNearlyEqual(Summary.MaxWeight, 80.0f));
	TestTrue(TEXT("The summary identifies the selected character"), Summary.bIsSelected);
	return true;
}

#endif
