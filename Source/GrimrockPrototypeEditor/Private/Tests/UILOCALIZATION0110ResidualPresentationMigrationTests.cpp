#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Internationalization/Regex.h"
#include "Localization/UILocalizationCsvAuthoring.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION0110
{
	const TCHAR* MigratedSources[] = {
		TEXT("Source/GrimrockPrototype/Private/UI/GridInventoryWidget.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockGameInstance.cpp"),
		TEXT("Source/GrimrockPrototype/Private/UI/GrimrockLoadGameSlotWidget.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Magic/GridSpellbookUI.cpp"),
		TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidget.cpp")
	};

	const TCHAR* RequiredKeys[] = {
		TEXT("Inventory.Bag.Weight"),
		TEXT("Progression.Skill.AllocationFailed"),
		TEXT("Progression.Skill.CancellationFailed"),
		TEXT("SaveLoad.Slot.Available"),
		TEXT("SaveLoad.Slot.Numbered"),
		TEXT("SaveLoad.Slot.Primary"),
		TEXT("SaveLoad.Slot.Unavailable"),
		TEXT("Spellbook.Fallback.DefinitionUnavailable")
	};

	const TCHAR* ForbiddenPlayerFacingLiterals[] = {
		TEXT("FString::Printf(TEXT(\"Poids : %.1f / %.1f\")"),
		TEXT("FText::FromString(TEXT(\"Sauvegarde principale\"))"),
		TEXT("FString::Printf(TEXT(\"Sauvegarde %d\")"),
		TEXT("FText::FromString(TEXT(\"Disponible\"))"),
		TEXT("FText::FromString(TEXT(\"Indisponible\"))"),
		TEXT("FText::FromString(TEXT(\"Définition de sort indisponible.\"))"),
		TEXT("FText::FromString(TEXT(\"Attribution de compétence impossible.\"))"),
		TEXT("FText::FromString(TEXT(\"Annulation de compétence impossible.\"))")
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION0110ResidualPresentationMigrationTest,
	"Game.UI.Localization.LOCALIZATION01.ResidualPresentationCsvCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION0110ResidualPresentationMigrationTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION0110;

	TArray<FUILocalizationCsvEntry> Entries;
	FString Error;
	if (!FUILocalizationCsvAuthoring::LoadAndValidateProductionCsv(Entries, Error))
	{
		AddError(Error);
		return false;
	}

	TSet<FString> CsvKeys;
	for (const FUILocalizationCsvEntry& Entry : Entries)
	{
		CsvKeys.Add(Entry.Key);
	}

	for (const TCHAR* RequiredKey : RequiredKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("UI.csv contains residual localization key '%s'"), RequiredKey),
			CsvKeys.Contains(RequiredKey));
	}

	const FRegexPattern KeyPattern(TEXT("TEXT\\(\\\"((?:Inventory|SaveLoad|Spellbook|Progression)\\.[A-Za-z0-9.]+)\\\"\\)"));
	TSet<FString> ReferencedKeys;
	FString InventorySource;
	FString SaveSource;

	for (const TCHAR* RelativePath : MigratedSources)
	{
		FString Source;
		const FString Filename = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		if (!FFileHelper::LoadFileToString(Source, *Filename))
		{
			AddError(FString::Printf(TEXT("Unable to read migrated residual source '%s'."), *Filename));
			continue;
		}

		if (FString(RelativePath).EndsWith(TEXT("GridInventoryWidget.cpp")))
		{
			InventorySource = Source;
		}
		else if (FString(RelativePath).EndsWith(TEXT("GrimrockGameInstance.cpp")))
		{
			SaveSource = Source;
		}

		for (const TCHAR* ForbiddenLiteral : ForbiddenPlayerFacingLiterals)
		{
			TestFalse(
				*FString::Printf(TEXT("%s no longer owns migrated player-facing wording"), RelativePath),
				Source.Contains(ForbiddenLiteral));
		}

		FRegexMatcher Matcher(KeyPattern, Source);
		while (Matcher.FindNext())
		{
			ReferencedKeys.Add(Matcher.GetCaptureGroup(1));
		}
	}

	TestEqual(
		TEXT("Residual sources reference exactly the authored residual keys"),
		ReferencedKeys.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(RequiredKeys)));

	for (const TCHAR* RequiredKey : RequiredKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("Migrated residual sources reference key '%s'"), RequiredKey),
			ReferencedKeys.Contains(RequiredKey));
	}

	TestTrue(
		TEXT("Inventory weight uses localized named formatting"),
		InventorySource.Contains(TEXT("FText::FormatNamed")) &&
			InventorySource.Contains(TEXT("Inventory.Bag.Weight")));
	TestTrue(
		TEXT("Save slot numbering uses localized named formatting"),
		SaveSource.Contains(TEXT("FText::FormatNamed")) &&
			SaveSource.Contains(TEXT("SaveLoad.Slot.Numbered")));

	FString EnglishFixture;
	const FString EnglishFixturePath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/Localization/ApplyEnglishSmokeTranslations.ps1"));
	if (FFileHelper::LoadFileToString(EnglishFixture, *EnglishFixturePath))
	{
		TestTrue(TEXT("English fixture covers inventory weight"), EnglishFixture.Contains(TEXT("Inventory.Bag.Weight")));
		TestTrue(TEXT("English fixture covers numbered save slot"), EnglishFixture.Contains(TEXT("SaveLoad.Slot.Numbered")));
		TestTrue(TEXT("English fixture covers Spellbook fallback"), EnglishFixture.Contains(TEXT("Spellbook.Fallback.DefinitionUnavailable")));
		TestTrue(TEXT("English fixture covers Skill fallback"), EnglishFixture.Contains(TEXT("Progression.Skill.AllocationFailed")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read English smoke fixture '%s'."), *EnglishFixturePath));
	}

	FString SmokeCommandlet;
	const FString SmokePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Source/GrimrockPrototypeEditor/Private/Localization/UILocalizationEnglishSmokeCommandlet.cpp"));
	if (FFileHelper::LoadFileToString(SmokeCommandlet, *SmokePath))
	{
		TestTrue(TEXT("Runtime smoke covers inventory residual"), SmokeCommandlet.Contains(TEXT("Inventory.Bag.Weight")));
		TestTrue(TEXT("Runtime smoke covers SaveLoad residual"), SmokeCommandlet.Contains(TEXT("SaveLoad.Slot.Primary")));
		TestTrue(TEXT("Runtime smoke covers Spellbook residual"), SmokeCommandlet.Contains(TEXT("Spellbook.Fallback.DefinitionUnavailable")));
		TestTrue(TEXT("Runtime smoke covers Skill residual"), SmokeCommandlet.Contains(TEXT("Progression.Skill.CancellationFailed")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read runtime smoke commandlet '%s'."), *SmokePath));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
