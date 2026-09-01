#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorPIT03LinkPolicyTest, "Grimrock.Pit.PIT03.EditorLinkPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorPIT03LinkPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridLevelObjectData Pit;
	Pit.ObjectId = FGuid::NewGuid();
	Pit.Type = EGridLevelObjectType::Pit;

	const TArray<EGridObjectCommand> Commands = GridEditorLinkPolicy::GetSupportedCommandsForTarget(Pit);
	TestEqual(TEXT("Pit exposes five state commands"), Commands.Num(), 5);
	TestTrue(TEXT("Pit supports Open"), Commands.Contains(EGridObjectCommand::Open));
	TestTrue(TEXT("Pit supports Close"), Commands.Contains(EGridObjectCommand::Close));
	TestTrue(TEXT("Pit supports Toggle"), Commands.Contains(EGridObjectCommand::Toggle));
	TestTrue(TEXT("Pit supports Activate"), Commands.Contains(EGridObjectCommand::Activate));
	TestTrue(TEXT("Pit supports Deactivate"), Commands.Contains(EGridObjectCommand::Deactivate));

	const TArray<EGridObjectEvent> Events = GridEditorLinkPolicy::GetSupportedEventsForSource(Pit);
	TestEqual(TEXT("Pit emits Opened and Closed"), Events.Num(), 2);
	TestTrue(TEXT("Pit emits Opened"), Events.Contains(EGridObjectEvent::Opened));
	TestTrue(TEXT("Pit emits Closed"), Events.Contains(EGridObjectEvent::Closed));

	for (const EGridObjectCommand Command : Commands)
	{
		TestEqual(TEXT("Every Pit command has gameplay runtime support"),
			GridEditorLinkPolicy::GetCommandRuntimeSupport(Pit, Command), EGridEditorCommandRuntimeSupport::Gameplay);
	}

	TestTrue(TEXT("Pit can receive connectors"), GridEditorLinkPolicy::CanObjectReceiveCommands(Pit));
	TestTrue(TEXT("Pit can emit connectors"), GridEditorLinkPolicy::CanObjectEmitEvents(Pit));
	return true;
}

#endif
