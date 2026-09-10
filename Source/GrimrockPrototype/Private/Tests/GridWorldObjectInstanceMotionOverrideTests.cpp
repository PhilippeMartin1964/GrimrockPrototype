#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridWorldObjectInstanceVisual.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FRecoveryC11TestWorld
	{
		UWorld* World = nullptr;

		FRecoveryC11TestWorld()
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
				FName(*FString::Printf(TEXT("RecoveryC11_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

		~FRecoveryC11TestWorld()
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

	UStaticMeshComponent* FindStaticMeshComponentByName(AActor* Actor, FName ComponentName)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TArray<UStaticMeshComponent*> Components;
		Actor->GetComponents<UStaticMeshComponent>(Components);
		for (UStaticMeshComponent* Component : Components)
		{
			if (Component && Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecoveryC11InstanceMovingPartOverridesTest,
	"Grimrock.WorldObjects.RECOVERY01.C11.InstanceMovingPartOverrides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRecoveryC11InstanceMovingPartOverridesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// 1) Reflected authoring schema: generic only, no mechanism-specific fields.
	UScriptStruct* OverrideStruct = FGridWorldObjectMovingPartInstanceOverride::StaticStruct();
	UScriptStruct* ConfigStruct = FGridWorldObjectInstanceConfig::StaticStruct();

	if (!TestNotNull(TEXT("Moving-part instance override struct exists"), OverrideStruct) ||
		!TestNotNull(TEXT("World-object instance config struct exists"), ConfigStruct))
	{
		return false;
	}

	for (const FName PropertyName : {
				TEXT("PartIndex"),
				TEXT("bOverrideLocalTransform"),
				TEXT("LocalTransform"),
				TEXT("bOverrideMotionAmount"),
				TEXT("MotionAmount"),
				TEXT("bOverrideMotionDuration"),
				TEXT("MotionDuration")})
	{
		TestNotNull(
			*FString::Printf(TEXT("Override exposes %s"), *PropertyName.ToString()),
			OverrideStruct->FindPropertyByName(PropertyName));
	}

	TestNotNull(
		TEXT("InstanceConfig exposes MovingPartOverrides"),
		ConfigStruct->FindPropertyByName(TEXT("MovingPartOverrides")));
	TestNull(TEXT("No DoorAnimation instance field"), ConfigStruct->FindPropertyByName(TEXT("DoorAnimation")));
	TestNull(TEXT("No ButtonAnimation instance field"), ConfigStruct->FindPropertyByName(TEXT("ButtonAnimation")));
	TestNull(TEXT("No PressurePlateAnimation instance field"), ConfigStruct->FindPropertyByName(TEXT("PressurePlateAnimation")));
	TestNull(TEXT("No PitAnimation instance field"), ConfigStruct->FindPropertyByName(TEXT("PitAnimation")));

	UGridLevelAsset* MutableLevel = NewObject<UGridLevelAsset>(GetTransientPackage());
	FGridWorldObjectInstance MutablePlacement;
	MutablePlacement.InstanceId = FGuid::NewGuid();
	MutablePlacement.Type = EGridLevelObjectType::Door;
	MutableLevel->WorldObjectInstances.Add(MutablePlacement);

	FGridWorldObjectInstanceConfig ReplacementConfig;
	FGridWorldObjectMovingPartInstanceOverride ReplacementOverride;
	ReplacementOverride.PartIndex = 0;
	ReplacementOverride.bOverrideMotionDuration = true;
	ReplacementOverride.MotionDuration = 2.5f;
	ReplacementConfig.MovingPartOverrides.Add(ReplacementOverride);

	TestTrue(
		TEXT("LevelAsset exposes narrow InstanceConfig mutation boundary"),
		MutableLevel->SetWorldObjectInstanceConfig(MutablePlacement.InstanceId, ReplacementConfig));
	const FGridWorldObjectInstance* MutatedPlacement =
		MutableLevel->FindWorldObjectInstanceById(MutablePlacement.InstanceId);
	TestNotNull(TEXT("Mutated placement still exists"), MutatedPlacement);
	if (MutatedPlacement)
	{
		TestEqual(
			TEXT("InstanceConfig setter persists one moving-part override"),
			MutatedPlacement->InstanceConfig.MovingPartOverrides.Num(),
			1);
	}

	// 2) Runtime payload preserves the sparse visual overrides independently from Behavior.
	FGridWorldObjectInstance SourceInstance;
	SourceInstance.InstanceId = FGuid::NewGuid();
	SourceInstance.Type = EGridLevelObjectType::Door;

	FGridWorldObjectMovingPartInstanceOverride PayloadOverride;
	PayloadOverride.PartIndex = 0;
	PayloadOverride.bOverrideMotionAmount = true;
	PayloadOverride.MotionAmount = 160.0f;
	PayloadOverride.bOverrideMotionDuration = true;
	PayloadOverride.MotionDuration = 3.85f;
	SourceInstance.InstanceConfig.MovingPartOverrides.Add(PayloadOverride);

	const FGridRuntimeWorldObjectData RuntimePayload(SourceInstance);
	TestEqual(TEXT("Runtime payload carries one moving-part override"), RuntimePayload.MovingPartOverrides.Num(), 1);
	if (RuntimePayload.MovingPartOverrides.Num() == 1)
	{
		TestEqual(TEXT("Runtime payload preserves PartIndex"), RuntimePayload.MovingPartOverrides[0].PartIndex, 0);
		TestEqual(TEXT("Runtime payload preserves Amount"), RuntimePayload.MovingPartOverrides[0].MotionAmount, 160.0f);
		TestEqual(TEXT("Runtime payload preserves Duration"), RuntimePayload.MovingPartOverrides[0].MotionDuration, 3.85f);
	}

	// 3) Generic resolver covers the historical pressure-plate shape without a plate-specific struct.
	FGridWorldObjectMovingPart PlateDefinitionPart;
	PlateDefinitionPart.Mesh = NewObject<UStaticMesh>(GetTransientPackage());
	PlateDefinitionPart.LocalTransform = FTransform::Identity;
	PlateDefinitionPart.Motion.Type = EGridWorldObjectMotionType::Translation;
	PlateDefinitionPart.Motion.Axis = EGridWorldObjectMotionAxis::Z;
	PlateDefinitionPart.Motion.Amount = -4.0f;
	PlateDefinitionPart.Motion.Duration = 0.25f;

	FGridWorldObjectMovingPartInstanceOverride PlateOverride;
	PlateOverride.PartIndex = 0;
	PlateOverride.bOverrideLocalTransform = true;
	PlateOverride.LocalTransform = FTransform(FQuat::Identity, FVector(0.0f, 0.0f, -1.0f));
	PlateOverride.bOverrideMotionAmount = true;
	PlateOverride.MotionAmount = -3.0f;

	const TArray<FGridWorldObjectMovingPartInstanceOverride> PlateOverrides{PlateOverride};
	const FGridWorldObjectMovingPart ResolvedPlate =
		GridWorldObjectInstanceVisual::ResolveMovingPart(PlateDefinitionPart, PlateOverrides, 0);

	TestTrue(
		TEXT("Pressure-plate rest transform can be overridden generically"),
		ResolvedPlate.LocalTransform.GetLocation().Equals(FVector(0.0f, 0.0f, -1.0f), KINDA_SMALL_NUMBER));
	TestEqual(TEXT("Pressure-plate travel can be overridden generically"), ResolvedPlate.Motion.Amount, -3.0f);
	TestEqual(TEXT("Unspecified pressure-plate duration still inherits definition"), ResolvedPlate.Motion.Duration, 0.25f);

	const FGridWorldObjectMovingPart UnrelatedPart =
		GridWorldObjectInstanceVisual::ResolveMovingPart(PlateDefinitionPart, PlateOverrides, 1);
	TestTrue(
		TEXT("PartIndex keeps overrides sparse"),
		UnrelatedPart.LocalTransform.Equals(PlateDefinitionPart.LocalTransform, KINDA_SMALL_NUMBER));
	TestEqual(TEXT("Unrelated part keeps definition amount"), UnrelatedPart.Motion.Amount, -4.0f);

	// 4) Runtime mechanism consumes resolved Amount + Duration.
	FRecoveryC11TestWorld TestWorld;
	if (!TestNotNull(TEXT("C1.1 transient world exists"), TestWorld.World))
	{
		return false;
	}

	UGridWorldObjectDefinitionAsset* DoorDefinition = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
	DoorDefinition->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(DoorDefinition);
	DoorDefinition->MovingParts.Part0.LocalTransform = FTransform::Identity;
	DoorDefinition->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Translation;
	DoorDefinition->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Z;
	DoorDefinition->MovingParts.Part0.Motion.Amount = 170.0f;
	DoorDefinition->MovingParts.Part0.Motion.Duration = 5.0f;

	FGridWorldObjectInstance DoorInstance;
	DoorInstance.InstanceId = FGuid::NewGuid();
	DoorInstance.Type = EGridLevelObjectType::Door;
	DoorInstance.CellX = 1;
	DoorInstance.CellY = 1;
	DoorInstance.WallSide = EGridEdge::North;

	FGridWorldObjectMovingPartInstanceOverride DoorOverride;
	DoorOverride.PartIndex = 0;
	DoorOverride.bOverrideMotionAmount = true;
	DoorOverride.MotionAmount = 160.0f;
	DoorOverride.bOverrideMotionDuration = true;
	DoorOverride.MotionDuration = 3.85f;
	DoorInstance.InstanceConfig.MovingPartOverrides.Add(DoorOverride);

	AGridDoorActor* Door = TestWorld.World->SpawnActor<AGridDoorActor>();
	if (!TestNotNull(TEXT("C1.1 door exists"), Door))
	{
		return false;
	}

	const FGridRuntimeWorldObjectData DoorRuntimeData(DoorInstance);
	Door->InitializeRuntimeMechanismVisuals(DoorRuntimeData, DoorDefinition, FTransform::Identity);
	Door->InitializeRuntimeWorldObject(DoorRuntimeData, nullptr, FTransform::Identity);

	TestEqual(TEXT("Door duration resolves from instance override"), Door->MoveDuration, 3.85f);
	Door->OpenDoor();
	Door->Tick(3.85f);
	TestTrue(TEXT("Door reaches fully open state using overridden duration"), Door->IsFullyOpen());

	if (UStaticMeshComponent* DoorPart = FindStaticMeshComponentByName(Door, TEXT("MovingMesh")))
	{
		TestTrue(
			TEXT("Door amount resolves from instance override"),
			DoorPart->GetRelativeLocation().Equals(FVector(0.0f, 0.0f, 160.0f), 0.01f));
	}
	else
	{
		AddError(TEXT("Door MovingMesh component is missing."));
	}

	// 5) Editor preview consumes the same resolved LocalTransform for authored rest state.
	UGridWorldObjectDefinitionAsset* PlateDefinition = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
	PlateDefinition->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(PlateDefinition);
	PlateDefinition->MovingParts.Part0.LocalTransform = FTransform::Identity;
	PlateDefinition->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Translation;
	PlateDefinition->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Z;
	PlateDefinition->MovingParts.Part0.Motion.Amount = -4.0f;
	PlateDefinition->MovingParts.Part0.Motion.Duration = 0.25f;

	FGridWorldObjectInstance PlateInstance;
	PlateInstance.InstanceId = FGuid::NewGuid();
	PlateInstance.Type = EGridLevelObjectType::PressurePlate;
	PlateInstance.InstanceConfig.MovingPartOverrides.Add(PlateOverride);

	AGridEditorPreviewObjectActor* Preview = TestWorld.World->SpawnActor<AGridEditorPreviewObjectActor>();
	if (!TestNotNull(TEXT("C1.1 preview actor exists"), Preview))
	{
		return false;
	}

	Preview->InitializePreviewObjectFromDefinition(
		PlateInstance.InstanceId,
		PlateInstance.Type,
		PlateDefinition,
		&PlateInstance.InstanceConfig);

	if (UStaticMeshComponent* PreviewPart = FindStaticMeshComponentByName(Preview, TEXT("MovingPart0")))
	{
		TestTrue(
			TEXT("Editor preview resolves instance rest transform"),
			PreviewPart->GetRelativeLocation().Equals(FVector(0.0f, 0.0f, -1.0f), KINDA_SMALL_NUMBER));
	}
	else
	{
		AddError(TEXT("Preview MovingPart0 component is missing."));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
