#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridSkillDetailWidget.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/UILocalizationText.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUITEXTCENTRAL013SkillVocabularyTest,
	"Game.UI.TextCentral.CENTRAL01.SkillRemainingVocabulary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITEXTCENTRAL013SkillVocabularyTest::RunTest(const FString&)
{
	TestTrue(
		TEXT("Unknown Skill fallback resolves from ST_UI"),
		FUILocalizationText::Get(TEXT("Skill.Fallback.Unknown"))
			.EqualTo(FText::FromString(TEXT("Compétence inconnue"))));
	TestTrue(
		TEXT("Trained state resolves from ST_UI"),
		FUILocalizationText::Get(TEXT("Skill.Training.Trained"))
			.EqualTo(FText::FromString(TEXT("Entraînée"))));
	TestTrue(
		TEXT("Untrained-allowed state resolves from ST_UI"),
		FUILocalizationText::Get(TEXT("Skill.Training.UntrainedAllowed"))
			.EqualTo(FText::FromString(TEXT("Utilisable sans entraînement"))));
	TestTrue(
		TEXT("Training-required state resolves from ST_UI"),
		FUILocalizationText::Get(TEXT("Skill.Training.Required"))
			.EqualTo(FText::FromString(TEXT("Entraînement requis"))));

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Rank"), FText::AsNumber(2));
	Arguments.Add(TEXT("MaxRank"), FText::AsNumber(5));
	TestTrue(
		TEXT("Rank format uses named ST_UI placeholders"),
		FText::Format(
			FUILocalizationText::Get(TEXT("Skill.Rank.Format")),
			Arguments)
			.EqualTo(FText::FromString(TEXT("Rang 2 / 5"))));

	FGridSkillEntryView MissingAuthoredName;
	MissingAuthoredName.SkillId = TEXT("Skill_InternalOnly");
	MissingAuthoredName.Rank = 2;
	MissingAuthoredName.MaxRank = 5;
	TestTrue(
		TEXT("Missing authored Skill name uses localized fallback instead of technical id"),
		UGridSkillEntryWidget::ResolveDisplayName(MissingAuthoredName)
			.EqualTo(FText::FromString(TEXT("Compétence inconnue"))));

	TestTrue(
		TEXT("Compact row rank formatter consumes centralized vocabulary"),
		UGridSkillEntryWidget::ResolveRankLabel(MissingAuthoredName)
			.EqualTo(FText::FromString(TEXT("Rang 2 / 5"))));
	TestTrue(
		TEXT("Detail rank formatter consumes centralized vocabulary"),
		UGridSkillDetailWidget::ResolveRankLabel(2, 5)
			.EqualTo(FText::FromString(TEXT("Rang 2 / 5"))));
	TestTrue(
		TEXT("Detail training formatter consumes centralized vocabulary"),
		UGridSkillDetailWidget::ResolveTrainingLabel(EGridSkillTrainingState::TrainingRequired)
			.EqualTo(FText::FromString(TEXT("Entraînement requis"))));

	return true;
}

#endif
