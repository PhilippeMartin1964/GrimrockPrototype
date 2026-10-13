#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Internationalization/Regex.h"
#include "Localization/UILocalizationCsvAuthoring.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION018
{
	const TCHAR* MigratedSources[] = {
		TEXT("Source/GrimrockPrototype/Private/Runtime/GridDoorActor.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/GridLeverActor.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/GridButtonActor.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/GridItemActor.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/GridReceptacleActor.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/GridGenericObjectActor.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/GridWallLockActor.cpp")
	};

	const TCHAR* RequiredKeys[] = {
		TEXT("Interaction.Button.Push"),
		TEXT("Interaction.Door.Close"),
		TEXT("Interaction.Door.Open"),
		TEXT("Interaction.Item.Take"),
		TEXT("Interaction.Lever.Pull"),
		TEXT("Interaction.Lever.Push"),
		TEXT("Interaction.Lock.AlreadyUnlocked"),
		TEXT("Interaction.Lock.Locked"),
		TEXT("Interaction.Lock.MissingKey"),
		TEXT("Interaction.Lock.UnlockSuccess"),
		TEXT("Interaction.Readable.Read"),
		TEXT("Interaction.Receptacle.PlaceItem")
	};

	const TCHAR* ForbiddenPlayerFacingLiterals[] = {
		TEXT("FText::FromString(bIsOpen ? TEXT(\"Close door\") : TEXT(\"Open door\"))"),
		TEXT("FText::FromString(bIsOn ? TEXT(\"Push lever\") : TEXT(\"Pull lever\"))"),
		TEXT("FText::FromString(TEXT(\"Push button\"))"),
		TEXT("FText::FromString(TEXT(\"Take\"))"),
		TEXT("FText::FromString(TEXT(\"Place item\"))"),
		TEXT("FText::FromString(TEXT(\"Read\"))"),
		TEXT("FText::FromString(TEXT(\"La serrure est verrouillée.\"))"),
		TEXT("FText::FromString(TEXT(\"La serrure s'ouvre avec un déclic métallique.\"))"),
		TEXT("FText::FromString(TEXT(\"La serrure est déjà déverrouillée.\"))"),
		TEXT("FText::FromString(TEXT(\"Il vous manque la clé adéquate.\"))")
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION018InteractionPromptMigrationTest,
	"Game.UI.Localization.LOCALIZATION01.InteractionPromptCsvCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION018InteractionPromptMigrationTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION018;

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
			*FString::Printf(TEXT("UI.csv contains Interaction key '%s'"), RequiredKey),
			CsvKeys.Contains(RequiredKey));
	}

	const FRegexPattern KeyPattern(TEXT("TEXT\\(\\\"(Interaction\\.[A-Za-z0-9.]+)\\\"\\)"));
	TSet<FString> ReferencedKeys;

	for (const TCHAR* RelativePath : MigratedSources)
	{
		FString Source;
		const FString Filename = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		if (!FFileHelper::LoadFileToString(Source, *Filename))
		{
			AddError(FString::Printf(TEXT("Unable to read migrated interaction source '%s'."), *Filename));
			continue;
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
		TEXT("Interaction sources reference exactly the authored Interaction keys"),
		ReferencedKeys.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(RequiredKeys)));

	for (const TCHAR* RequiredKey : RequiredKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("Migrated interaction sources reference key '%s'"), RequiredKey),
			ReferencedKeys.Contains(RequiredKey));
	}

	FString EnglishFixture;
	const FString EnglishFixturePath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/Localization/ApplyEnglishSmokeTranslations.ps1"));
	if (FFileHelper::LoadFileToString(EnglishFixture, *EnglishFixturePath))
	{
		TestTrue(TEXT("English fixture covers door interaction"), EnglishFixture.Contains(TEXT("Interaction.Door.Open")));
		TestTrue(TEXT("English fixture covers item interaction"), EnglishFixture.Contains(TEXT("Interaction.Item.Take")));
		TestTrue(TEXT("English fixture covers lock fallback"), EnglishFixture.Contains(TEXT("Interaction.Lock.Locked")));
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
		TestTrue(TEXT("Runtime smoke covers Interaction domain"), SmokeCommandlet.Contains(TEXT("Interaction.Receptacle.PlaceItem")));
		TestTrue(TEXT("Runtime smoke checks English door action"), SmokeCommandlet.Contains(TEXT("TEXT(\"Open door\")")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read runtime smoke commandlet '%s'."), *SmokePath));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
