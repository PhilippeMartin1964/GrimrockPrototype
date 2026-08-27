#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	URPGRaceAsset* CreateHumanRaceDefinition()
	{
		URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
		Race->RaceId = TEXT("Human");
		Race->DisplayName = FText::FromString(TEXT("Human"));
		Race->AttributeBonuses = FRPGAttributes{ 1, 1, 1, 1, 1, 1 };
		return Race;
	}

	URPGClassAsset* CreateWarriorClassDefinition()
	{
		URPGClassAsset* CharacterClass = NewObject<URPGClassAsset>();
		CharacterClass->ClassId = TEXT("Warrior");
		CharacterClass->DisplayName = FText::FromString(TEXT("Warrior"));
		CharacterClass->BaseAttributes = FRPGAttributes{ 15, 11, 13, 9, 9, 9 };
		CharacterClass->HealthAtLevelOne = 18;
		CharacterClass->HealthPerLevel = 8;
		CharacterClass->ManaAtLevelOne = 0;
		CharacterClass->ManaPerLevel = 0;
		return CharacterClass;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGAttributeModifierCC1Test, "Grimrock.CharacterCreation.CC1.AttributeModifiers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGAttributeModifierCC1Test::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Attribute 8 gives modifier -1"), URPGCharacterRulesLibrary::GetAttributeModifier(8), -1);
	TestEqual(TEXT("Attribute 9 gives modifier -1"), URPGCharacterRulesLibrary::GetAttributeModifier(9), -1);
	TestEqual(TEXT("Attribute 10 gives modifier 0"), URPGCharacterRulesLibrary::GetAttributeModifier(10), 0);
	TestEqual(TEXT("Attribute 16 gives modifier 3"), URPGCharacterRulesLibrary::GetAttributeModifier(16), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGHumanWarriorProfileCC1Test, "Grimrock.CharacterCreation.CC1.HumanWarriorProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGHumanWarriorProfileCC1Test::RunTest(const FString& Parameters)
{
	const URPGRaceAsset* HumanRace = CreateHumanRaceDefinition();
	const URPGClassAsset* WarriorClass = CreateWarriorClassDefinition();

	TestTrue(TEXT("Human race definition is valid"), HumanRace->IsValidDefinition());
	TestTrue(TEXT("Warrior class definition is valid"), WarriorClass->IsValidDefinition());

	const FRPGAttributes Attributes = URPGCharacterRulesLibrary::AddAttributes(WarriorClass->BaseAttributes, HumanRace->AttributeBonuses);

	TestEqual(TEXT("Strength is 16"), Attributes.Strength, 16);
	TestEqual(TEXT("Dexterity is 12"), Attributes.Dexterity, 12);
	TestEqual(TEXT("Constitution is 14"), Attributes.Constitution, 14);
	TestEqual(TEXT("Intelligence is 10"), Attributes.Intelligence, 10);
	TestEqual(TEXT("Wisdom is 10"), Attributes.Wisdom, 10);
	TestEqual(TEXT("Charisma is 10"), Attributes.Charisma, 10);
	TestTrue(TEXT("Starting attributes stay in the supported range"), URPGCharacterRulesLibrary::AreAttributesInRange(Attributes));

	const FRPGDerivedStats DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Attributes, WarriorClass, 1);
	const FRPGCharacterResources Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(DerivedStats, WarriorClass);
	TestEqual(TEXT("Level-one warrior has 20 health with Constitution modifier"), DerivedStats.MaxHealth, 20);
	TestEqual(TEXT("Current health starts full"), Resources.CurrentHealth, 20);
	TestEqual(TEXT("Warrior starts without mana"), DerivedStats.MaxMana, 0);
	TestTrue(TEXT("Strength 16 gives carry capacity 80"), FMath::IsNearlyEqual(URPGCharacterRulesLibrary::CalculateMaxCarryWeight(Attributes), 80.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGCurrentAttributeAuthorityCC1Test, "Grimrock.CharacterCreation.CC1.CurrentAttributeAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGCurrentAttributeAuthorityCC1Test::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
	FGridCharacterInventoryState Character;
	Character.Attributes = FRPGAttributes(13, 10, 10, 10, 10, 10);
	Component->PartyInventoryState.ActiveCharacters.Add(Character);

	Component->InitializeDefaultPartyIfNeeded();

	const FGridCharacterInventoryState& InitializedCharacter = Component->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Current Attributes.Strength remains authoritative"), InitializedCharacter.Attributes.Strength, 13);
	TestTrue(TEXT("Current Strength controls carry capacity"), FMath::IsNearlyEqual(InitializedCharacter.MaxCarryWeight, 65.0f));
	return true;
}

#endif
