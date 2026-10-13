#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Localization/UILocalizationCsvAuthoring.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION032DSharedWbpVocabularyTest,
	"Game.UI.Localization.LOCALIZATION03.SharedWbpVocabulary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION032DSharedWbpVocabularyTest::RunTest(const FString&)
{
	TArray<FUILocalizationCsvEntry> Entries;
	FString Error;
	if (!FUILocalizationCsvAuthoring::LoadAndValidateProductionCsv(Entries, Error))
	{
		AddError(Error);
		return false;
	}

	TMap<FString, FString> SourceByKey;
	for (const FUILocalizationCsvEntry& Entry : Entries)
	{
		SourceByKey.Add(Entry.Key, Entry.SourceString);
	}

	const TMap<FString, FString> Expected = {
		{ TEXT("Navigation.Back"), TEXT("Retour") },
		{ TEXT("Menu.LoadGame"), TEXT("Charger partie") },
		{ TEXT("Menu.Options"), TEXT("Options") },
		{ TEXT("Menu.License"), TEXT("Licence") },
		{ TEXT("Inventory.Category.Equipment"), TEXT("Equipement") },
		{ TEXT("Resource.Mana"), TEXT("Mana") }
	};

	for (const TPair<FString, FString>& Pair : Expected)
	{
		const FString* Source = SourceByKey.Find(Pair.Key);
		if (!TestNotNull(
			*FString::Printf(TEXT("UI.csv contains shared WBP key '%s'"), *Pair.Key),
			Source))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("Shared WBP key '%s' keeps its exact authored source"), *Pair.Key),
			*Source,
			Pair.Value);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
