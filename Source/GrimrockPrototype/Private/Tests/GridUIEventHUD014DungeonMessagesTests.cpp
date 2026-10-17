#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridEventFeedPublisher.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD014ObservabilityTest, "Grimrock.UI.EventHUD01.DungeonMessages.Observability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD014ObservabilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FIntPoint PartyCell(10, 10);
	TestTrue(TEXT("Same-cell dungeon effect is observable"),
		GridEventFeedPublisher::IsCellObservable(PartyCell, FIntPoint(10, 10)));
	TestTrue(TEXT("Adjacent cardinal dungeon effect is observable"),
		GridEventFeedPublisher::IsCellObservable(PartyCell, FIntPoint(11, 10)));
	TestFalse(TEXT("Two-cell remote effect is hidden by conservative anti-spoiler policy"),
		GridEventFeedPublisher::IsCellObservable(PartyCell, FIntPoint(12, 10)));
	TestFalse(TEXT("Diagonal effect is not assumed observable without a dedicated perception rule"),
		GridEventFeedPublisher::IsCellObservable(PartyCell, FIntPoint(11, 11)));
	TestTrue(TEXT("Explicit larger window can admit a two-cell effect"),
		GridEventFeedPublisher::IsCellObservable(PartyCell, FIntPoint(12, 10), 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD014SourceIdentityTest, "Grimrock.UI.EventHUD01.DungeonMessages.SourceIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD014SourceIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGuid SourceGuid(0x12345678, 0x90abcdef, 0x13572468, 0x24681357);
	const FName SourceId = GridEventFeedPublisher::SourceIdFromGuid(SourceGuid);
	TestFalse(TEXT("Valid runtime GUID maps to a stable non-empty source id"), SourceId.IsNone());
	TestEqual(TEXT("Source id uses compact GUID digits"),
		SourceId.ToString(),
		SourceGuid.ToString(EGuidFormats::Digits));
	TestTrue(TEXT("Invalid GUID maps to NAME_None"),
		GridEventFeedPublisher::SourceIdFromGuid(FGuid()).IsNone());
	return true;
}

#endif
