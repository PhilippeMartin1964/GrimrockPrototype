#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FRecoveryC12TestWorld
	{
		UWorld* World = nullptr;

		FRecoveryC12TestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				FName(*FString::Printf(TEXT("RecoveryC12_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FRecoveryC12TestWorld()
		{
			if (!World)
			{
				return;
			}
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecoveryC12DoorChainInstanceOverridesTest,
	"Grimrock.WorldObjects.RECOVERY01.C12.DoorChainInstanceOverrides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRecoveryC12DoorChainInstanceOverridesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* ConfigStruct = FGridWorldObjectInstanceConfig::StaticStruct();
	const UEnum* ChainModeEnum = StaticEnum<EGridDoorChainMode>();

	if (!TestNotNull(TEXT("InstanceConfig struct exists"), ConfigStruct) ||
		!TestNotNull(TEXT("Door-chain mode enum exists"), ChainModeEnum))
	{
		return false;
	}

	TestNotNull(TEXT("InstanceConfig exposes DoorChainMode"), ConfigStruct->FindPropertyByName(TEXT("DoorChainMode")));
	TestNotNull(
		TEXT("InstanceConfig exposes bOverrideChainPullDuration"),
		ConfigStruct->FindPropertyByName(TEXT("bOverrideChainPullDuration")));
	TestNotNull(TEXT("InstanceConfig exposes ChainPullDuration"), ConfigStruct->FindPropertyByName(TEXT("ChainPullDuration")));
	TestNull(TEXT("InstanceConfig does not expose ChainPullDistance"), ConfigStruct->FindPropertyByName(TEXT("ChainPullDistance")));

	if (ChainModeEnum)
	{
		TestTrue(
			TEXT("Door-chain Inherit mode exists"),
			ChainModeEnum->IsValidEnumValue(static_cast<int64>(EGridDoorChainMode::Inherit)));
		TestTrue(
			TEXT("Door-chain Enabled mode exists"),
			ChainModeEnum->IsValidEnumValue(static_cast<int64>(EGridDoorChainMode::Enabled)));
		TestTrue(
			TEXT("Door-chain Disabled mode exists"),
			ChainModeEnum->IsValidEnumValue(static_cast<int64>(EGridDoorChainMode::Disabled)));
	}

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(GetTransientPackage());
	Definition->DefaultBehavior.DoorAnimation.bHasChainMechanism = true;
	Definition->DefaultBehavior.DoorAnimation.ChainPullDistance = 20.0f;
	Definition->DefaultBehavior.DoorAnimation.ChainPullDuration = 0.25f;

	FGridWorldObjectInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.Type = EGridLevelObjectType::Door;

	// Sparse default: inherit all definition-owned chain values.
	FGridObjectBehaviorParams Resolved = GridObjectInstanceBehavior::Resolve(Instance, Definition);
	TestTrue(TEXT("Inherit keeps definition chain enabled"), Resolved.DoorAnimation.bHasChainMechanism);
	TestEqual(TEXT("Inherit keeps definition chain distance"), Resolved.DoorAnimation.ChainPullDistance, 20.0f);
	TestEqual(TEXT("Inherit keeps definition chain duration"), Resolved.DoorAnimation.ChainPullDuration, 0.25f);

	// Explicit disable affects only presence.
	Instance.InstanceConfig.DoorChainMode = EGridDoorChainMode::Disabled;
	Resolved = GridObjectInstanceBehavior::Resolve(Instance, Definition);
	TestFalse(TEXT("Disabled forces chain off"), Resolved.DoorAnimation.bHasChainMechanism);
	TestEqual(TEXT("Disabled keeps definition distance"), Resolved.DoorAnimation.ChainPullDistance, 20.0f);
	TestEqual(TEXT("Disabled without duration override keeps definition duration"), Resolved.DoorAnimation.ChainPullDuration, 0.25f);

	// Explicit enabled mode is supported even though current recovered Wood/Grating defaults are enabled.
	Instance.InstanceConfig.DoorChainMode = EGridDoorChainMode::Enabled;
	Instance.InstanceConfig.bOverrideChainPullDuration = true;
	Instance.InstanceConfig.ChainPullDuration = 0.60f;
	Resolved = GridObjectInstanceBehavior::Resolve(Instance, Definition);
	TestTrue(TEXT("Enabled forces chain on"), Resolved.DoorAnimation.bHasChainMechanism);
	TestEqual(TEXT("Duration override is applied"), Resolved.DoorAnimation.ChainPullDuration, 0.60f);
	TestEqual(TEXT("Duration override never changes distance"), Resolved.DoorAnimation.ChainPullDistance, 20.0f);

	// Historical 28,27 shape: chain disabled but authored duration retained.
	Instance.InstanceConfig.DoorChainMode = EGridDoorChainMode::Disabled;
	Instance.InstanceConfig.ChainPullDuration = 0.50f;
	Resolved = GridObjectInstanceBehavior::Resolve(Instance, Definition);
	TestFalse(TEXT("Disabled+duration remains disabled"), Resolved.DoorAnimation.bHasChainMechanism);
	TestEqual(TEXT("Disabled+duration preserves authored duration"), Resolved.DoorAnimation.ChainPullDuration, 0.50f);

	// Runtime payload preserves the tri-state and duration flag independently of Behavior.
	const FGridRuntimeWorldObjectData RuntimePayload(Instance);
	TestEqual(TEXT("Runtime payload preserves chain mode"), RuntimePayload.DoorChainMode, EGridDoorChainMode::Disabled);
	TestTrue(TEXT("Runtime payload preserves duration override flag"), RuntimePayload.bOverrideChainPullDuration);
	TestEqual(TEXT("Runtime payload preserves chain duration"), RuntimePayload.ChainPullDuration, 0.50f);

	const FGridObjectBehaviorParams RuntimeResolved = GridObjectInstanceBehavior::Resolve(RuntimePayload, Definition);
	TestFalse(TEXT("Runtime resolver consumes disabled chain mode"), RuntimeResolved.DoorAnimation.bHasChainMechanism);
	TestEqual(TEXT("Runtime resolver consumes chain duration"), RuntimeResolved.DoorAnimation.ChainPullDuration, 0.50f);
	TestEqual(TEXT("Runtime resolver inherits distance"), RuntimeResolved.DoorAnimation.ChainPullDistance, 20.0f);

	// Integration: AGridDoorActor already consumes EffectiveBehavior.DoorAnimation.
	FRecoveryC12TestWorld TestWorld;
	if (!TestNotNull(TEXT("C1.2 transient world exists"), TestWorld.World))
	{
		return false;
	}

	UGridWorldObjectDefinitionAsset* DoorDefinition = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
	DoorDefinition->DefaultBehavior.DoorAnimation.bHasChainMechanism = true;
	DoorDefinition->DefaultBehavior.DoorAnimation.ChainPullDistance = 20.0f;
	DoorDefinition->DefaultBehavior.DoorAnimation.ChainPullDuration = 0.25f;

	FGridWorldObjectInstance EnabledInstance;
	EnabledInstance.InstanceId = FGuid::NewGuid();
	EnabledInstance.Type = EGridLevelObjectType::Door;
	EnabledInstance.CellX = 1;
	EnabledInstance.CellY = 1;
	EnabledInstance.WallSide = EGridEdge::North;
	EnabledInstance.InstanceConfig.DoorChainMode = EGridDoorChainMode::Enabled;
	EnabledInstance.InstanceConfig.bOverrideChainPullDuration = true;
	EnabledInstance.InstanceConfig.ChainPullDuration = 0.60f;

	AGridDoorActor* EnabledDoor = TestWorld.World->SpawnActor<AGridDoorActor>();
	if (!TestNotNull(TEXT("Enabled C1.2 door exists"), EnabledDoor))
	{
		return false;
	}
	EnabledDoor->ChainSupportMesh = NewObject<UStaticMesh>(EnabledDoor);
	EnabledDoor->ChainMovingMesh = NewObject<UStaticMesh>(EnabledDoor);

	const FGridRuntimeWorldObjectData EnabledData(EnabledInstance);
	EnabledDoor->InitializeRuntimeMechanismVisuals(EnabledData, DoorDefinition, FTransform::Identity);
	EnabledDoor->InitializeRuntimeWorldObject(EnabledData, nullptr, FTransform::Identity);

	TestTrue(TEXT("Explicitly enabled chain is visible on runtime door"), EnabledDoor->ChainMovingMeshComponent->IsVisible());
	EnabledDoor->PullChain();
	EnabledDoor->Tick(0.30f);
	TestTrue(
		TEXT("0.60s instance duration drives the chain timeline"),
		EnabledDoor->ChainMovingMeshComponent->GetRelativeLocation().Equals(FVector(0.0f, 0.0f, -20.0f), 0.01f));

	FGridWorldObjectInstance DisabledInstance = EnabledInstance;
	DisabledInstance.InstanceId = FGuid::NewGuid();
	DisabledInstance.InstanceConfig.DoorChainMode = EGridDoorChainMode::Disabled;
	DisabledInstance.InstanceConfig.bOverrideChainPullDuration = false;

	AGridDoorActor* DisabledDoor = TestWorld.World->SpawnActor<AGridDoorActor>();
	if (!TestNotNull(TEXT("Disabled C1.2 door exists"), DisabledDoor))
	{
		return false;
	}
	DisabledDoor->ChainSupportMesh = NewObject<UStaticMesh>(DisabledDoor);
	DisabledDoor->ChainMovingMesh = NewObject<UStaticMesh>(DisabledDoor);

	const FGridRuntimeWorldObjectData DisabledData(DisabledInstance);
	DisabledDoor->InitializeRuntimeMechanismVisuals(DisabledData, DoorDefinition, FTransform::Identity);
	DisabledDoor->InitializeRuntimeWorldObject(DisabledData, nullptr, FTransform::Identity);

	TestFalse(TEXT("Explicitly disabled chain is hidden on runtime door"), DisabledDoor->ChainMovingMeshComponent->IsVisible());
	TestEqual(
		TEXT("Disabled chain interaction has no collision"),
		DisabledDoor->GetChainInteractionComponent()->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
