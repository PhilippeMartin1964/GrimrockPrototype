#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Internationalization/Regex.h"
#include "Localization/UILocalizationCsvAuthoring.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION017
{
	const TCHAR* MigratedSource =
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPresentation.cpp");

	const TCHAR* RequiredKeys[] = {
		TEXT("Status.Control.BlockSpellActions"),
		TEXT("Status.Control.BlockTranslation"),
		TEXT("Status.Control.SkipActivation"),
		TEXT("Status.Duration.Compact.Permanent"),
		TEXT("Status.Duration.Compact.Rounds"),
		TEXT("Status.Duration.Compact.Turns"),
		TEXT("Status.Duration.Permanent"),
		TEXT("Status.Duration.Rounds"),
		TEXT("Status.Duration.Turns"),
		TEXT("Status.Label.Stacked"),
		TEXT("Status.Label.StackedTimed"),
		TEXT("Status.Label.Timed"),
		TEXT("Status.Tooltip.Duration"),
		TEXT("Status.Tooltip.Initiative"),
		TEXT("Status.Tooltip.PeriodicDamage"),
		TEXT("Status.Tooltip.Stacks")
	};

	const TCHAR* ForbiddenPlayerFacingLiterals[] = {
		TEXT("Durée : %s"),
		TEXT("Stacks : %d"),
		TEXT("Initiative : %+d"),
		TEXT("Dégâts périodiques actifs."),
		TEXT("La prochaine activation correspondante est perdue."),
		TEXT("Les actions de type Sort sont bloquées."),
		TEXT("Les déplacements de case sont bloqués."),
		TEXT("T%d"),
		TEXT("R%d"),
		TEXT("PERM"),
		TEXT("1 tour"),
		TEXT("%d tours"),
		TEXT("1 manche"),
		TEXT("%d manches"),
		TEXT("FText::FromString(TEXT(\"Permanent\"))")
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION017StatusEffectPresentationMigrationTest,
	"Game.UI.Localization.LOCALIZATION01.StatusEffectPresentationCsvCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION017StatusEffectPresentationMigrationTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION017;

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
			*FString::Printf(TEXT("UI.csv contains Status key '%s'"), RequiredKey),
			CsvKeys.Contains(RequiredKey));
	}

	FString Source;
	const FString Filename = FPaths::Combine(FPaths::ProjectDir(), MigratedSource);
	if (!FFileHelper::LoadFileToString(Source, *Filename))
	{
		AddError(FString::Printf(TEXT("Unable to read migrated source '%s'."), *Filename));
		return false;
	}

	for (const TCHAR* ForbiddenLiteral : ForbiddenPlayerFacingLiterals)
	{
		TestFalse(
			*FString::Printf(TEXT("Status presentation no longer owns player wording '%s'"), ForbiddenLiteral),
			Source.Contains(ForbiddenLiteral));
	}

	TestTrue(
		TEXT("Status tooltip lines remain FText throughout composition"),
		Source.Contains(TEXT("TArray<FText> ToolTipLines")));
	TestFalse(
		TEXT("Status tooltip no longer rebuilds localized lines through FString"),
		Source.Contains(TEXT("TArray<FString> ToolTipLines")));

	const FRegexPattern KeyPattern(TEXT("TEXT\\(\\\"(Status\\.[A-Za-z0-9.]+)\\\"\\)"));
	FRegexMatcher Matcher(KeyPattern, Source);
	TSet<FString> ReferencedKeys;
	while (Matcher.FindNext())
	{
		ReferencedKeys.Add(Matcher.GetCaptureGroup(1));
	}

	TestEqual(
		TEXT("Status presentation references exactly the authored Status keys"),
		ReferencedKeys.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(RequiredKeys)));

	for (const FString& Key : ReferencedKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("UI.csv contains referenced Status key '%s'"), *Key),
			CsvKeys.Contains(Key));
	}

	FString EnglishFixture;
	const FString EnglishFixturePath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/Localization/ApplyEnglishSmokeTranslations.ps1"));
	if (FFileHelper::LoadFileToString(EnglishFixture, *EnglishFixturePath))
	{
		TestTrue(TEXT("English smoke fixture covers Status duration"), EnglishFixture.Contains(TEXT("Status.Duration.Rounds")));
		TestTrue(TEXT("English smoke fixture covers Status tooltip"), EnglishFixture.Contains(TEXT("Status.Tooltip.Duration")));
		TestTrue(TEXT("English smoke fixture covers Status control wording"), EnglishFixture.Contains(TEXT("Status.Control.BlockTranslation")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read English smoke fixture '%s'."), *EnglishFixturePath));
	}

	FString SmokeCommandlet;
	const FString SmokeCommandletPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Source/GrimrockPrototypeEditor/Private/Localization/UILocalizationEnglishSmokeCommandlet.cpp"));
	if (FFileHelper::LoadFileToString(SmokeCommandlet, *SmokeCommandletPath))
	{
		TestTrue(TEXT("Runtime smoke verifies formatted Status duration"), SmokeCommandlet.Contains(TEXT("FGridStatusEffectPresentationBuilder::FormatDuration")));
		TestTrue(TEXT("Runtime smoke checks two French rounds"), SmokeCommandlet.Contains(TEXT("TEXT(\"2 manches\")")));
		TestTrue(TEXT("Runtime smoke checks two English rounds"), SmokeCommandlet.Contains(TEXT("TEXT(\"2 rounds\")")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read runtime smoke commandlet '%s'."), *SmokeCommandletPath));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
