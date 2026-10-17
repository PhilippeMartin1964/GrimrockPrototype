#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsWidgetFeedbackInternal.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UI/UILocalizationText.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS0210SkillErrorBoundaryTest,
	"Grimrock.UI.Skills02.ErrorBoundary.PlayerFacingSkillName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS0210SkillErrorBoundaryTest::RunTest(const FString&)
{
	const FName SkillId(TEXT("Skill_InternalProbe"));

	FGridSkillsPageView View;
	FGridSkillEntryView Entry;
	Entry.SkillId = SkillId;
	Entry.DisplayName = FText::FromString(TEXT("Nom projeté"));
	View.Skills.Add(Entry);

	URPGSkillAsset* Definition = NewObject<URPGSkillAsset>();
	Definition->SkillId = SkillId;
	Definition->DisplayName = FText::FromString(TEXT("Nom canonique"));

	TestTrue(
		TEXT("Projected read-model name has first priority"),
		GridSkillsWidgetFeedback::ResolvePlayerFacingSkillName(
			View,
			SkillId,
			Definition).EqualTo(FText::FromString(TEXT("Nom projeté"))));

	View.Skills[0].DisplayName = FText::GetEmpty();
	TestTrue(
		TEXT("Canonical asset name is used when the projected name is empty"),
		GridSkillsWidgetFeedback::ResolvePlayerFacingSkillName(
			View,
			SkillId,
			Definition).EqualTo(FText::FromString(TEXT("Nom canonique"))));

	View.Skills.Reset();
	TestTrue(
		TEXT("Canonical asset name survives a temporarily missing UI row"),
		GridSkillsWidgetFeedback::ResolvePlayerFacingSkillName(
			View,
			SkillId,
			Definition).EqualTo(FText::FromString(TEXT("Nom canonique"))));

	Definition->DisplayName = FText::GetEmpty();
	const FText Fallback =
		GridSkillsWidgetFeedback::ResolvePlayerFacingSkillName(
			View,
			SkillId,
			Definition);
	TestTrue(
		TEXT("Missing authored presentation uses the centralized localized fallback"),
		Fallback.EqualTo(FUILocalizationText::Get(TEXT("Skill.Fallback.Unknown"))));
	TestFalse(
		TEXT("Localized fallback never exposes the technical SkillId"),
		Fallback.ToString().Contains(SkillId.ToString()));

	Definition->SkillId = TEXT("Skill_Other");
	Definition->DisplayName = FText::FromString(TEXT("Mauvais nom"));
	TestTrue(
		TEXT("Mismatched canonical definition cannot provide another Skill name"),
		GridSkillsWidgetFeedback::ResolvePlayerFacingSkillName(
			View,
			SkillId,
			Definition).EqualTo(FUILocalizationText::Get(TEXT("Skill.Fallback.Unknown"))));

	FRPGSkillPointMutationResult Result;
	Result.bCommitted = true;
	Result.SkillId = SkillId;
	Result.NewRank = 2;
	Result.RemainingPoints = 1;
	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeSkillRankPurchaseNotification(
			Result,
			Fallback);
	TestTrue(
		TEXT("Success notification uses the localized unknown-Skill fallback"),
		Notification.Message.ToString().Contains(Fallback.ToString()));
	TestFalse(
		TEXT("Success notification never leaks the technical SkillId"),
		Notification.Message.ToString().Contains(SkillId.ToString()));

	FGridSkillEntryView ValidEntry;
	ValidEntry.SkillId = TEXT("Skill_Acrobatics");
	ValidEntry.Rank = 2;
	ValidEntry.MaxRank = 5;
	ValidEntry.CurrentRankCap = 2;
	ValidEntry.bTrained = true;
	UGridSkillEntryWidget* Widget = NewObject<UGridSkillEntryWidget>();
	TestTrue(TEXT("Compact row accepts the canonical read-model invariant"), Widget->InitializeSkillEntry(ValidEntry));

	FGridSkillEntryView InvalidEntry = ValidEntry;
	InvalidEntry.Rank = 3;
	TestFalse(TEXT("Canonical read model rejects rank above current cap"), InvalidEntry.IsValid());
	TestFalse(TEXT("Compact row rejects the same invalid read model"), Widget->InitializeSkillEntry(InvalidEntry));

	return true;
}

#endif
