#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/UnrealType.h"

namespace GridWorldObjectMIG00
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

	TestNotNull(TEXT("Legacy PlacementKind is the current placement authority"), ArchetypeClass->FindPropertyByName(TEXT("PlacementKind")));
	TestNotNull(TEXT("Legacy PlacementZOffset exists"), ArchetypeClass->FindPropertyByName(TEXT("PlacementZOffset")));
	TestNotNull(TEXT("Legacy WallInset exists"), ArchetypeClass->FindPropertyByName(TEXT("WallInset")));
	TestNotNull(TEXT("Legacy LocalOffsetAlongWall exists"), ArchetypeClass->FindPropertyByName(TEXT("LocalOffsetAlongWall")));
	TestNotNull(TEXT("Legacy LocalOffsetVertical exists"), ArchetypeClass->FindPropertyByName(TEXT("LocalOffsetVertical")));

	const UEnum* PlacementEnum = StaticEnum<EGridObjectPlacementKind>();
	TestNotNull(TEXT("Legacy placement enum exists"), PlacementEnum);
	if (PlacementEnum)
	{
		TestTrue(TEXT("Center exists before migration"), PlacementEnum->GetValueByNameString(TEXT("Center")) != INDEX_NONE);
		TestTrue(TEXT("Edge exists before migration"), PlacementEnum->GetValueByNameString(TEXT("Edge")) != INDEX_NONE);
		TestTrue(TEXT("Floor exists before migration"), PlacementEnum->GetValueByNameString(TEXT("Floor")) != INDEX_NONE);
		TestTrue(TEXT("Wall exists before migration"), PlacementEnum->GetValueByNameString(TEXT("Wall")) != INDEX_NONE);
		TestTrue(TEXT("Ceiling exists before migration"), PlacementEnum->GetValueByNameString(TEXT("Ceiling")) != INDEX_NONE);
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
	using namespace GridWorldObjectMIG00;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("MIG00 world exists"), TestWorld.World))
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

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	Archetype->ArchetypeId = TEXT("WORLDOBJ_MIG00_Test");
	Archetype->SupportedType = EGridLevelObjectType::Decoration;
	Runtime->ObjectArchetypes.Add(Archetype);

	FTransform Transform;

	// Center and Floor share the same cell-centered transform contract.
	FGridLevelObjectData CenterObject = MakeObject(EGridLevelObjectType::Decoration);
	CenterObject.LocalYaw = 30.0f;
	Archetype->PlacementKind = EGridObjectPlacementKind::Center;
	Archetype->PlacementZOffset = 12.0f;
	TestTrue(TEXT("Center transform resolves"), Runtime->GetObjectPlacementTransform(CenterObject, Transform));
	TestTrue(TEXT("Center location is cell center + Z offset"), IsLocation(Transform, FVector(300.0f, 500.0f, 12.0f)));
	TestTrue(TEXT("Center LocalYaw is applied"), IsRotation(Transform, FRotator(0.0f, 30.0f, 0.0f)));

	Archetype->PlacementKind = EGridObjectPlacementKind::Floor;
	TestTrue(TEXT("Floor transform resolves"), Runtime->GetObjectPlacementTransform(CenterObject, Transform));
	TestTrue(TEXT("Floor currently matches Center"), IsLocation(Transform, FVector(300.0f, 500.0f, 12.0f)));

	// Ceiling is also routed through the old centered/Z-offset path.
	Archetype->PlacementKind = EGridObjectPlacementKind::Ceiling;
	Archetype->PlacementZOffset = 188.0f;
	CenterObject.LocalYaw = 0.0f;
	TestTrue(TEXT("Ceiling transform resolves"), Runtime->GetObjectPlacementTransform(CenterObject, Transform));
	TestTrue(TEXT("Ceiling currently uses absolute Z offset from the cell floor"), IsLocation(Transform, FVector(300.0f, 500.0f, 188.0f)));

	// Wall and Edge share the same wall-mounted transform contract.
	FGridLevelObjectData WallObject = MakeObject(EGridLevelObjectType::Decoration, EGridEdge::North);
	WallObject.LocalYaw = 15.0f;
	Archetype->PlacementKind = EGridObjectPlacementKind::Wall;
	Archetype->PlacementZOffset = 100.0f;
	Archetype->WallInset = 6.0f;
	Archetype->LocalOffsetAlongWall = 25.0f;
	Archetype->LocalOffsetVertical = 10.0f;
	TestTrue(TEXT("Wall transform resolves"), Runtime->GetObjectPlacementTransform(WallObject, Transform));
	TestTrue(TEXT("Wall offsets are applied in wall-local semantics"), IsLocation(Transform, FVector(325.0f, 594.0f, 110.0f)));
	TestTrue(TEXT("Wall anchor rotation plus LocalYaw is applied"), IsRotation(Transform, FRotator(0.0f, 105.0f, 0.0f)));

	Archetype->PlacementKind = EGridObjectPlacementKind::Edge;
	TestTrue(TEXT("Edge transform resolves"), Runtime->GetObjectPlacementTransform(WallObject, Transform));
	TestTrue(TEXT("Edge currently matches Wall"), IsLocation(Transform, FVector(325.0f, 594.0f, 110.0f)));

	// Doors bypass generic wall offsets and use the exact boundary transform.
	FGridLevelObjectData DoorObject = MakeObject(EGridLevelObjectType::Door, EGridEdge::North);
	Archetype->SupportedType = EGridLevelObjectType::Door;
	Archetype->PlacementKind = EGridObjectPlacementKind::Edge;
	Archetype->PlacementZOffset = 77.0f;
	Archetype->WallInset = 22.0f;
	Archetype->LocalOffsetAlongWall = 33.0f;
	Archetype->LocalOffsetVertical = 44.0f;
	TestTrue(TEXT("Door transform resolves"), Runtime->GetObjectPlacementTransform(DoorObject, Transform));
	TestTrue(TEXT("Door is anchored on the exact North boundary"), IsLocation(Transform, FVector(300.0f, 600.0f, 0.0f)));
	TestTrue(TEXT("Door uses the historical North boundary rotation"), IsRotation(Transform, FRotator::ZeroRotator));

	// Item-on-edge is a separate floor-item rule and uses a minimum 18 cm inset.
	FGridLevelObjectData ItemObject = MakeObject(EGridLevelObjectType::Item, EGridEdge::East);
	Archetype->SupportedType = EGridLevelObjectType::Item;
	Archetype->PlacementKind = EGridObjectPlacementKind::Floor;
	Archetype->PlacementZOffset = 12.0f;
	Archetype->WallInset = 6.0f;
	TestTrue(TEXT("Floor item edge transform resolves"), Runtime->GetObjectPlacementTransform(ItemObject, Transform));
	TestTrue(TEXT("Floor item edge uses the historical minimum inset"), IsLocation(Transform, FVector(382.0f, 500.0f, 12.0f)));
	TestTrue(TEXT("East floor item faces the edge"), IsRotation(Transform, FRotator(0.0f, 90.0f, 0.0f)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
