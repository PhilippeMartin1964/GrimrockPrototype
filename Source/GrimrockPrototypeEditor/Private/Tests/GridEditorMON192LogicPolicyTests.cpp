#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"

namespace
{
	FGridLevelObjectData MakeEditorLogicNode(EGridLogicNodeType NodeType)
	{
		FGridLevelObjectData ObjectData;
		ObjectData.ObjectId = FGuid(19, 2, 3, 100 + static_cast<int32>(NodeType));
		ObjectData.Type = EGridLevelObjectType::Logic;
		ObjectData.Logic.NodeType = NodeType;
		return ObjectData;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorMON1923LogicPolicyTest, "Grimrock.MON19.2.Editor.LogicPolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON1923LogicPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridLevelObjectData Relay = MakeEditorLogicNode(EGridLogicNodeType::Relay);
	const TArray<EGridObjectEvent> RelayEvents = GridEditorLinkPolicy::GetSupportedEventsForSource(Relay);
	TestEqual(TEXT("Relay exposes one source event"), RelayEvents.Num(), 1);
	TestTrue(TEXT("Relay emits Activated"), RelayEvents.Contains(EGridObjectEvent::Activated));
	const TArray<EGridObjectCommand> RelayCommands = GridEditorLinkPolicy::GetSupportedCommandsForTarget(Relay);
	TestEqual(TEXT("Relay exposes one target command"), RelayCommands.Num(), 1);
	TestTrue(TEXT("Relay receives LogicExecute"), RelayCommands.Contains(EGridObjectCommand::LogicExecute));

	const FGridLevelObjectData Compare = MakeEditorLogicNode(EGridLogicNodeType::CompareInt);
	const TArray<EGridObjectEvent> CompareEvents = GridEditorLinkPolicy::GetSupportedEventsForSource(Compare);
	TestEqual(TEXT("Comparator exposes true and false events"), CompareEvents.Num(), 2);
	TestTrue(TEXT("Comparator exposes Activated"), CompareEvents.Contains(EGridObjectEvent::Activated));
	TestTrue(TEXT("Comparator exposes Deactivated"), CompareEvents.Contains(EGridObjectEvent::Deactivated));

	const FGridLevelObjectData Latch = MakeEditorLogicNode(EGridLogicNodeType::Latch);
	const TArray<EGridObjectCommand> LatchCommands = GridEditorLinkPolicy::GetSupportedCommandsForTarget(Latch);
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

	const FGridLevelObjectData Relay = MakeEditorLogicNode(EGridLogicNodeType::Relay);
	TestTrue(TEXT("LogicExecute is Gameplay runtime support"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(Relay, EGridObjectCommand::LogicExecute) == EGridEditorCommandRuntimeSupport::Gameplay);
	TestTrue(TEXT("Relay rejects LogicReset"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(Relay, EGridObjectCommand::LogicReset) == EGridEditorCommandRuntimeSupport::Unsupported);

	const FGridLevelObjectData Latch = MakeEditorLogicNode(EGridLogicNodeType::Latch);
	TestTrue(TEXT("Latch reset is Gameplay runtime support"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(Latch, EGridObjectCommand::LogicReset) == EGridEditorCommandRuntimeSupport::Gameplay);
	TestTrue(TEXT("Logic rejects generic Toggle"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(Latch, EGridObjectCommand::Toggle) == EGridEditorCommandRuntimeSupport::Unsupported);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
