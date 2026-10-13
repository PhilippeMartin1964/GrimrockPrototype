#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/TextKey.h"
#include "Localization/UILocalizationCsvAuthoring.h"
#include "UObject/Package.h"

namespace UILOCALIZATION014
{
	bool ParseFixture(
		FAutomationTestBase& Test,
		const FString& Csv,
		TArray<FUILocalizationCsvEntry>& OutEntries)
	{
		FString Error;
		const bool bValid = FUILocalizationCsvAuthoring::ParseAndValidateCsv(Csv, OutEntries, Error);
		if (!bValid)
		{
			Test.AddError(FString::Printf(TEXT("Fixture rejected: %s"), *Error));
		}
		return bValid;
	}

	bool RejectFixture(
		FAutomationTestBase& Test,
		const FString& Label,
		const FString& Csv)
	{
		TArray<FUILocalizationCsvEntry> Entries;
		FString Error;
		const bool bValid = FUILocalizationCsvAuthoring::ParseAndValidateCsv(Csv, Entries, Error);
		Test.TestFalse(*Label, bValid);
		Test.TestFalse(*FString::Printf(TEXT("%s reports a diagnostic"), *Label), Error.IsEmpty());
		return !bValid;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION014CsvSourceIntegrityTest,
	"Game.UI.Localization.LOCALIZATION01.CsvSourceIntegrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION014CsvSourceIntegrityTest::RunTest(const FString&)
{
	TestFalse(
		TEXT("Localization source path uses a neutral identifier"),
		FString(FUILocalizationCsvAuthoring::SourceRelativePath()).Contains(TEXT("Grimrock"), ESearchCase::IgnoreCase));
	TestFalse(
		TEXT("Generated String Table package uses a neutral identifier"),
		FString(FUILocalizationCsvAuthoring::PackageName()).Contains(TEXT("Grimrock"), ESearchCase::IgnoreCase));
	TestFalse(
		TEXT("Generated String Table asset uses a neutral identifier"),
		FString(FUILocalizationCsvAuthoring::AssetName()).Contains(TEXT("Grimrock"), ESearchCase::IgnoreCase));

	TArray<FUILocalizationCsvEntry> Entries;
	FString Error;
	const bool bValid = FUILocalizationCsvAuthoring::LoadAndValidateProductionCsv(Entries, Error);
	TestTrue(TEXT("Production UI.csv exists and satisfies the localization authoring contract"), bValid);
	if (!bValid && !Error.IsEmpty())
	{
		AddError(Error);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION014CsvValidationContractTest,
	"Game.UI.Localization.LOCALIZATION01.CsvValidationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION014CsvValidationContractTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION014;

	const FString ValidCsv =
		TEXT("Key,SourceString,Comment\n")
		TEXT("\"Interaction.Door.Open\",\"Ouvrir la porte\",\"Interaction générique\"\n")
		TEXT("\"SaveLoad.Slot.Indexed\",\"Sauvegarde {Index}\",\"Slot numéroté\"\n");

	TArray<FUILocalizationCsvEntry> Entries;
	if (!ParseFixture(*this, ValidCsv, Entries))
	{
		return false;
	}

	TestEqual(TEXT("Valid fixture exposes two rows"), Entries.Num(), 2);
	TestEqual(TEXT("First key is preserved"), Entries[0].Key, FString(TEXT("Interaction.Door.Open")));
	TestEqual(TEXT("Named placeholder is preserved"), Entries[1].SourceString, FString(TEXT("Sauvegarde {Index}")));

	RejectFixture(
		*this,
		TEXT("Duplicate keys are rejected"),
		TEXT("Key,SourceString,Comment\n")
		TEXT("Interaction.Door.Open,Ouvrir,One\n")
		TEXT("Interaction.Door.Open,Fermer,Two\n"));

	RejectFixture(
		*this,
		TEXT("Forbidden identifier token is rejected case-insensitively"),
		TEXT("Key,SourceString,Comment\n")
		TEXT("Interaction.GRIMROCK.Open,Ouvrir,Forbidden\n"));

	RejectFixture(
		*this,
		TEXT("Invalid key syntax is rejected"),
		TEXT("Key,SourceString,Comment\n")
		TEXT("Interaction Door Open,Ouvrir,Invalid\n"));

	RejectFixture(
		*this,
		TEXT("Empty source strings are rejected"),
		TEXT("Key,SourceString,Comment\n")
		TEXT("Interaction.Door.Open,,Missing\n"));

	RejectFixture(
		*this,
		TEXT("Ordered placeholders are rejected"),
		TEXT("Key,SourceString,Comment\n")
		TEXT("SaveLoad.Slot.Indexed,Sauvegarde {0},Ordered\n"));

	RejectFixture(
		*this,
		TEXT("Legacy printf placeholders are rejected"),
		TEXT("Key,SourceString,Comment\n")
		TEXT("SaveLoad.Slot.Indexed,Sauvegarde %d,Legacy\n"));

	RejectFixture(
		*this,
		TEXT("Unbalanced braces are rejected"),
		TEXT("Key,SourceString,Comment\n")
		TEXT("SaveLoad.Slot.Indexed,Sauvegarde {Index,Invalid\n"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION014StringTableProjectionTest,
	"Game.UI.Localization.LOCALIZATION01.StringTableProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION014StringTableProjectionTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION014;

	const FString Csv =
		TEXT("Key,SourceString,Comment\n")
		TEXT("Interaction.Door.Open,Ouvrir la porte,Interaction\n")
		TEXT("Combat.Hud.EndTurn,Terminer le tour actif,Combat\n");

	TArray<FUILocalizationCsvEntry> Entries;
	if (!ParseFixture(*this, Csv, Entries))
	{
		return false;
	}

	UStringTable* TableAsset = NewObject<UStringTable>(GetTransientPackage());
	if (!TestNotNull(TEXT("Transient String Table can be created"), TableAsset))
	{
		return false;
	}

	TestFalse(
		TEXT("Fresh table does not already match non-empty fixture"),
		FUILocalizationCsvAuthoring::MatchesEntries(*TableAsset, Entries));

	FUILocalizationCsvAuthoring::ApplyEntries(*TableAsset, Entries);
	TestTrue(
		TEXT("First projection exactly matches the CSV fixture"),
		FUILocalizationCsvAuthoring::MatchesEntries(*TableAsset, Entries));

	FUILocalizationCsvAuthoring::ApplyEntries(*TableAsset, Entries);
	TestTrue(
		TEXT("Second identical projection remains exact and idempotent"),
		FUILocalizationCsvAuthoring::MatchesEntries(*TableAsset, Entries));

	TArray<FUILocalizationCsvEntry> ReducedEntries = Entries;
	ReducedEntries.RemoveAt(0);
	FUILocalizationCsvAuthoring::ApplyEntries(*TableAsset, ReducedEntries);
	TestTrue(
		TEXT("Removing a CSV row removes the projected String Table entry"),
		FUILocalizationCsvAuthoring::MatchesEntries(*TableAsset, ReducedEntries));

	FString RemovedSource;
	TestFalse(
		TEXT("Removed key no longer exists in the generated table"),
		TableAsset->GetStringTable()->GetSourceString(FTextKey(TEXT("Interaction.Door.Open")), RemovedSource));

	FString RemainingSource;
	TestTrue(
		TEXT("Remaining key resolves from the generated table"),
		TableAsset->GetStringTable()->GetSourceString(FTextKey(TEXT("Combat.Hud.EndTurn")), RemainingSource));
	TestEqual(TEXT("Remaining source string is exact"), RemainingSource, FString(TEXT("Terminer le tour actif")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
