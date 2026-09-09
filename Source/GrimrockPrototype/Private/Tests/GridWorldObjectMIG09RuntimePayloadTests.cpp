#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Runtime/GridRuntimeWorldObjectData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG09RuntimePayloadTest,
	"Grimrock.WorldObjects.MIG09.RuntimeWorldObjectPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG09RuntimePayloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridWorldObjectInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.WorldObjectDefinitionId = TEXT("Door_RuntimePayload");
	Instance.Type = EGridLevelObjectType::Door;
	Instance.CellX = 7;
	Instance.CellY = 9;
	Instance.WallSide = EGridEdge::West;
	Instance.bInitiallyEnabled = true;
	Instance.bInitiallyActive = true;
	Instance.ReadableTextOverride = FText::FromString(TEXT("Runtime override"));
	Instance.InstanceConfig.Pit.bInitiallyOpen = false;
	Instance.InstanceConfig.Transition.bIsTransition = true;
	Instance.InstanceConfig.Transition.TargetLevelId = TEXT("LowerLevel");
	Instance.InstanceConfig.bStartsUnlocked = true;

	const FGridRuntimeWorldObjectData RuntimeData(Instance);
	TestEqual(TEXT("Typed InstanceId maps to runtime ObjectId"), RuntimeData.ObjectId, Instance.InstanceId);
	TestEqual(TEXT("Typed definition id maps to runtime definition lookup id"), RuntimeData.WorldObjectDefinitionId, Instance.WorldObjectDefinitionId);
	TestEqual(TEXT("Typed CellX maps to runtime"), RuntimeData.CellX, 7);
	TestEqual(TEXT("Typed CellY maps to runtime"), RuntimeData.CellY, 9);
	TestEqual(TEXT("Typed WallSide maps to runtime Edge"), RuntimeData.Edge, EGridEdge::West);
	TestTrue(TEXT("Typed initial active state maps to runtime"), RuntimeData.bInitiallyActive);
	TestFalse(TEXT("Typed Pit state remains instance-owned"), RuntimeData.Behavior.Pit.bInitiallyOpen);
	TestTrue(TEXT("Typed transition remains instance-owned"), RuntimeData.Behavior.Transition.bIsTransition);
	TestEqual(TEXT("Typed transition target maps to runtime"), RuntimeData.Behavior.Transition.TargetLevelId, FName(TEXT("LowerLevel")));
	TestTrue(TEXT("Typed lock initial state remains instance-owned"), RuntimeData.Behavior.Lock.bStartsUnlocked);

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>();
	Definition->DefinitionId = Instance.WorldObjectDefinitionId;
	Definition->DefaultBehavior.ButtonAnimation.ButtonHoldTime = 0.75f;
	Definition->DefaultBehavior.DoorAnimation.bHasChainMechanism = true;
	Definition->DefaultBehavior.DoorAnimation.ChainPullDistance = 24.0f;
	Definition->DefaultBehavior.Lock.bConsumeKeyOnUnlock = true;
	Definition->DefaultBehavior.Pit.bInitiallyOpen = true;

	const FGridObjectBehaviorParams Resolved = GridObjectInstanceBehavior::Resolve(RuntimeData, Definition);
	TestEqual(TEXT("Shared button behavior comes from Definition"), Resolved.ButtonAnimation.ButtonHoldTime, 0.75f);
	TestTrue(TEXT("Shared chain behavior comes from Definition"), Resolved.DoorAnimation.bHasChainMechanism);
	TestEqual(TEXT("Shared chain distance comes from Definition"), Resolved.DoorAnimation.ChainPullDistance, 24.0f);
	TestTrue(TEXT("Shared lock consumption rule comes from Definition"), Resolved.Lock.bConsumeKeyOnUnlock);
	TestFalse(TEXT("Instance-owned Pit state overrides Definition"), Resolved.Pit.bInitiallyOpen);
	TestTrue(TEXT("Instance-owned lock initial state overrides Definition"), Resolved.Lock.bStartsUnlocked);

	const FGridObjectBehaviorParams NativeResolved = GridObjectInstanceBehavior::Resolve(Instance, Definition);
	TestEqual(TEXT("Runtime boundary preserves native shared behavior"), Resolved.ButtonAnimation.ButtonHoldTime, NativeResolved.ButtonAnimation.ButtonHoldTime);
	TestEqual(TEXT("Runtime boundary preserves native Pit override"), Resolved.Pit.bInitiallyOpen, NativeResolved.Pit.bInitiallyOpen);
	TestEqual(TEXT("Runtime boundary preserves native lock override"), Resolved.Lock.bStartsUnlocked, NativeResolved.Lock.bStartsUnlocked);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
