#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyProgressionReminderState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridPartyProgressionReminderStateTest,
	"Grimrock.UI.EventHUD01.ProgressionReminder.Cadence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyProgressionReminderStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridPartyProgressionReminderState Reminder;
	const FGuid Elias(1, 0, 0, 1);
	const FGuid Mina(1, 0, 0, 2);
	TMap<FGuid, int32> Pending;

	TestFalse(TEXT("No points means no reminder"), Reminder.Advance(Pending, 3600.f));
	Pending.Add(Elias, 1);
	TestTrue(TEXT("New unspent points announce immediately"), Reminder.Advance(Pending, 0.f));
	TestFalse(TEXT("No duplicate on next frame"), Reminder.Advance(Pending, 0.016f));
	TestFalse(TEXT("No reminder before 300 active seconds"), Reminder.Advance(Pending, 299.f));
	TestTrue(TEXT("Five-minute reminder fires"), Reminder.Advance(Pending, 1.f));
	TestFalse(TEXT("Reminder does not spam each frame"), Reminder.Advance(Pending, 0.016f));

	Pending[Elias] = 2;
	TestTrue(TEXT("Another level announces immediately even if points already remained"),
		Reminder.Advance(Pending, 0.0f));
	Pending.Add(Mina, 1);
	TestTrue(TEXT("Another character gaining points announces immediately"), Reminder.Advance(Pending, 0.f));
	TestFalse(TEXT("Pause/zero elapsed time does not create a reminder"), Reminder.Advance(Pending, 0.f));
	Pending.Remove(Elias);
	TestTrue(TEXT("Changed pending roster refreshes the notice"), Reminder.Advance(Pending, 0.f));
	Pending.Reset();
	TestFalse(TEXT("Spending all points stops reminders"), Reminder.Advance(Pending, 600.f));
	Pending.Add(Elias, 2);
	TestTrue(TEXT("New points after an empty interval announce immediately"), Reminder.Advance(Pending, 0.f));
	return true;
}
#endif
