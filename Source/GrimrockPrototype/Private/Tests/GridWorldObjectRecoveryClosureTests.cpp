#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectVisual.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectRecoveryClosureTest,
	"Grimrock.WorldObjects.RECOVERY01.D.FinalArchitectureContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectRecoveryClosureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* MotionStruct = FGridWorldObjectMotion::StaticStruct();
	if (!TestNotNull(TEXT("World-object Motion struct exists"), MotionStruct))
	{
		return false;
	}
	TestNotNull(TEXT("Motion keeps forward Duration"), MotionStruct->FindPropertyByName(TEXT("Duration")));
	TestNotNull(TEXT("Motion exposes ReverseDuration"), MotionStruct->FindPropertyByName(TEXT("ReverseDuration")));

	FGridWorldObjectMotion Motion;
	Motion.Duration = 0.08f;
	Motion.ReverseDuration = 0.0f;
	TestEqual(TEXT("Reverse duration falls back to forward duration"), Motion.GetDuration(true), 0.08f);
	Motion.ReverseDuration = 0.10f;
	TestEqual(TEXT("Explicit reverse duration is honored"), Motion.GetDuration(true), 0.10f);

	UScriptStruct* ConfigStruct = FGridWorldObjectInstanceConfig::StaticStruct();
	if (!TestNotNull(TEXT("InstanceConfig struct exists"), ConfigStruct))
	{
		return false;
	}
	TestNotNull(TEXT("InstanceConfig exposes MovingPartOverrides"), ConfigStruct->FindPropertyByName(TEXT("MovingPartOverrides")));
	TestNotNull(TEXT("InstanceConfig exposes DoorChainMode"), ConfigStruct->FindPropertyByName(TEXT("DoorChainMode")));
	TestNotNull(
		TEXT("InstanceConfig exposes bOverrideChainPullDuration"),
		ConfigStruct->FindPropertyByName(TEXT("bOverrideChainPullDuration")));
	TestNotNull(TEXT("InstanceConfig exposes ChainPullDuration"), ConfigStruct->FindPropertyByName(TEXT("ChainPullDuration")));
	TestNull(TEXT("InstanceConfig never duplicates ChainPullDistance"), ConfigStruct->FindPropertyByName(TEXT("ChainPullDistance")));

	UScriptStruct* OverrideStruct = FGridWorldObjectMovingPartInstanceOverride::StaticStruct();
	if (!TestNotNull(TEXT("Moving-part instance override struct exists"), OverrideStruct))
	{
		return false;
	}
	TestNotNull(TEXT("Moving-part override identifies PartIndex"), OverrideStruct->FindPropertyByName(TEXT("PartIndex")));
	TestNotNull(TEXT("Moving-part override supports LocalTransform"), OverrideStruct->FindPropertyByName(TEXT("LocalTransform")));
	TestNotNull(TEXT("Moving-part override supports MotionAmount"), OverrideStruct->FindPropertyByName(TEXT("MotionAmount")));
	TestNotNull(TEXT("Moving-part override supports MotionDuration"), OverrideStruct->FindPropertyByName(TEXT("MotionDuration")));
	TestNull(TEXT("Moving-part override cannot replace Mesh"), OverrideStruct->FindPropertyByName(TEXT("Mesh")));
	TestNull(TEXT("Moving-part override cannot replace MotionType"), OverrideStruct->FindPropertyByName(TEXT("MotionType")));
	TestNull(TEXT("Moving-part override cannot replace MotionAxis"), OverrideStruct->FindPropertyByName(TEXT("MotionAxis")));
	TestNull(TEXT("Moving-part override cannot replace MotionPivot"), OverrideStruct->FindPropertyByName(TEXT("MotionPivot")));
	TestNull(
		TEXT("Moving-part override cannot replace ReverseDuration"),
		OverrideStruct->FindPropertyByName(TEXT("MotionReverseDuration")));

	UScriptStruct* MonsterSpawnStruct = FGridMonsterSpawnInstance::StaticStruct();
	if (!TestNotNull(TEXT("MonsterSpawn typed placement exists"), MonsterSpawnStruct))
	{
		return false;
	}
	TestNotNull(TEXT("MonsterSpawn keeps CellX"), MonsterSpawnStruct->FindPropertyByName(TEXT("CellX")));
	TestNotNull(TEXT("MonsterSpawn keeps CellY"), MonsterSpawnStruct->FindPropertyByName(TEXT("CellY")));
	TestNotNull(TEXT("MonsterSpawn keeps cardinal Facing"), MonsterSpawnStruct->FindPropertyByName(TEXT("Facing")));
	TestNull(TEXT("MonsterSpawn has no legacy LocalOffset"), MonsterSpawnStruct->FindPropertyByName(TEXT("LocalOffset")));
	TestNull(TEXT("MonsterSpawn has no legacy LocalYaw"), MonsterSpawnStruct->FindPropertyByName(TEXT("LocalYaw")));
	TestNull(TEXT("MonsterSpawn has no world-object InstanceConfig"), MonsterSpawnStruct->FindPropertyByName(TEXT("InstanceConfig")));
	TestNull(TEXT("MonsterSpawn has no DefaultLocalPosition"), MonsterSpawnStruct->FindPropertyByName(TEXT("DefaultLocalPosition")));

	UScriptStruct* LogicStruct = FGridLogicObjectInstance::StaticStruct();
	if (!TestNotNull(TEXT("LogicObject typed placement exists"), LogicStruct))
	{
		return false;
	}
	TestNotNull(TEXT("LogicObject keeps CellX"), LogicStruct->FindPropertyByName(TEXT("CellX")));
	TestNotNull(TEXT("LogicObject keeps CellY"), LogicStruct->FindPropertyByName(TEXT("CellY")));
	TestNull(TEXT("LogicObject has no legacy LocalOffset"), LogicStruct->FindPropertyByName(TEXT("LocalOffset")));
	TestNull(TEXT("LogicObject has no legacy LocalYaw"), LogicStruct->FindPropertyByName(TEXT("LocalYaw")));
	TestNull(TEXT("LogicObject has no world-object InstanceConfig"), LogicStruct->FindPropertyByName(TEXT("InstanceConfig")));
	TestNull(TEXT("LogicObject has no DefaultLocalPosition"), LogicStruct->FindPropertyByName(TEXT("DefaultLocalPosition")));

	TestTrue(
		TEXT("MonsterSpawn routes to its typed bucket"),
		GridLevelPlacement::GetBucket(EGridLevelObjectType::MonsterSpawn) == EGridLevelPlacementBucket::MonsterSpawn);
	TestTrue(
		TEXT("StoryCompanion routes to LogicObject"),
		GridLevelPlacement::GetBucket(EGridLevelObjectType::StoryCompanion) == EGridLevelPlacementBucket::LogicObject);
	TestTrue(
		TEXT("CustomRecruiter routes to LogicObject"),
		GridLevelPlacement::GetBucket(EGridLevelObjectType::CustomRecruiter) == EGridLevelPlacementBucket::LogicObject);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
