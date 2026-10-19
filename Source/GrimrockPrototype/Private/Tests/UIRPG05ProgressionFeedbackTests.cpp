#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/GridRPGNotificationWidget.h"
#include "UI/RPGProgressionFeedbackService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG05RejectMessagesTest,
	"Grimrock.UI.RPG05.Feedback.RejectMessages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG05RejectMessagesTest::RunTest(const FString&)
{
	const TArray<ERPGClassProgressionCommitRejectReason> Reasons = {
		ERPGClassProgressionCommitRejectReason::InvalidInventory,
		ERPGClassProgressionCommitRejectReason::InvalidCharacter,
		ERPGClassProgressionCommitRejectReason::InvalidClassDefinition,
		ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection,
		ERPGClassProgressionCommitRejectReason::EmptyRequest,
		ERPGClassProgressionCommitRejectReason::DuplicateRequest,
		ERPGClassProgressionCommitRejectReason::UnknownChoice,
		ERPGClassProgressionCommitRejectReason::AlreadySelected,
		ERPGClassProgressionCommitRejectReason::LevelTooLow,
		ERPGClassProgressionCommitRejectReason::MissingPrerequisite,
		ERPGClassProgressionCommitRejectReason::InsufficientChoicePoints,
		ERPGClassProgressionCommitRejectReason::MutuallyExclusiveChoice
	};

	for (const ERPGClassProgressionCommitRejectReason Reason : Reasons)
	{
		TestFalse(
			TEXT("Every transaction rejection has a French presentation message"),
			FRPGProgressionFeedbackService::GetCommitRejectMessage(Reason).IsEmpty());
	}

	TestEqual(
		TEXT("Level-too-low wording is stable"),
		FRPGProgressionFeedbackService::GetCommitRejectMessage(
			ERPGClassProgressionCommitRejectReason::LevelTooLow).ToString(),
		FString(TEXT("Le niveau requis n'est pas atteint.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG05TalentSuccessNotificationTest,
	"Grimrock.UI.RPG05.Feedback.TalentSuccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG05TalentSuccessNotificationTest::RunTest(const FString&)
{
	FRPGClassProgressionCommitResult Result;
	Result.bCommitted = true;
	Result.RemainingPoints = 2;
	Result.CommittedChoiceIds = { TEXT("Talent_Test") };

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeTalentCommitNotification(
			Result, FText::FromString(TEXT("Posture défensive")));

	TestTrue(TEXT("Success notification is valid"), Notification.IsValid());
	TestEqual(TEXT("Success severity"), Notification.Severity, ERPGProgressionNotificationSeverity::Success);
	TestEqual(TEXT("Success title"), Notification.Title.ToString(), FString(TEXT("Talent acquis")));
	TestTrue(TEXT("Success mentions Talent"), Notification.Message.ToString().Contains(TEXT("Posture défensive")));
	TestTrue(TEXT("Success mentions remaining points"), Notification.Message.ToString().Contains(TEXT("2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG05TalentFailureNotificationTest,
	"Grimrock.UI.RPG05.Feedback.TalentFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG05TalentFailureNotificationTest::RunTest(const FString&)
{
	FRPGClassProgressionCommitResult Result;
	Result.bCommitted = false;
	Result.RejectReason = ERPGClassProgressionCommitRejectReason::InsufficientChoicePoints;

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeTalentCommitNotification(Result, FText::GetEmpty());

	TestEqual(TEXT("Failure severity"), Notification.Severity, ERPGProgressionNotificationSeverity::Error);
	TestEqual(TEXT("Failure title"), Notification.Title.ToString(), FString(TEXT("Acquisition refusée")));
	TestEqual(
		TEXT("Failure exposes authoritative rejection reason"),
		Notification.Message.ToString(),
		FString(TEXT("Il n'y a pas assez de points de talent.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG05SkillAndAttributeFeedbackTest,
	"Grimrock.UI.RPG05.Feedback.SkillAndAttribute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG05SkillAndAttributeFeedbackTest::RunTest(const FString&)
{
	// Purchase feedback still belongs to the non-modal RPG notification panel.
	FRPGSkillPointMutationResult SkillResult;
	SkillResult.bCommitted = true;
	SkillResult.NewRank = 2;
	SkillResult.RemainingPoints = 3;
	const FRPGProgressionNotificationView SkillFeedback =
		FRPGProgressionFeedbackService::MakeSkillRankPurchaseNotification(
			SkillResult, FText::FromString(TEXT("Acrobatie")));
	TestTrue(TEXT("Skill purchase feedback is valid"), SkillFeedback.IsValid());
	TestEqual(TEXT("Skill purchase uses success severity"), SkillFeedback.Severity,
		ERPGProgressionNotificationSeverity::Success);
	TestTrue(TEXT("Skill purchase names the selected skill"),
		SkillFeedback.Message.ToString().Contains(TEXT("Acrobatie")));

	FRPGAttributePointMutationResult AttributeResult;
	AttributeResult.bCommitted = false;
	AttributeResult.RejectReason = ERPGAttributePointMutationRejectReason::AttributeCapReached;
	const FRPGProgressionNotificationView AttributeFeedback =
		FRPGProgressionFeedbackService::MakeAttributePointPurchaseNotification(
			AttributeResult, FText::FromString(TEXT("Force")));
	TestTrue(TEXT("Attribute rejection feedback is valid"), AttributeFeedback.IsValid());
	TestEqual(TEXT("Attribute rejection uses warning severity"), AttributeFeedback.Severity,
		ERPGProgressionNotificationSeverity::Warning);
	TestTrue(TEXT("Attribute rejection explains the cap"),
		AttributeFeedback.Message.ToString().Contains(TEXT("20")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG05NotificationWidgetStateTest,
	"Grimrock.UI.RPG05.Notification.WidgetState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG05NotificationWidgetStateTest::RunTest(const FString&)
{
	UGridRPGNotificationWidget* Widget = NewObject<UGridRPGNotificationWidget>();

	FRPGProgressionNotificationView Notification;
	Notification.Severity = ERPGProgressionNotificationSeverity::Info;
	Notification.Title = FText::FromString(TEXT("Information"));
	Notification.Message = FText::FromString(TEXT("Message"));
	Notification.DurationSeconds = 0.0f;

	Widget->ShowNotification(Notification);
	TestTrue(TEXT("Widget stores active notification"), Widget->HasNotification());
	TestEqual(TEXT("Widget stores title"), Widget->CurrentNotification.Title.ToString(), FString(TEXT("Information")));

	Widget->DismissNotification();
	TestFalse(TEXT("Dismiss clears notification"), Widget->HasNotification());
	return true;
}

#endif
