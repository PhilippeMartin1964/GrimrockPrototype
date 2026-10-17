#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridEventFeedSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD011BufferTest, "Grimrock.UI.EventHUD01.Core.Buffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD011BufferTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridEventFeedBuffer Buffer;
	TestEqual(TEXT("Default capacity is 200 logical events"), Buffer.GetMaxEntries(), 200);

	for (int32 Index = 1; Index <= 205; ++Index)
	{
		FGridHudEventEntry Entry;
		Entry.EventCode = FName(*FString::Printf(TEXT("Event_%03d"), Index));
		Entry.Message = FText::FromString(FString::Printf(TEXT("Message %d"), Index));
		TestTrue(TEXT("Observable event is accepted"), Buffer.TryPublish(Entry, true));
	}

	const TArray<FGridHudEventEntry>& Entries = Buffer.GetEntries();
	TestEqual(TEXT("FIFO keeps only 200 entries"), Entries.Num(), 200);
	if (Entries.Num() == 200)
	{
		TestEqual(TEXT("Oldest five entries are evicted"), Entries[0].SequenceId, int64(6));
		TestEqual(TEXT("Newest entry keeps its monotonic sequence"), Entries.Last().SequenceId, int64(205));
		TestEqual(TEXT("Oldest retained event is Event_006"), Entries[0].EventCode, FName(TEXT("Event_006")));
		TestEqual(TEXT("Newest retained event is Event_205"), Entries.Last().EventCode, FName(TEXT("Event_205")));
	}

	Buffer.SetMaxEntries(3);
	TestEqual(TEXT("Capacity can shrink at runtime"), Buffer.GetMaxEntries(), 3);
	TestEqual(TEXT("Shrinking retains only newest entries"), Buffer.GetEntries().Num(), 3);
	if (Buffer.GetEntries().Num() == 3)
	{
		TestEqual(TEXT("Newest history survives capacity shrink"), Buffer.GetEntries().Last().SequenceId, int64(205));
		TestEqual(TEXT("Shrink removes from the oldest side"), Buffer.GetEntries()[0].SequenceId, int64(203));
	}

	Buffer.SetMaxEntries(0);
	TestEqual(TEXT("Capacity is clamped to at least one"), Buffer.GetMaxEntries(), 1);
	TestEqual(TEXT("Clamped capacity trims history"), Buffer.GetEntries().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD011VisibilityTest, "Grimrock.UI.EventHUD01.Core.Visibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD011VisibilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridEventFeedBuffer Buffer(8);

	FGridHudEventEntry Hidden;
	Hidden.EventCode = TEXT("HiddenTrigger");
	Hidden.Visibility = EGridEventVisibility::Hidden;
	TestFalse(TEXT("Hidden events never enter the player feed"), Buffer.TryPublish(Hidden, true));

	FGridHudEventEntry Unobserved;
	Unobserved.EventCode = TEXT("RemoteDoor");
	Unobserved.Visibility = EGridEventVisibility::Observable;
	TestFalse(TEXT("Unobserved events are rejected"), Buffer.TryPublish(Unobserved, false));

	FGridHudEventEntry Observed;
	Observed.EventCode = TEXT("NearbyDoor");
	Observed.Visibility = EGridEventVisibility::Observable;
	TestTrue(TEXT("Observed events are accepted"), Buffer.TryPublish(Observed, true));

	FGridHudEventEntry Always;
	Always.EventCode = TEXT("InformationPanel");
	Always.Visibility = EGridEventVisibility::Always;
	TestTrue(TEXT("Always events bypass observation"), Buffer.TryPublish(Always, false));

	TestEqual(TEXT("Rejected events do not consume sequence ids"), Buffer.GetEntries().Num(), 2);
	if (Buffer.GetEntries().Num() == 2)
	{
		TestEqual(TEXT("First accepted entry starts at sequence one"), Buffer.GetEntries()[0].SequenceId, int64(1));
		TestEqual(TEXT("Second accepted entry has sequence two"), Buffer.GetEntries()[1].SequenceId, int64(2));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD011SemanticPayloadTest, "Grimrock.UI.EventHUD01.Core.SemanticPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD011SemanticPayloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridEventFeedBuffer Buffer(4);

	FGridHudEventEntry Entry;
	Entry.Category = EGridHudEventCategory::Combat;
	Entry.Semantic = EGridHudEventSemantic::Fire;
	Entry.EventCode = TEXT("SpellDamage");
	Entry.Header = FText::FromString(TEXT("Elias"));
	Entry.Message = FText::FromString(TEXT("Elias inflige des dégâts de feu."));
	Entry.SourceId = TEXT("Spell_Fireball");

	FGridHudEventTextFragment ActorFragment;
	ActorFragment.Text = FText::FromString(TEXT("Elias inflige "));
	Entry.MessageFragments.Add(ActorFragment);

	FGridHudEventTextFragment DamageFragment;
	DamageFragment.Text = FText::FromString(TEXT("11"));
	DamageFragment.Semantic = EGridHudEventSemantic::PhysicalDamage;
	DamageFragment.SemanticId = TEXT("DamageValue");
	Entry.MessageFragments.Add(DamageFragment);

	FGridHudEventTextFragment FireFragment;
	FireFragment.Text = FText::FromString(TEXT(" dégâts de FEU."));
	FireFragment.Semantic = EGridHudEventSemantic::Fire;
	FireFragment.SemanticId = TEXT("DamageType.Fire");
	Entry.MessageFragments.Add(FireFragment);

	Entry.Details.Add(FText::FromString(TEXT("1d8 : 6")));

	FGridHudEventEntry Published;
	TestTrue(TEXT("Semantic payload is publishable"), Buffer.TryPublish(Entry, true, &Published));
	TestEqual(TEXT("Category is preserved"), Published.Category, EGridHudEventCategory::Combat);
	TestEqual(TEXT("Event semantic is preserved"), Published.Semantic, EGridHudEventSemantic::Fire);
	TestEqual(TEXT("Semantic fragments are preserved"), Published.MessageFragments.Num(), 3);
	TestEqual(TEXT("Detail lines remain part of one logical entry"), Published.Details.Num(), 1);
	if (Published.MessageFragments.Num() == 3)
	{
		TestEqual(TEXT("Future tooltip semantic id is preserved"), Published.MessageFragments[2].SemanticId, FName(TEXT("DamageType.Fire")));
	}

	TestTrue(TEXT("Event Feed authority remains a UWorldSubsystem"),
		UGridEventFeedSubsystem::StaticClass()->IsChildOf(UWorldSubsystem::StaticClass()));
	return true;
}

#endif
