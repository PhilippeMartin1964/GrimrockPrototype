#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
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

	FGridLevelObjectData MakeObject(FName ArchetypeId, EGridLevelObjectType Type, int32 X, int32 Y, EGridEdge Edge = EGridEdge::None)
	{
		FGridLevelObjectData Object;
		Object.ObjectId = FGuid::NewGuid();
		Object.ArchetypeId = ArchetypeId;
		Object.Type = Type;
		Object.CellX = X;
		Object.CellY = Y;
		Object.Edge = Edge;
		Object.bInitiallyEnabled = true;
		return Object;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG03RuntimeSpawnWithoutLegacyMeshTest,
	"Grimrock.WorldObjects.MIG03.RuntimeSpawnWithoutLegacyMesh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG03RuntimeSpawnWithoutLegacyMeshTest::RunTest(const FString& Parameters)
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

	// Static generic object: the target StaticPart is authoritative and all legacy visual fields stay null.
	UGridObjectArchetypeAsset* StaticArchetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	StaticArchetype->ArchetypeId = TEXT("MIG03_TargetStatic");
	StaticArchetype->SupportedType = EGridLevelObjectType::Decoration;
	StaticArchetype->PlacementSurface = EGridObjectPlacementKind::Floor;
	StaticArchetype->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
	StaticArchetype->StaticPart.Mesh = NewObject<UStaticMesh>(StaticArchetype);
	StaticArchetype->StaticPart.LocalTransform = FTransform(FRotator(0.0f, 20.0f, 0.0f), FVector(3.0f, 4.0f, 5.0f));
	StaticArchetype->RefreshPlacementRuntimeProjection();
	TestNull(TEXT("Target static archetype has no PreviewMesh"), StaticArchetype->PreviewMesh.Get());
	TestNull(TEXT("Target static archetype has no FixedMesh"), StaticArchetype->FixedMesh.Get());
	TestNull(TEXT("Target static archetype has no MovingMesh"), StaticArchetype->MovingMesh.Get());
	Runtime->ObjectArchetypes.Add(StaticArchetype);

	const FGridLevelObjectData StaticObject = MakeObject(StaticArchetype->ArchetypeId, EGridLevelObjectType::Decoration, 1, 1);
	Runtime->LevelAsset->Objects.Add(StaticObject);

	// Moving-only mechanism: Part0 must be enough to spawn and initialize the mechanism; no legacy mesh is required.
	UGridObjectArchetypeAsset* ButtonArchetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	ButtonArchetype->ArchetypeId = TEXT("MIG03_TargetButton");
	ButtonArchetype->SupportedType = EGridLevelObjectType::Button;
	ButtonArchetype->PlacementSurface = EGridObjectPlacementKind::Wall;
	ButtonArchetype->RuntimeActorClass = AGridButtonActor::StaticClass();
	ButtonArchetype->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(ButtonArchetype);
	ButtonArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Translation;
	ButtonArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::X;
	ButtonArchetype->MovingParts.Part0.Motion.Amount = 6.0f;
	ButtonArchetype->MovingParts.Part0.Motion.Duration = 0.08f;
	ButtonArchetype->RefreshPlacementRuntimeProjection();
	TestNull(TEXT("Target button has no legacy MovingMesh"), ButtonArchetype->MovingMesh.Get());
	Runtime->ObjectArchetypes.Add(ButtonArchetype);

	const FGridLevelObjectData ButtonObject = MakeObject(ButtonArchetype->ArchetypeId, EGridLevelObjectType::Button, 2, 1, EGridEdge::North);
	Runtime->LevelAsset->Objects.Add(ButtonObject);

	// Invisible runtime object: actor existence is independent of presentation mesh existence.
	UGridObjectArchetypeAsset* TriggerArchetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	TriggerArchetype->ArchetypeId = TEXT("MIG03_InvisibleTrigger");
	TriggerArchetype->SupportedType = EGridLevelObjectType::Trigger;
	TriggerArchetype->PlacementSurface = EGridObjectPlacementKind::Floor;
	TriggerArchetype->RuntimeActorClass = AGridRuntimeObjectActor::StaticClass();
	TriggerArchetype->RefreshPlacementRuntimeProjection();
	Runtime->ObjectArchetypes.Add(TriggerArchetype);

	const FGridLevelObjectData TriggerObject = MakeObject(TriggerArchetype->ArchetypeId, EGridLevelObjectType::Trigger, 1, 2);
	Runtime->LevelAsset->Objects.Add(TriggerObject);

	Runtime->RebuildLevel(EGridRuntimeRebuildMode::Full);

	AGridGenericObjectActor* StaticActor = Runtime->FindRuntimeObjectActor<AGridGenericObjectActor>(StaticObject.ObjectId);
	TestNotNull(TEXT("StaticPart-only generic object spawns without any legacy mesh"), StaticActor);
	if (StaticActor && StaticActor->MeshComponent)
	{
		TestEqual(TEXT("Generic runtime uses StaticPart mesh"), StaticActor->MeshComponent->GetStaticMesh(), StaticArchetype->StaticPart.Mesh.Get());
		TestTrue(TEXT("Generic runtime applies StaticPart LocalTransform"),
			StaticActor->MeshComponent->GetRelativeTransform().Equals(StaticArchetype->StaticPart.LocalTransform, 0.01f));
	}

	AGridButtonActor* ButtonActor = Runtime->FindRuntimeObjectActor<AGridButtonActor>(ButtonObject.ObjectId);
	TestNotNull(TEXT("MovingPart-only mechanism spawns without legacy mesh"), ButtonActor);

	AGridRuntimeObjectActor* TriggerActor = Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(TriggerObject.ObjectId);
	TestNotNull(TEXT("Runtime actor can spawn with no presentation mesh at all"), TriggerActor);
	if (TriggerActor && TriggerActor->MeshComponent)
	{
		TestNull(TEXT("Invisible runtime actor remains meshless"), TriggerActor->MeshComponent->GetStaticMesh());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
