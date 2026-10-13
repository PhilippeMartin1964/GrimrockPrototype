#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Localization/UILocalizationCsvAuthoring.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION015
{
	const TCHAR* MigratedSources[] = {
		TEXT("Source/GrimrockPrototype/Private/UI/RPGCharacterCreationWidget.cpp"),
		TEXT("Source/GrimrockPrototype/Private/UI/RPGCharacterCreationWizardWidget.cpp"),
		TEXT("Source/GrimrockPrototype/Private/UI/RPGCharacterCreationAttributesStepWidget.cpp")
	};

	const TCHAR* ForbiddenPlayerFacingLiterals[] = {
		TEXT("Féminin"),
		TEXT("Masculin"),
		TEXT("Race non configurée"),
		TEXT("Classe non configurée"),
		TEXT("Identité du personnage"),
		TEXT("Nom du personnage"),
		TEXT("Choix actuels"),
		TEXT("Composant d'inventaire indisponible."),
		TEXT("Répartissez tous les points de caractéristiques"),
		TEXT("Points restants : {0} / {1}"),
		TEXT("Charge %d · CàC"),
		TEXT("Précision %+d · Esquive"),
		TEXT("Création du personnage impossible."),
		TEXT("Recrutement impossible.")
	};

	bool LoadSource(const TCHAR* RelativePath, FString& OutSource, FAutomationTestBase& Test)
	{
		const FString Filename = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		if (!FFileHelper::LoadFileToString(OutSource, *Filename))
		{
			Test.AddError(FString::Printf(TEXT("Unable to read migrated source '%s'."), *Filename));
			return false;
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION015CharacterCreationCsvCoverageTest,
	"Game.UI.Localization.LOCALIZATION01.CharacterCreationCsvCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION015CharacterCreationCsvCoverageTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION015;

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

	TestTrue(TEXT("Character Creation CSV contains authored entries"), Entries.Num() > 0);

	const FRegexPattern KeyPattern(TEXT("TEXT\\(\\\"(CharacterCreation\\.[A-Za-z0-9.]+)\\\"\\)"));
	TSet<FString> ReferencedKeys;

	for (const TCHAR* RelativePath : MigratedSources)
	{
		FString Source;
		if (!LoadSource(RelativePath, Source, *this))
		{
			continue;
		}

		for (const TCHAR* ForbiddenLiteral : ForbiddenPlayerFacingLiterals)
		{
			TestFalse(
				*FString::Printf(TEXT("%s no longer owns player wording '%s'"), RelativePath, ForbiddenLiteral),
				Source.Contains(ForbiddenLiteral));
		}

		FRegexMatcher Matcher(KeyPattern, Source);
		while (Matcher.FindNext())
		{
			ReferencedKeys.Add(Matcher.GetCaptureGroup(1));
		}
	}

	TestTrue(TEXT("Migrated Character Creation sources reference localization keys"), ReferencedKeys.Num() > 0);

	for (const FString& Key : ReferencedKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("UI.csv contains referenced key '%s'"), *Key),
			CsvKeys.Contains(Key));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
