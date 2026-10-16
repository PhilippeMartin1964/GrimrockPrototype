#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUITEXTCENTRAL013SkillVocabularySourceGuardTest,
	"Game.UI.TextCentral.CENTRAL01.SkillVocabularySourceGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITEXTCENTRAL013SkillVocabularySourceGuardTest::RunTest(const FString&)
{
	const TArray<FString> TargetFiles = {
		FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Source/GrimrockPrototype/Private/UI/GridSkillEntryWidget.cpp")),
		FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Source/GrimrockPrototype/Private/UI/GridSkillDetailWidget.cpp"))
	};

	FString CombinedSource;
	for (const FString& Filename : TargetFiles)
	{
		FString Source;
		if (!TestTrue(
			*FString::Printf(TEXT("Skill presentation source loads: %s"), *Filename),
			FFileHelper::LoadFileToString(Source, *Filename)))
		{
			continue;
		}
		CombinedSource += Source;
	}

	const TArray<FString> ForbiddenLegacyTokens = {
		TEXT("NSLOCTEXT(\"GridSkills\""),
		TEXT("\"UnknownSkill\""),
		TEXT("\"SkillRankFormat\""),
		TEXT("\"SkillTrained\""),
		TEXT("\"SkillUntrainedAllowed\""),
		TEXT("\"SkillTrainingRequired\"")
	};
	for (const FString& Token : ForbiddenLegacyTokens)
	{
		TestFalse(
			*FString::Printf(TEXT("Skill presentation source contains no legacy token %s"), *Token),
			CombinedSource.Contains(Token, ESearchCase::CaseSensitive));
	}

	const TArray<FString> RequiredKeys = {
		TEXT("Skill.Fallback.Unknown"),
		TEXT("Skill.Rank.Format"),
		TEXT("Skill.Training.Trained"),
		TEXT("Skill.Training.UntrainedAllowed"),
		TEXT("Skill.Training.Required")
	};
	for (const FString& Key : RequiredKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("Skill presentation source uses centralized key %s"), *Key),
			CombinedSource.Contains(Key, ESearchCase::CaseSensitive));
	}

	return true;
}

#endif
