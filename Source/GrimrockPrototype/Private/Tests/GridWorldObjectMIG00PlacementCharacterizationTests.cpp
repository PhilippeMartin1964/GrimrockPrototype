#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPlacementTransformResolver.h"
#include "UObject/UnrealType.h"

namespace GridWorldObjectMIG00Characterization
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
				FName(*FString::Printf(TEXT("WORLDOBJ_MIG00_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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
		return Level;
	}

	FGridWorldObjectInstance MakeObject(EGridLevelObjectType Type, EGridEdge Edge = EGridEdge::None)
	{
		FGridWorldObjectInstance Object;
		Object.InstanceId = FGuid::NewGuid();
		Object.Type = Type;
		Object.WorldObjectDefinitionId = TEXT("WORLDOBJ_MIG00_Test");
		Object.CellX = 1;
		Object.CellY = 2;
		Object.WallSide = Edge;
		return Object;
	}

	bool IsLocation(const FTransform& Transform, const FVector& Expected)
	{
		return Transform.GetLocation().Equals(Expected, 0.01f);
	}

	bool IsRotation(const FTransform& Transform, const FRotator& Expected)
	{
		return Transform.Rotator().Equals(Expected, 0.01f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG00PlacementSchemaCharacterizationTest,
	"Grimrock.WorldObjects.MIG00.Characterization.PlacementSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG00PlacementSchemaCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* DefinitionClass = UGridWorldObjectDefinitionAsset::StaticClass();
	TestNotNull(TEXT("World object definition class exists"), DefinitionClass);
	if (!DefinitionClass)
	{
		return false;
	}

	// The pre-MIG01 schema has been removed; the historical placement behavior below remains characterized.
	TestNull(TEXT("Legacy PlacementKind bridge is removed"), DefinitionClass->FindPropertyByName(TEXT("PlacementKind")));
	TestNull(TEXT("Legacy PlacementZOffset bridge is removed"), DefinitionClass->FindPropertyByName(TEXT("PlacementZOffset")));
	TestNull(TEXT("Legacy WallInset bridge is removed"), DefinitionClass->FindPropertyByName(TEXT("WallInset")));
	TestNull(TEXT("Legacy LocalOffsetAlongWall bridge is removed"), DefinitionClass->FindPropertyByName(TEXT("LocalOffsetAlongWall")));
	TestNull(TEXT("Legacy LocalOffsetVertical bridge is removed"), DefinitionClass->FindPropertyByName(TEXT("LocalOffsetVertical")));

	const UEnum* PlacementEnum = StaticEnum<EGridObjectPlacementKind>();
	TestNotNull(TEXT("Legacy placement enum exists"), PlacementEnum);
	if (PlacementEnum)
	{
		TestTrue(TEXT("Historical Center symbol is still available during MIG01"), PlacementEnum->GetValueByNameString(TEXT("Center")) != INDEX_NONE);
		TestTrue(TEXT("Historical Edge symbol is still available during MIG01"), PlacementEnum->GetValueByNameString(TEXT("Edge")) != INDEX_NONE);
		TestTrue(TEXT("Floor symbol exists"), PlacementEnum->GetValueByNameString(TEXT("Floor")) != INDEX_NONE);
		TestTrue(TEXT("Wall symbol exists"), PlacementEnum->GetValueByNameString(TEXT("Wall")) != INDEX_NONE);
		TestTrue(TEXT("Ceiling symbol exists"), PlacementEnum->GetValueByNameString(TEXT("Ceiling")) != INDEX_NONE);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG00PlacementTransformCharacterizationTest,
	"Grimrock.WorldObjects.MIG00.Characterization.PlacementTransforms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG00PlacementTransformCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	GridWorldObjectMIG00Characterization::FTestWorld TestWorld;
	if (!TestNotNull(TEXT("MIG00 world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime))
	{
		return false;
	}

	Runtime->LevelAsset = GridWorldObjectMIG00Characterization::MakeLevel(Runtime);
	Runtime->GridOrigin = FVector::ZeroVector;
	Runtime->SetActorLocation(FVector::ZeroVector);

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	Definition->DefinitionId = TEXT("WORLDOBJ_MIG00_Test");
	Definition->SupportedType = EGridLevelObjectType::Decoration;
	Runtime->WorldObjectDefinitions.Add(Definition);

	FTransform Transform;

	// Historical Center contract: MIG01 expresses it as Floor with a centered local position.
	FGridWorldObjectInstance CenterObject = GridWorldObjectMIG00Characterization::MakeObject(EGridLevelObjectType::Decoration);
	CenterObject.bHasLocalTransformOverride = true;
	CenterObject.LocalTransformOverride = FTransform(FRotator(0.0f, 30.0f, 0.0f));
	Definition->PlacementSurface = EGridObjectPlacementKind::Floor;
	Definition->DefaultLocalPosition.U = 0.0f;
	Definition->DefaultLocalPosition.V = 0.0f;
	Definition->DefaultLocalPosition.N = 12.0f;
	TestTrue(TEXT("Historical Center transform resolves through Floor"), GridPlacementTransformResolver::ResolveWorldObject(*Runtime, CenterObject, Transform));
	TestTrue(TEXT("Historical Center location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(300.0f, 500.0f, 12.0f)));
	TestTrue(TEXT("Historical Center LocalYaw is preserved"), GridWorldObjectMIG00Characterization::IsRotation(Transform, FRotator(0.0f, 30.0f, 0.0f)));

	// Historical Floor contract maps directly to Floor.
	TestTrue(TEXT("Historical Floor transform resolves"), GridPlacementTransformResolver::ResolveWorldObject(*Runtime, CenterObject, Transform));
	TestTrue(TEXT("Historical Floor location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(300.0f, 500.0f, 12.0f)));

	// Historical absolute ceiling Z=188 becomes N=12 below the current 200 cm ceiling plane.
	Definition->PlacementSurface = EGridObjectPlacementKind::Ceiling;
	Definition->DefaultLocalPosition.U = 0.0f;
	Definition->DefaultLocalPosition.V = 0.0f;
	Definition->DefaultLocalPosition.N = 12.0f;
	CenterObject.bHasLocalTransformOverride = true;
	CenterObject.LocalTransformOverride = FTransform(FRotator(0.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Historical Ceiling transform resolves"), GridPlacementTransformResolver::ResolveWorldObject(*Runtime, CenterObject, Transform));
	TestTrue(TEXT("Historical Ceiling location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(300.0f, 500.0f, 188.0f)));

	// Historical Wall offsets map to U=25, V=100+10=110, N=6.
	// The existing wall-mounted helper uses the boundary anchor rotation only; LocalYaw is ignored.
	FGridWorldObjectInstance WallObject = GridWorldObjectMIG00Characterization::MakeObject(EGridLevelObjectType::Decoration, EGridEdge::North);
	WallObject.bHasLocalTransformOverride = true;
	WallObject.LocalTransformOverride = FTransform(FRotator(0.0f, 15.0f, 0.0f));
	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	Definition->DefaultLocalPosition.U = 25.0f;
	Definition->DefaultLocalPosition.V = 110.0f;
	Definition->DefaultLocalPosition.N = 6.0f;
	TestTrue(TEXT("Historical Wall transform resolves"), GridPlacementTransformResolver::ResolveWorldObject(*Runtime, WallObject, Transform));
	TestTrue(TEXT("Historical Wall location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(325.0f, 594.0f, 110.0f)));
	TestTrue(TEXT("Historical Wall anchor rotation is preserved"), GridWorldObjectMIG00Characterization::IsRotation(Transform, FRotator(0.0f, 90.0f, 0.0f)));

	// Historical Edge placement is now Wall placement plus the instance boundary (ObjectData.Edge).
	TestTrue(TEXT("Historical Edge transform resolves through Wall + Edge topology"), GridPlacementTransformResolver::ResolveWorldObject(*Runtime, WallObject, Transform));
	TestTrue(TEXT("Historical Edge location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(325.0f, 594.0f, 110.0f)));

	// Doors remain exactly boundary-anchored; Edge is topology, not PlacementSurface.
	FGridWorldObjectInstance DoorObject = GridWorldObjectMIG00Characterization::MakeObject(EGridLevelObjectType::Door, EGridEdge::North);
	Definition->SupportedType = EGridLevelObjectType::Door;
	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	Definition->DefaultLocalPosition.U = 0.0f;
	Definition->DefaultLocalPosition.V = 0.0f;
	Definition->DefaultLocalPosition.N = 0.0f;
	TestTrue(TEXT("Historical Door transform resolves through Wall + boundary"), GridPlacementTransformResolver::ResolveWorldObject(*Runtime, DoorObject, Transform));
	TestTrue(TEXT("Historical Door boundary location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(300.0f, 600.0f, 0.0f)));
	TestTrue(TEXT("Historical Door North rotation is preserved"), GridWorldObjectMIG00Characterization::IsRotation(Transform, FRotator::ZeroRotator));

	// Loose item edge placement is independent of WorldObjectDefinition.
	FGridLooseItemInstance ItemObject;
	ItemObject.InstanceId = FGuid::NewGuid();
	ItemObject.CellX = 1;
	ItemObject.CellY = 2;
	ItemObject.SurfaceSide = EGridEdge::East;
	TestTrue(TEXT("Historical floor item edge transform resolves"), GridPlacementTransformResolver::ResolveLooseItem(*Runtime, ItemObject, Transform));
	TestTrue(TEXT("Historical floor item edge location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(382.0f, 500.0f, 12.0f)));
	TestTrue(TEXT("Historical East floor item rotation is preserved"), GridWorldObjectMIG00Characterization::IsRotation(Transform, FRotator(0.0f, 90.0f, 0.0f)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
