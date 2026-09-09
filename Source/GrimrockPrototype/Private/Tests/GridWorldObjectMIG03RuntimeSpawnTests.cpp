#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPlacementTransformResolver.h"
#include "Runtime/GridRuntimeObjectActor.h"

namespace GridWorldObjectMIG03RuntimeSpawn
{
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
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
				EWorldType::Game, false,
				FName(*FString::Printf(TEXT("WORLDOBJ_MIG03_Spawn_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
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

	UGridLevelAsset* MakeLevel(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 4;
		Level->Height = 4;
		Level->CellSize = 200.0f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
		}
		return Level;
	}

	FGridWorldObjectInstance MakeWorldObjectInstance(FName DefinitionId, EGridLevelObjectType Type, int32 X, int32 Y, EGridEdge WallSide = EGridEdge::None)
	{
		FGridWorldObjectInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.WorldObjectDefinitionId = DefinitionId;
		Instance.Type = Type;
		Instance.CellX = X;
		Instance.CellY = Y;
		Instance.WallSide = WallSide;
		Instance.bInitiallyEnabled = true;
		return Instance;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG03RuntimeSpawnFromVisualCompositionTest,
	"Grimrock.WorldObjects.MIG03.RuntimeSpawnFromVisualComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG03RuntimeSpawnFromVisualCompositionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridWorldObjectMIG03RuntimeSpawn;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("MIG03 runtime spawn world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime))
	{
		return false;
	}
	Runtime->LevelAsset = MakeLevel(Runtime);
	Runtime->GridOrigin = FVector::ZeroVector;
	Runtime->SetActorLocation(FVector::ZeroVector);

	// Static generic object: StaticPart alone is authoritative for presentation and local transform.
	UGridWorldObjectDefinitionAsset* StaticDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	StaticDefinition->DefinitionId = TEXT("MIG03_TargetStatic");
	StaticDefinition->SupportedType = EGridLevelObjectType::Decoration;
	StaticDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	StaticDefinition->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
	StaticDefinition->StaticPart.Mesh = NewObject<UStaticMesh>(StaticDefinition);
	StaticDefinition->StaticPart.LocalTransform = FTransform(FRotator(0.0f, 20.0f, 0.0f), FVector(3.0f, 4.0f, 5.0f));
	TestTrue(TEXT("Static definition reports a target visual part"), StaticDefinition->HasAnyVisualPart());
	TestFalse(TEXT("Static definition has no moving visual part"), StaticDefinition->HasMovingVisualPart());
	Runtime->WorldObjectDefinitions.Add(StaticDefinition);

	const FGridWorldObjectInstance StaticObject =
		MakeWorldObjectInstance(StaticDefinition->DefinitionId, EGridLevelObjectType::Decoration, 1, 1);
	Runtime->LevelAsset->WorldObjectInstances.Add(StaticObject);

	// Moving-only mechanism: Part0 is enough to spawn and initialize the mechanism.
	UGridWorldObjectDefinitionAsset* ButtonDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	ButtonDefinition->DefinitionId = TEXT("MIG03_TargetButton");
	ButtonDefinition->SupportedType = EGridLevelObjectType::Button;
	ButtonDefinition->ObjectCategory = EGridObjectCategory::Mechanism;
	ButtonDefinition->bIsInteractable = true;
	ButtonDefinition->PlacementSurface = EGridObjectPlacementKind::Wall;
	ButtonDefinition->RuntimeActorClass = AGridButtonActor::StaticClass();
	ButtonDefinition->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(ButtonDefinition);
	ButtonDefinition->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Translation;
	ButtonDefinition->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::X;
	ButtonDefinition->MovingParts.Part0.Motion.Amount = 6.0f;
	ButtonDefinition->MovingParts.Part0.Motion.Duration = 0.08f;
	TestTrue(TEXT("Button definition reports moving presentation"), ButtonDefinition->HasMovingVisualPart());
	TestEqual(TEXT("Button definition defines exactly one moving part"), ButtonDefinition->GetDefinedMovingPartCount(), 1);
	Runtime->WorldObjectDefinitions.Add(ButtonDefinition);

	const FGridWorldObjectInstance ButtonObject =
		MakeWorldObjectInstance(ButtonDefinition->DefinitionId, EGridLevelObjectType::Button, 2, 1, EGridEdge::North);
	Runtime->LevelAsset->WorldObjectInstances.Add(ButtonObject);

	// Invisible runtime object: actor existence is independent of presentation existence.
	UGridWorldObjectDefinitionAsset* TriggerDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	TriggerDefinition->DefinitionId = TEXT("MIG03_InvisibleTrigger");
	TriggerDefinition->SupportedType = EGridLevelObjectType::Trigger;
	TriggerDefinition->ObjectCategory = EGridObjectCategory::Mechanism;
	TriggerDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	TriggerDefinition->RuntimeActorClass = AGridRuntimeObjectActor::StaticClass();
	TestFalse(TEXT("Invisible trigger has no visual composition"), TriggerDefinition->HasAnyVisualPart());
	Runtime->WorldObjectDefinitions.Add(TriggerDefinition);

	const FGridWorldObjectInstance TriggerObject =
		MakeWorldObjectInstance(TriggerDefinition->DefinitionId, EGridLevelObjectType::Trigger, 1, 2);
	Runtime->LevelAsset->WorldObjectInstances.Add(TriggerObject);

	Runtime->RebuildLevel(EGridRuntimeRebuildMode::Full);

	AGridGenericObjectActor* StaticActor = Runtime->FindRuntimeObjectActor<AGridGenericObjectActor>(StaticObject.InstanceId);
	TestNotNull(TEXT("StaticPart-only generic object spawns from target visual composition"), StaticActor);
	if (StaticActor && StaticActor->MeshComponent)
	{
		TestTrue(TEXT("Generic runtime uses StaticPart mesh"), StaticActor->MeshComponent->GetStaticMesh() == StaticDefinition->StaticPart.Mesh.Get());
		TestTrue(TEXT("Generic runtime applies StaticPart LocalTransform"),
			StaticActor->MeshComponent->GetRelativeTransform().Equals(StaticDefinition->StaticPart.LocalTransform, 0.01f));
	}

	AGridButtonActor* ButtonActor = Runtime->FindRuntimeObjectActor<AGridButtonActor>(ButtonObject.InstanceId);
	TestNotNull(TEXT("MovingPart-only mechanism spawns from target visual composition"), ButtonActor);
	if (ButtonActor)
	{
		FTransform PreviewPlacement;
		TestTrue(TEXT("Typed preview placement resolves for the mechanism"),
			GridPlacementTransformResolver::ResolveWorldObject(*Runtime, ButtonObject, PreviewPlacement));
		TestTrue(TEXT("Runtime and preview use the same placement transform"), ButtonActor->GetActorTransform().Equals(PreviewPlacement));
	}

	AGridRuntimeObjectActor* TriggerActor = Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(TriggerObject.InstanceId);
	TestNotNull(TEXT("Runtime actor can spawn with no presentation at all"), TriggerActor);
	if (TriggerActor && TriggerActor->MeshComponent)
	{
		TestNull(TEXT("Invisible runtime actor remains meshless"), TriggerActor->MeshComponent->GetStaticMesh());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
