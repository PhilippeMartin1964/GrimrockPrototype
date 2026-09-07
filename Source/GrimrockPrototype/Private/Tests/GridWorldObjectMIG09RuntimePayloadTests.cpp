#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectArchetypeAsset.h"
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
	TestEqual(TEXT("Typed definition id maps to runtime archetype lookup id"), RuntimeData.ArchetypeId, Instance.WorldObjectDefinitionId);
	TestEqual(TEXT("Typed CellX maps to runtime"), RuntimeData.CellX, 7);
	TestEqual(TEXT("Typed CellY maps to runtime"), RuntimeData.CellY, 9);
	TestEqual(TEXT("Typed WallSide maps to runtime Edge"), RuntimeData.Edge, EGridEdge::West);
	TestTrue(TEXT("Typed initial active state maps to runtime"), RuntimeData.bInitiallyActive);
	TestFalse(TEXT("Typed Pit state remains instance-owned"), RuntimeData.Behavior.Pit.bInitiallyOpen);
	TestTrue(TEXT("Typed transition remains instance-owned"), RuntimeData.Behavior.Transition.bIsTransition);
	TestEqual(TEXT("Typed transition target maps to runtime"), RuntimeData.Behavior.Transition.TargetLevelId, FName(TEXT("LowerLevel")));
	TestTrue(TEXT("Typed lock initial state remains instance-owned"), RuntimeData.Behavior.Lock.bStartsUnlocked);

	UGridObjectArchetypeAsset* Definition = NewObject<UGridObjectArchetypeAsset>();
	Definition->ArchetypeId = Instance.WorldObjectDefinitionId;
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

	FGridLevelObjectData LegacySnapshot;
	LegacySnapshot.ObjectId = Instance.InstanceId;
	LegacySnapshot.ArchetypeId = Instance.WorldObjectDefinitionId;
	LegacySnapshot.Type = Instance.Type;
	LegacySnapshot.CellX = Instance.CellX;
	LegacySnapshot.CellY = Instance.CellY;
	LegacySnapshot.Edge = Instance.WallSide;
	LegacySnapshot.bInitiallyActive = true;
	LegacySnapshot.Behavior.Pit.bInitiallyOpen = false;
	const FGridRuntimeWorldObjectData BridgedData = LegacySnapshot;
	TestEqual(TEXT("Temporary E2 legacy bridge preserves ObjectId"), BridgedData.ObjectId, LegacySnapshot.ObjectId);
	TestEqual(TEXT("Temporary E2 legacy bridge preserves ArchetypeId"), BridgedData.ArchetypeId, LegacySnapshot.ArchetypeId);
	TestFalse(TEXT("Temporary E2 legacy bridge preserves behavior snapshot"), BridgedData.Behavior.Pit.bInitiallyOpen);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
