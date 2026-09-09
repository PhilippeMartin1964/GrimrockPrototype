#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorMON1923LogicPolicyTest, "Grimrock.MON19.2.Editor.LogicPolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON1923LogicPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const EGridLogicNodeType Relay = EGridLogicNodeType::Relay;
	const TArray<EGridObjectEvent> RelayEvents = GridEditorLinkPolicy::GetSupportedEventsForSource(EGridLevelObjectType::Logic, Relay);
	TestEqual(TEXT("Relay exposes one source event"), RelayEvents.Num(), 1);
	TestTrue(TEXT("Relay emits Activated"), RelayEvents.Contains(EGridObjectEvent::Activated));
	const TArray<EGridObjectCommand> RelayCommands = GridEditorLinkPolicy::GetSupportedCommandsForTarget(EGridLevelObjectType::Logic, Relay);
	TestEqual(TEXT("Relay exposes one target command"), RelayCommands.Num(), 1);
	TestTrue(TEXT("Relay receives LogicExecute"), RelayCommands.Contains(EGridObjectCommand::LogicExecute));

	const EGridLogicNodeType Compare = EGridLogicNodeType::CompareInt;
	const TArray<EGridObjectEvent> CompareEvents = GridEditorLinkPolicy::GetSupportedEventsForSource(EGridLevelObjectType::Logic, Compare);
	TestEqual(TEXT("Comparator exposes true and false events"), CompareEvents.Num(), 2);
	TestTrue(TEXT("Comparator exposes Activated"), CompareEvents.Contains(EGridObjectEvent::Activated));
	TestTrue(TEXT("Comparator exposes Deactivated"), CompareEvents.Contains(EGridObjectEvent::Deactivated));

	const EGridLogicNodeType Latch = EGridLogicNodeType::Latch;
	const TArray<EGridObjectCommand> LatchCommands = GridEditorLinkPolicy::GetSupportedCommandsForTarget(EGridLevelObjectType::Logic, Latch);
	TestEqual(TEXT("Latch exposes execute and reset"), LatchCommands.Num(), 2);
	TestTrue(TEXT("Latch receives LogicExecute"), LatchCommands.Contains(EGridObjectCommand::LogicExecute));
	TestTrue(TEXT("Latch receives LogicReset"), LatchCommands.Contains(EGridObjectCommand::LogicReset));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorMON1923LogicRuntimeSupportTest, "Grimrock.MON19.2.Editor.LogicRuntimeSupport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON1923LogicRuntimeSupportTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const EGridLogicNodeType Relay = EGridLogicNodeType::Relay;
	TestTrue(TEXT("LogicExecute is Gameplay runtime support"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(EGridLevelObjectType::Logic, Relay, EGridObjectCommand::LogicExecute) == EGridEditorCommandRuntimeSupport::Gameplay);
	TestTrue(TEXT("Relay rejects LogicReset"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(EGridLevelObjectType::Logic, Relay, EGridObjectCommand::LogicReset) == EGridEditorCommandRuntimeSupport::Unsupported);

	const EGridLogicNodeType Latch = EGridLogicNodeType::Latch;
	TestTrue(TEXT("Latch reset is Gameplay runtime support"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(EGridLevelObjectType::Logic, Latch, EGridObjectCommand::LogicReset) == EGridEditorCommandRuntimeSupport::Gameplay);
	TestTrue(TEXT("Logic rejects generic Toggle"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(EGridLevelObjectType::Logic, Latch, EGridObjectCommand::Toggle) == EGridEditorCommandRuntimeSupport::Unsupported);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
