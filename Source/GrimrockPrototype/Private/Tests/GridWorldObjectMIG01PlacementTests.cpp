#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/UnrealType.h"

namespace GridWorldObjectMIG01
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
				FName(*FString::Printf(TEXT("WORLDOBJ_MIG01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

	FGridLevelObjectData MakeObject(EGridLevelObjectType Type, EGridEdge Edge = EGridEdge::None)
	{
		FGridLevelObjectData Object;
		Object.ObjectId = FGuid::NewGuid();
		Object.Type = Type;
		Object.ArchetypeId = TEXT("WORLDOBJ_MIG01_Test");
		Object.CellX = 1;
		Object.CellY = 2;
		Object.Edge = Edge;
		Object.LocalYaw = 0.0f;
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
	FGridWorldObjectMIG01PlacementSchemaTest,
	"Grimrock.WorldObjects.MIG01.PlacementSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG01PlacementSchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ArchetypeClass = UGridObjectArchetypeAsset::StaticClass();
	TestNotNull(TEXT("World object archetype class exists"), ArchetypeClass);
	if (!ArchetypeClass)
	{
		return false;
	}

	FProperty* SurfaceProperty = ArchetypeClass->FindPropertyByName(TEXT("PlacementSurface"));
	TestNotNull(TEXT("PlacementSurface is the reflected placement authority"), SurfaceProperty);
	if (SurfaceProperty)
	{
		TestEqual(TEXT("PlacementSurface is displayed as Placement Surface"), SurfaceProperty->GetMetaData(TEXT("DisplayName")), FString(TEXT("Placement Surface")));
	}

	TestNotNull(TEXT("DefaultLocalPosition exists"), ArchetypeClass->FindPropertyByName(TEXT("DefaultLocalPosition")));

	const TArray<FName> TransientBridgeProperties = {
		TEXT("PlacementKind"),
		TEXT("PlacementZOffset"),
		TEXT("WallInset"),
		TEXT("LocalOffsetAlongWall"),
		TEXT("LocalOffsetVertical")
	};
	for (const FName PropertyName : TransientBridgeProperties)
	{
		FProperty* Property = ArchetypeClass->FindPropertyByName(PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s remains only as a transient source bridge"), *PropertyName.ToString()), Property);
		if (Property)
		{
			TestTrue(*FString::Printf(TEXT("%s is transient and therefore not serialized"), *PropertyName.ToString()),
				Property->HasAnyPropertyFlags(CPF_Transient));
			TestFalse(*FString::Printf(TEXT("%s is not an authoring parameter"), *PropertyName.ToString()),
				Property->HasAnyPropertyFlags(CPF_Edit));
		}
	}

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>();
	TestTrue(TEXT("Floor is a valid placement surface"), Archetype->HasValidPlacementSurface());

	Archetype->PlacementSurface = EGridObjectPlacementKind::Wall;
	TestTrue(TEXT("Wall is a valid placement surface"), Archetype->HasValidPlacementSurface());

	Archetype->PlacementSurface = EGridObjectPlacementKind::Ceiling;
	TestTrue(TEXT("Ceiling is a valid placement surface"), Archetype->HasValidPlacementSurface());

	Archetype->PlacementSurface = EGridObjectPlacementKind::Center;
	TestFalse(TEXT("Center is no longer a valid placement surface"), Archetype->HasValidPlacementSurface());

	Archetype->PlacementSurface = EGridObjectPlacementKind::Edge;
	TestFalse(TEXT("Edge is no longer a valid placement surface"), Archetype->HasValidPlacementSurface());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG01PlacementTransformParityTest,
	"Grimrock.WorldObjects.MIG01.PlacementTransformParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG01PlacementTransformParityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	GridWorldObjectMIG01::FTestWorld TestWorld;
	if (!TestNotNull(TEXT("MIG01 world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime))
	{
		return false;
	}

	Runtime->LevelAsset = GridWorldObjectMIG01::MakeLevel(Runtime);
	Runtime->GridOrigin = FVector::ZeroVector;
	Runtime->SetActorLocation(FVector::ZeroVector);

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	Archetype->ArchetypeId = TEXT("WORLDOBJ_MIG01_Test");
	Archetype->SupportedType = EGridLevelObjectType::Decoration;
	Runtime->ObjectArchetypes.Add(Archetype);

	FTransform Transform;

	// Floor: U/V remain zero, N is height above floor.
	FGridLevelObjectData FloorObject = GridWorldObjectMIG01::MakeObject(EGridLevelObjectType::Decoration);
	FloorObject.LocalYaw = 30.0f;
	Archetype->PlacementSurface = EGridObjectPlacementKind::Floor;
	Archetype->DefaultLocalPosition.U = 0.0f;
	Archetype->DefaultLocalPosition.V = 0.0f;
	Archetype->DefaultLocalPosition.N = 12.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	TestTrue(TEXT("Floor transform resolves"), Runtime->GetObjectPlacementTransform(FloorObject, Transform));
	TestTrue(TEXT("Floor N preserves the characterized height"), GridWorldObjectMIG01::IsLocation(Transform, FVector(300.0f, 500.0f, 12.0f)));
	TestTrue(TEXT("Per-instance LocalYaw is preserved"), GridWorldObjectMIG01::IsRotation(Transform, FRotator(0.0f, 30.0f, 0.0f)));

	// Ceiling: N is measured downward from the current 200 cm ceiling plane.
	Archetype->PlacementSurface = EGridObjectPlacementKind::Ceiling;
	Archetype->DefaultLocalPosition.U = 0.0f;
	Archetype->DefaultLocalPosition.V = 0.0f;
	Archetype->DefaultLocalPosition.N = 12.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	FloorObject.LocalYaw = 0.0f;
	TestTrue(TEXT("Ceiling transform resolves"), Runtime->GetObjectPlacementTransform(FloorObject, Transform));
	TestTrue(TEXT("Ceiling N is distance below ceiling"), GridWorldObjectMIG01::IsLocation(Transform, FVector(300.0f, 500.0f, 188.0f)));

	// Wall: U = along wall, V = vertical, N = inset into the cell.
	// MIG01 preserves the existing wall-mounted rotation contract: the boundary anchor drives yaw and LocalYaw is ignored.
	FGridLevelObjectData WallObject = GridWorldObjectMIG01::MakeObject(EGridLevelObjectType::Decoration, EGridEdge::North);
	WallObject.LocalYaw = 15.0f;
	Archetype->PlacementSurface = EGridObjectPlacementKind::Wall;
	Archetype->DefaultLocalPosition.U = 25.0f;
	Archetype->DefaultLocalPosition.V = 110.0f;
	Archetype->DefaultLocalPosition.N = 6.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	TestTrue(TEXT("Wall transform resolves"), Runtime->GetObjectPlacementTransform(WallObject, Transform));
	TestTrue(TEXT("Wall U/V/N preserves characterized placement"), GridWorldObjectMIG01::IsLocation(Transform, FVector(325.0f, 594.0f, 110.0f)));
	TestTrue(TEXT("Wall anchor rotation is preserved"), GridWorldObjectMIG01::IsRotation(Transform, FRotator(0.0f, 90.0f, 0.0f)));

	// Door remains boundary-anchored. Edge is topology (ObjectData.Edge), not a placement surface.
	FGridLevelObjectData DoorObject = GridWorldObjectMIG01::MakeObject(EGridLevelObjectType::Door, EGridEdge::North);
	Archetype->SupportedType = EGridLevelObjectType::Door;
	Archetype->PlacementSurface = EGridObjectPlacementKind::Wall;
	Archetype->DefaultLocalPosition.U = 0.0f;
	Archetype->DefaultLocalPosition.V = 0.0f;
	Archetype->DefaultLocalPosition.N = 0.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	TestTrue(TEXT("Door transform resolves on Wall surface"), Runtime->GetObjectPlacementTransform(DoorObject, Transform));
	TestTrue(TEXT("Door remains anchored on the exact North boundary"), GridWorldObjectMIG01::IsLocation(Transform, FVector(300.0f, 600.0f, 0.0f)));
	TestTrue(TEXT("Door historical boundary rotation is preserved"), GridWorldObjectMIG01::IsRotation(Transform, FRotator::ZeroRotator));

	// Item edge placement remains a separate item-instance rule for now.
	FGridLevelObjectData ItemObject = GridWorldObjectMIG01::MakeObject(EGridLevelObjectType::Item, EGridEdge::East);
	Archetype->SupportedType = EGridLevelObjectType::Item;
	Archetype->PlacementSurface = EGridObjectPlacementKind::Floor;
	Archetype->DefaultLocalPosition.U = 0.0f;
	Archetype->DefaultLocalPosition.V = 0.0f;
	Archetype->DefaultLocalPosition.N = 12.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	TestTrue(TEXT("Floor item edge transform resolves"), Runtime->GetObjectPlacementTransform(ItemObject, Transform));
	TestTrue(TEXT("Floor item edge keeps the characterized 18 cm minimum inset"), GridWorldObjectMIG01::IsLocation(Transform, FVector(382.0f, 500.0f, 12.0f)));
	TestTrue(TEXT("East floor item faces the edge"), GridWorldObjectMIG01::IsRotation(Transform, FRotator(0.0f, 90.0f, 0.0f)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
