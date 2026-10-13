#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Internationalization/Regex.h"
#include "Localization/UILocalizationCsvAuthoring.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION0111
{
	const TCHAR* WidgetSourcePath =
		TEXT("Source/GrimrockPrototype/Private/UI/RPGStoryCompanionRecruitmentWidget.cpp");
	const TCHAR* WidgetHeaderPath =
		TEXT("Source/GrimrockPrototype/Public/UI/RPGStoryCompanionRecruitmentWidget.h");
	const TCHAR* RegistrationServicePath =
		TEXT("Source/GrimrockPrototype/Private/RPG/RPGStoryCompanionService.cpp");
	const TCHAR* RecruitmentServicePath =
		TEXT("Source/GrimrockPrototype/Private/RPG/RPGPartyRecruitmentService.cpp");

	const TCHAR* RequiredKeys[] = {
		TEXT("Recruitment.Activation.AmbiguousCandidate"),
		TEXT("Recruitment.Activation.CandidateNotFound"),
		TEXT("Recruitment.Activation.DuplicateActiveCharacter"),
		TEXT("Recruitment.Activation.Failed"),
		TEXT("Recruitment.Activation.InitialCharacterMissing"),
		TEXT("Recruitment.Activation.InvalidCandidate"),
		TEXT("Recruitment.Activation.InvalidCharacterId"),
		TEXT("Recruitment.Activation.InvalidInventory"),
		TEXT("Recruitment.Activation.InvalidPartyState"),
		TEXT("Recruitment.Activation.OwnershipValidationFailed"),
		TEXT("Recruitment.Activation.PartyFull"),
		TEXT("Recruitment.Registration.Failed"),
		TEXT("Recruitment.Registration.IdentityCollision"),
		TEXT("Recruitment.Registration.InvalidDefinition"),
		TEXT("Recruitment.Registration.InvalidInventory")
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION0111RecruitmentErrorBoundaryTest,
	"Game.UI.Localization.LOCALIZATION01.RecruitmentErrorBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION0111RecruitmentErrorBoundaryTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION0111;

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
			*FString::Printf(TEXT("UI.csv contains Recruitment key '%s'"), RequiredKey),
			CsvKeys.Contains(RequiredKey));
	}

	auto LoadSource =
		[this](const TCHAR* RelativePath, FString& OutSource) -> bool
		{
			const FString Filename = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
			if (!FFileHelper::LoadFileToString(OutSource, *Filename))
			{
				AddError(FString::Printf(TEXT("Unable to read recruitment boundary source '%s'."), *Filename));
				return false;
			}
			return true;
		};

	FString WidgetSource;
	FString WidgetHeader;
	FString RegistrationService;
	FString RecruitmentService;
	if (!LoadSource(WidgetSourcePath, WidgetSource) ||
		!LoadSource(WidgetHeaderPath, WidgetHeader) ||
		!LoadSource(RegistrationServicePath, RegistrationService) ||
		!LoadSource(RecruitmentServicePath, RecruitmentService))
	{
		return false;
	}

	TestFalse(
		TEXT("Recruitment widget never converts raw service Error directly to FText"),
		WidgetSource.Contains(TEXT("FText::FromString(Error)")));
	TestFalse(
		TEXT("Registration UI no longer passes diagnostic Error to presentation mapper"),
		WidgetSource.Contains(TEXT("BuildRegistrationFailureText(RegistrationResult.Error)")));
	TestFalse(
		TEXT("Recruitment UI no longer passes diagnostic Error to presentation mapper"),
		WidgetSource.Contains(TEXT("BuildRecruitmentFailureText(static_cast<int32>(RecruitmentResult.RejectReason), RecruitmentResult.Error)")));

	TestTrue(
		TEXT("Registration diagnostic remains available to logs"),
		WidgetSource.Contains(TEXT("*RegistrationResult.Error")));
	TestTrue(
		TEXT("Recruitment diagnostic remains available to logs"),
		WidgetSource.Contains(TEXT("*RecruitmentResult.Error")));

	TestTrue(
		TEXT("Registration mapper accepts stable registration status"),
		WidgetHeader.Contains(TEXT("BuildRegistrationFailureText(ERPGStoryCompanionRegistrationStatus Status)")));
	TestTrue(
		TEXT("Recruitment mapper accepts stable reject reason"),
		WidgetHeader.Contains(TEXT("BuildRecruitmentFailureText(ERPGPartyRecruitmentRejectReason RejectReason)")));
	TestFalse(
		TEXT("Presentation mapper signatures no longer accept raw FString diagnostics"),
		WidgetHeader.Contains(TEXT("BuildRegistrationFailureText(const FString& Error)")) ||
			WidgetHeader.Contains(TEXT("BuildRecruitmentFailureText(int32 RejectReasonValue, const FString& Error)")));

	TestTrue(
		TEXT("Story companion service still owns technical diagnostics"),
		RegistrationService.Contains(TEXT("OutResult.Error = TEXT(")));
	TestTrue(
		TEXT("Party recruitment service still owns technical diagnostics"),
		RecruitmentService.Contains(TEXT("OutResult.Error = TEXT(")));

	const FRegexPattern KeyPattern(TEXT("TEXT\\(\\\"(Recruitment\\.[A-Za-z0-9.]+)\\\"\\)"));
	FRegexMatcher Matcher(KeyPattern, WidgetSource);
	TSet<FString> ReferencedKeys;
	while (Matcher.FindNext())
	{
		ReferencedKeys.Add(Matcher.GetCaptureGroup(1));
	}

	TestEqual(
		TEXT("Recruitment widget references exactly the authored Recruitment keys"),
		ReferencedKeys.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(RequiredKeys)));

	for (const TCHAR* RequiredKey : RequiredKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("Recruitment widget references key '%s'"), RequiredKey),
			ReferencedKeys.Contains(RequiredKey));
	}

	FString EnglishFixture;
	const FString EnglishFixturePath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/Localization/ApplyEnglishSmokeTranslations.ps1"));
	if (FFileHelper::LoadFileToString(EnglishFixture, *EnglishFixturePath))
	{
		TestTrue(TEXT("English fixture covers registration collision"), EnglishFixture.Contains(TEXT("Recruitment.Registration.IdentityCollision")));
		TestTrue(TEXT("English fixture covers invalid candidate"), EnglishFixture.Contains(TEXT("Recruitment.Activation.InvalidCandidate")));
		TestTrue(TEXT("English fixture covers full party"), EnglishFixture.Contains(TEXT("Recruitment.Activation.PartyFull")));
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
		TestTrue(TEXT("Runtime smoke covers registration boundary"), SmokeCommandlet.Contains(TEXT("Recruitment.Registration.InvalidInventory")));
		TestTrue(TEXT("Runtime smoke covers activation boundary"), SmokeCommandlet.Contains(TEXT("Recruitment.Activation.CandidateNotFound")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read runtime smoke commandlet '%s'."), *SmokePath));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
