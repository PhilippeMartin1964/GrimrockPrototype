#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentOptionalLocalizationPresenterTest,
	"Grimrock.UI.RPG.DESC01.Detail.OptionalLocalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentOptionalLocalizationPresenterTest::RunTest(const FString&)
{
	FString Source;
	const FString SourcePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Source/GrimrockPrototype/Private/UI/GridTalentDetailWidget.cpp"));
	if (!TestTrue(
		TEXT("GridTalentDetailWidget.cpp loads"),
		FFileHelper::LoadFileToString(Source, *SourcePath)))
	{
		return false;
	}

	TestTrue(
		TEXT("Optional Talent detail labels use the optional localization resolver"),
		Source.Contains(TEXT("FUILocalizationText::GetOptional(Line.LabelTextId)")));
	TestFalse(
		TEXT("Optional Talent detail labels never use the strict localization resolver"),
		Source.Contains(TEXT("FUILocalizationText::Get(Line.LabelTextId)")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
