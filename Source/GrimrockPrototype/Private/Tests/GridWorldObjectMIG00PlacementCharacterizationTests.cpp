#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
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

	FGridLevelObjectData MakeObject(EGridLevelObjectType Type, EGridEdge Edge = EGridEdge::None)
	{
		FGridLevelObjectData Object;
		Object.ObjectId = FGuid::NewGuid();
		Object.Type = Type;
		Object.ArchetypeId = TEXT("WORLDOBJ_MIG00_Test");
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
	FGridWorldObjectMIG00PlacementSchemaCharacterizationTest,
	"Grimrock.WorldObjects.MIG00.Characterization.PlacementSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG00PlacementSchemaCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ArchetypeClass = UGridObjectArchetypeAsset::StaticClass();
	TestNotNull(TEXT("World object archetype class exists"), ArchetypeClass);
	if (!ArchetypeClass)
	{
		return false;
	}

	// MIG00 records the legacy schema that existed before MIG01. These fields are now
	// transient implementation bridges only; the test does not exercise them as authoring data.
	TestNotNull(TEXT("Legacy PlacementKind bridge exists during MIG01"), ArchetypeClass->FindPropertyByName(TEXT("PlacementKind")));
	TestNotNull(TEXT("Legacy PlacementZOffset bridge exists during MIG01"), ArchetypeClass->FindPropertyByName(TEXT("PlacementZOffset")));
	TestNotNull(TEXT("Legacy WallInset bridge exists during MIG01"), ArchetypeClass->FindPropertyByName(TEXT("WallInset")));
	TestNotNull(TEXT("Legacy LocalOffsetAlongWall bridge exists during MIG01"), ArchetypeClass->FindPropertyByName(TEXT("LocalOffsetAlongWall")));
	TestNotNull(TEXT("Legacy LocalOffsetVertical bridge exists during MIG01"), ArchetypeClass->FindPropertyByName(TEXT("LocalOffsetVertical")));

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

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	Archetype->ArchetypeId = TEXT("WORLDOBJ_MIG00_Test");
	Archetype->SupportedType = EGridLevelObjectType::Decoration;
	Runtime->ObjectArchetypes.Add(Archetype);

	FTransform Transform;

	// Historical Center contract: MIG01 expresses it as Floor with a centered local position.
	FGridLevelObjectData CenterObject = GridWorldObjectMIG00Characterization::MakeObject(EGridLevelObjectType::Decoration);
	CenterObject.LocalYaw = 30.0f;
	Archetype->PlacementSurface = EGridObjectPlacementKind::Floor;
	Archetype->DefaultLocalPosition.U = 0.0f;
	Archetype->DefaultLocalPosition.V = 0.0f;
	Archetype->DefaultLocalPosition.N = 12.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	TestTrue(TEXT("Historical Center transform resolves through Floor"), Runtime->GetObjectPlacementTransform(CenterObject, Transform));
	TestTrue(TEXT("Historical Center location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(300.0f, 500.0f, 12.0f)));
	TestTrue(TEXT("Historical Center LocalYaw is preserved"), GridWorldObjectMIG00Characterization::IsRotation(Transform, FRotator(0.0f, 30.0f, 0.0f)));

	// Historical Floor contract maps directly to Floor.
	TestTrue(TEXT("Historical Floor transform resolves"), Runtime->GetObjectPlacementTransform(CenterObject, Transform));
	TestTrue(TEXT("Historical Floor location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(300.0f, 500.0f, 12.0f)));

	// Historical absolute ceiling Z=188 becomes N=12 below the current 200 cm ceiling plane.
	Archetype->PlacementSurface = EGridObjectPlacementKind::Ceiling;
	Archetype->DefaultLocalPosition.U = 0.0f;
	Archetype->DefaultLocalPosition.V = 0.0f;
	Archetype->DefaultLocalPosition.N = 12.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	CenterObject.LocalYaw = 0.0f;
	TestTrue(TEXT("Historical Ceiling transform resolves"), Runtime->GetObjectPlacementTransform(CenterObject, Transform));
	TestTrue(TEXT("Historical Ceiling location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(300.0f, 500.0f, 188.0f)));

	// Historical Wall offsets map to U=25, V=100+10=110, N=6.
	// The existing wall-mounted helper uses the boundary anchor rotation only; LocalYaw is ignored.
	FGridLevelObjectData WallObject = GridWorldObjectMIG00Characterization::MakeObject(EGridLevelObjectType::Decoration, EGridEdge::North);
	WallObject.LocalYaw = 15.0f;
	Archetype->PlacementSurface = EGridObjectPlacementKind::Wall;
	Archetype->DefaultLocalPosition.U = 25.0f;
	Archetype->DefaultLocalPosition.V = 110.0f;
	Archetype->DefaultLocalPosition.N = 6.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	TestTrue(TEXT("Historical Wall transform resolves"), Runtime->GetObjectPlacementTransform(WallObject, Transform));
	TestTrue(TEXT("Historical Wall location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(325.0f, 594.0f, 110.0f)));
	TestTrue(TEXT("Historical Wall anchor rotation is preserved"), GridWorldObjectMIG00Characterization::IsRotation(Transform, FRotator(0.0f, 90.0f, 0.0f)));

	// Historical Edge placement is now Wall placement plus the instance boundary (ObjectData.Edge).
	TestTrue(TEXT("Historical Edge transform resolves through Wall + Edge topology"), Runtime->GetObjectPlacementTransform(WallObject, Transform));
	TestTrue(TEXT("Historical Edge location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(325.0f, 594.0f, 110.0f)));

	// Doors remain exactly boundary-anchored; Edge is topology, not PlacementSurface.
	FGridLevelObjectData DoorObject = GridWorldObjectMIG00Characterization::MakeObject(EGridLevelObjectType::Door, EGridEdge::North);
	Archetype->SupportedType = EGridLevelObjectType::Door;
	Archetype->PlacementSurface = EGridObjectPlacementKind::Wall;
	Archetype->DefaultLocalPosition.U = 0.0f;
	Archetype->DefaultLocalPosition.V = 0.0f;
	Archetype->DefaultLocalPosition.N = 0.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	TestTrue(TEXT("Historical Door transform resolves through Wall + boundary"), Runtime->GetObjectPlacementTransform(DoorObject, Transform));
	TestTrue(TEXT("Historical Door boundary location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(300.0f, 600.0f, 0.0f)));
	TestTrue(TEXT("Historical Door North rotation is preserved"), GridWorldObjectMIG00Characterization::IsRotation(Transform, FRotator::ZeroRotator));

	// Item-on-edge remains an item-instance rule for now; Floor N preserves its historical height.
	FGridLevelObjectData ItemObject = GridWorldObjectMIG00Characterization::MakeObject(EGridLevelObjectType::Item, EGridEdge::East);
	Archetype->SupportedType = EGridLevelObjectType::Item;
	Archetype->PlacementSurface = EGridObjectPlacementKind::Floor;
	Archetype->DefaultLocalPosition.U = 0.0f;
	Archetype->DefaultLocalPosition.V = 0.0f;
	Archetype->DefaultLocalPosition.N = 12.0f;
	Archetype->RefreshPlacementRuntimeProjection();
	TestTrue(TEXT("Historical floor item edge transform resolves"), Runtime->GetObjectPlacementTransform(ItemObject, Transform));
	TestTrue(TEXT("Historical floor item edge location is preserved"), GridWorldObjectMIG00Characterization::IsLocation(Transform, FVector(382.0f, 500.0f, 12.0f)));
	TestTrue(TEXT("Historical East floor item rotation is preserved"), GridWorldObjectMIG00Characterization::IsRotation(Transform, FRotator(0.0f, 90.0f, 0.0f)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
