#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GrimrockGameInstance.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridEventFeedWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0111TimestampFormatTest,
	"Grimrock.UI.EventHUD01.GameplayTimestamp.Format",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0111TimestampFormatTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(
		TEXT("New game displays day one at midnight"),
		UGridEventFeedWidget::FormatGameplayTimestamp(0).ToString(),
		FString(TEXT("[J 1, 00:00:00]")));

	TestEqual(
		TEXT("Two hours thirty-four minutes keep seconds visible"),
		UGridEventFeedWidget::FormatGameplayTimestamp(2 * 3600 + 34 * 60).ToString(),
		FString(TEXT("[J 1, 02:34:00]")));

	TestEqual(
		TEXT("Seconds are visible in the compact world timestamp"),
		UGridEventFeedWidget::FormatGameplayTimestamp(156).ToString(),
		FString(TEXT("[J 1, 00:02:36]")));

	TestEqual(
		TEXT("Days roll after twenty-four world hours"),
		UGridEventFeedWidget::FormatGameplayTimestamp(86400 + 2 * 3600 + 34 * 60 + 5).ToString(),
		FString(TEXT("[J 2, 02:34:05]")));

	TestEqual(
		TEXT("Negative defensive input is clamped to zero"),
		UGridEventFeedWidget::FormatGameplayTimestamp(-42).ToString(),
		FString(TEXT("[J 1, 00:00:00]")));

	TestEqual(
		TEXT("Last second of first day still displays day one"),
		UGridEventFeedWidget::FormatGameplayTimestamp(86400 - 1).ToString(),
		FString(TEXT("[J 1, 23:59:59]")));

	TestEqual(
		TEXT("Day two starts exactly at twenty-four world hours"),
		UGridEventFeedWidget::FormatGameplayTimestamp(86400).ToString(),
		FString(TEXT("[J 2, 00:00:00]")));

	TestEqual(
		TEXT("Day three starts exactly at forty-eight world hours"),
		UGridEventFeedWidget::FormatGameplayTimestamp(2 * 86400).ToString(),
		FString(TEXT("[J 3, 00:00:00]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0111ClockStateTest,
	"Grimrock.UI.EventHUD01.GameplayTimestamp.ClockState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0111ClockStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockGameInstance* GameInstance = NewObject<UGrimrockGameInstance>(GetTransientPackage());
	if (!TestNotNull(TEXT("Grimrock GameInstance exists"), GameInstance))
	{
		return false;
	}

	TestEqual(TEXT("World-time scale defaults to six"), GameInstance->GetWorldTimeScale(), 6.0);
	TestEqual(TEXT("Ten gameplay seconds become one in-world minute"),
		UGrimrockGameInstance::ScaleGameplayDeltaSeconds(10.0, GameInstance->GetWorldTimeScale()), 60.0);
	TestEqual(TEXT("Twenty-six gameplay seconds become two minutes thirty-six seconds"),
		UGrimrockGameInstance::ScaleGameplayDeltaSeconds(26.0, GameInstance->GetWorldTimeScale()), 156.0);
	TestEqual(TEXT("Negative delta is clamped"),
		UGrimrockGameInstance::ScaleGameplayDeltaSeconds(-5.0, 6.0), 0.0);
	TestEqual(TEXT("Negative time scale is clamped"),
		UGrimrockGameInstance::ScaleGameplayDeltaSeconds(10.0, -6.0), 0.0);

	GameInstance->RestoreGameplayElapsedSeconds(9240.75);
	TestEqual(TEXT("Restored clock keeps whole elapsed seconds"), GameInstance->GetGameplayElapsedWholeSeconds(), int64(9240));
	TestTrue(TEXT("Restored clock keeps fractional precision"),
		FMath::IsNearlyEqual(GameInstance->GetGameplayElapsedSeconds(), 9240.75));
	TestFalse(TEXT("Restore alone never starts gameplay timing"), GameInstance->IsGameplayClockRunning());

	GameInstance->ResetGameplayClock();
	TestEqual(TEXT("New Game reset returns the gameplay clock to zero"), GameInstance->GetGameplayElapsedWholeSeconds(), int64(0));

	GameInstance->RestoreGameplayElapsedSeconds(-50.0);
	TestEqual(TEXT("Invalid negative restored time is clamped to zero"), GameInstance->GetGameplayElapsedWholeSeconds(), int64(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0111SaveSchemaTest,
	"Grimrock.UI.EventHUD01.GameplayTimestamp.SaveSchemaV27",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0111SaveSchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Gameplay timestamp opens exact-match SaveGame v27"),
		UGrimrockPartySaveGame::CurrentSaveVersion, 27);

	const FProperty* Property =
		UGrimrockPartySaveGame::StaticClass()->FindPropertyByName(TEXT("GameplayElapsedSeconds"));
	if (!TestNotNull(TEXT("SaveGame owns GameplayElapsedSeconds"), Property))
	{
		return false;
	}
	TestTrue(TEXT("GameplayElapsedSeconds is durable"), Property->HasAnyPropertyFlags(CPF_SaveGame));

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->GameplayElapsedSeconds = 9240.5;
	TestTrue(TEXT("Finite positive gameplay time is structurally valid"),
		FMath::IsFinite(Save->GameplayElapsedSeconds) && Save->GameplayElapsedSeconds >= 0.0);

	Save->GameplayElapsedSeconds = -1.0;
	FText Error;
	TestFalse(TEXT("Negative gameplay time is rejected by save validation"), Save->ValidateCurrentState(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0111TimestampStyleTest,
	"Grimrock.UI.EventHUD01.GameplayTimestamp.Style",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0111TimestampStyleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridEventFeedWidget* Widget = NewObject<UGridEventFeedWidget>();
	if (!TestNotNull(TEXT("Event Feed widget exists"), Widget))
	{
		return false;
	}

	TestTrue(TEXT("Timestamp colour is visible"), Widget->TimestampColor.A > 0.0f);
	TestFalse(TEXT("Timestamp colour is distinct from neutral information"),
		Widget->TimestampColor.Equals(Widget->SemanticPalette.Information));
	TestFalse(TEXT("Timestamp colour is distinct from physical combat"),
		Widget->TimestampColor.Equals(Widget->SemanticPalette.PhysicalDamage));
	TestFalse(TEXT("Timestamp colour is distinct from dialogue"),
		Widget->TimestampColor.Equals(Widget->SemanticPalette.Dialogue));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0111EventPayloadTest,
	"Grimrock.UI.EventHUD01.GameplayTimestamp.EventPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0111EventPayloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridHudEventEntry Entry;
	TestFalse(TEXT("Unpublished entries start without a timestamp"), Entry.bHasGameplayTimestamp);
	TestEqual(TEXT("Unpublished entries start at zero seconds"), Entry.GameplayElapsedSeconds, int64(0));

	Entry.bHasGameplayTimestamp = true;
	Entry.GameplayElapsedSeconds = 9240;
	TestEqual(
		TEXT("Timestamp payload formats independently from semantic content"),
		UGridEventFeedWidget::FormatGameplayTimestamp(Entry.GameplayElapsedSeconds).ToString(),
		FString(TEXT("[J 1, 02:34:00]")));

	const FProperty* TimestampField =
		FGridHudEventEntry::StaticStruct()->FindPropertyByName(TEXT("GameplayElapsedSeconds"));
	if (TestNotNull(TEXT("Event payload exposes GameplayElapsedSeconds"), TimestampField))
	{
		TestFalse(TEXT("Event timestamps remain runtime-only and are never SaveGame fields"),
			TimestampField->HasAnyPropertyFlags(CPF_SaveGame));
	}
	return true;
}

#endif
