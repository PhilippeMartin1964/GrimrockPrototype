#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGLevelUpService.h"
#include "RPG/RPGProgressionPIESimulator.h"
#include "RPGMON155TestHelpers.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridPersistentHudWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01AutoAcknowledgementTest,
	"Grimrock.RPG.LEVELUX01.LevelUp.AutoAcknowledgement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01AutoAcknowledgementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 1000, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.LastAcknowledgedLevel = 1;

	TestTrue(TEXT("Canonical Level-Up applies"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0, false));
	TestEqual(TEXT("Character reaches level two"), Character.Level, 2);
	TestEqual(TEXT("Non-modal Level-Up is acknowledged in the same transaction"), Character.LastAcknowledgedLevel, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01SkillOnlyFeedbackTest,
	"Grimrock.RPG.LEVELUX01.Feedback.SkillOnlyLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01SkillOnlyFeedbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeLevelUpNotification(
			FText::FromString(TEXT("Elias")),
			4,
			5,
			1,
			0,
			3);

	TestEqual(TEXT("Title names the reached level"), Notification.Title.ToString(), FString(TEXT("Niveau 5 atteint")));
	TestTrue(TEXT("Odd level feedback contains the Skill Point"), Notification.Message.ToString().Contains(TEXT("compétence")));
	TestFalse(TEXT("Odd level feedback does not invent a Talent Point"), Notification.Message.ToString().Contains(TEXT("talent")));
	TestTrue(TEXT("Rank-cap milestone is visible"), Notification.Message.ToString().Contains(TEXT("3")));
	TestEqual(TEXT("Level-Up toast lasts five seconds"), Notification.DurationSeconds, 5.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01SkillTalentFeedbackTest,
	"Grimrock.RPG.LEVELUX01.Feedback.SkillAndTalentLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01SkillTalentFeedbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeLevelUpNotification(
			FText::FromString(TEXT("Elias")),
			5,
			6,
			1,
			1,
			0);

	TestTrue(TEXT("Even level feedback contains the Skill Point"), Notification.Message.ToString().Contains(TEXT("compétence")));
	TestTrue(TEXT("Even level feedback contains the Talent Point"), Notification.Message.ToString().Contains(TEXT("talent")));
	TestFalse(TEXT("No rank-cap line is fabricated"), Notification.Message.ToString().Contains(TEXT("Rang maximal")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01PersistentHudContractTest,
	"Grimrock.RPG.LEVELUX01.PersistentHud.NotificationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01PersistentHudContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* HudClass = UGridPersistentHudWidget::StaticClass();
	TestNotNull(
		TEXT("Persistent HUD exposes the optional progression toast binding"),
		FindFProperty<FProperty>(HudClass, FName(TEXT("Notification_Progression"))));
	TestNotNull(
		TEXT("Persistent HUD exposes the non-modal notification entry point"),
		HudClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridPersistentHudWidget, ShowProgressionNotification)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01SimulatorSuccessiveLevelsTest,
	"Grimrock.RPG.LEVELUX01.PIESimulator.SuccessiveLevels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01SimulatorSuccessiveLevelsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 0, ClassDefinition);

	FText Feedback;
	TestTrue(TEXT("PIE simulator reaches level two"), FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 2, Feedback));
	TestTrue(TEXT("PIE simulator immediately continues to level six"), FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 6, Feedback));

	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Successive simulation reaches level six"), Character.Level, 6);
	TestEqual(TEXT("Successive simulation needs no modal acknowledgement"), Character.LastAcknowledgedLevel, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01NoModalRuntimePathTest,
	"Grimrock.RPG.LEVELUX01.Runtime.NoModalCreationPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01NoModalRuntimePathTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FString HeaderText;
	FString SourceText;
	TestTrue(
		TEXT("Level-Up subsystem header loads"),
		FFileHelper::LoadFileToString(
			HeaderText,
			*FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Public/RPG/RPGLevelUpNotificationSubsystem.h"))));
	TestTrue(
		TEXT("Level-Up subsystem source loads"),
		FFileHelper::LoadFileToString(
			SourceText,
			*FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/RPG/RPGLevelUpNotificationSubsystem.cpp"))));

	TestFalse(TEXT("Runtime subsystem no longer owns URPGLevelUpWidget"), HeaderText.Contains(TEXT("URPGLevelUpWidget")));
	TestFalse(TEXT("Runtime subsystem no longer creates the Level-Up modal"), SourceText.Contains(TEXT("CreateWidget<URPGLevelUpWidget>")));
	TestFalse(TEXT("Runtime subsystem no longer defers feedback until combat ends"), HeaderText.Contains(TEXT("DeferredCombatTurnManager")));
	TestTrue(TEXT("Runtime subsystem still owns a transient toast queue"), HeaderText.Contains(TEXT("PendingNotifications")));
	return true;
}

#endif
