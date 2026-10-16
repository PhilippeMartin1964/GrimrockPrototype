#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUITEXTCENTRAL012D3LegacyMetadataTest,
	"Game.UI.TextCentral.CENTRAL01.TalentLegacyMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITEXTCENTRAL012D3LegacyMetadataTest::RunTest(const FString&)
{
	const FString CsvPath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Localization/Source/UI.csv"));

	FString Csv;
	if (!TestTrue(TEXT("UI.csv loads"), FFileHelper::LoadFileToString(Csv, *CsvPath)))
	{
		return false;
	}

	const TArray<FString> ForbiddenTokens = {
		FString(TEXT("DA_RPGTalent")) + TEXT("DetailTextSet"),
		FString(TEXT("RPGTalent")) + TEXT("DetailTextSet"),
		FString(TEXT("RPGTalent")) + TEXT("LocalizationText"),
		FString(TEXT("Talent")) + TEXT(".Recipe.")
	};

	for (const FString& Token : ForbiddenTokens)
	{
		TestFalse(
			*FString::Printf(
				TEXT("UI.csv contains no deleted Talent legacy metadata token '%s'"),
				*Token),
			Csv.Contains(Token, ESearchCase::CaseSensitive));
	}

	const FString NeutralTalentComment =
		TEXT("Vocabulaire statique présenté dans l'interface Talent");
	TestTrue(
		TEXT("UI.csv uses neutral current-authority metadata for Talent vocabulary"),
		Csv.Contains(NeutralTalentComment, ESearchCase::CaseSensitive));

	return true;
}

#endif
