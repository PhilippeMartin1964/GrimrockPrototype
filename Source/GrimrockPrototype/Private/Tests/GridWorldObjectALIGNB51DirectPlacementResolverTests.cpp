#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPlacementTransformResolver.h"

namespace GridWorldObjectALIGNB51
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
				FName(*FString::Printf(TEXT("WORLDOBJ_ALIGN_B5_1_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectALIGNB51DirectPlacementResolverTest,
	"Grimrock.WorldObjects.ALIGN_B5_1.DirectPlacementResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectALIGNB51DirectPlacementResolverTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const TArray<FName> RemovedPlacementProperties = {
		TEXT("PlacementKind"), TEXT("PlacementZOffset"), TEXT("WallInset"), TEXT("LocalOffsetAlongWall"), TEXT("LocalOffsetVertical")
	};
	for (const FName PropertyName : RemovedPlacementProperties)
	{
		TestNull(*FString::Printf(TEXT("%s is absent from the definition"), *PropertyName.ToString()),
			UGridWorldObjectDefinitionAsset::StaticClass()->FindPropertyByName(PropertyName));
	}

	GridWorldObjectALIGNB51::FTestWorld TestWorld;
	if (!TestNotNull(TEXT("ALIGN-B5.1 world exists"), TestWorld.World))
	{
		return false;
	}
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime))
	{
		return false;
	}
	Runtime->LevelAsset = NewObject<UGridLevelAsset>(Runtime);
	Runtime->LevelAsset->Width = 4;
	Runtime->LevelAsset->Height = 4;
	Runtime->LevelAsset->CellSize = 200.0f;
	Runtime->LevelAsset->EnsureCellCount();
	Runtime->GridOrigin = FVector::ZeroVector;
	Runtime->SetActorLocation(FVector::ZeroVector);

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	Definition->DefinitionId = TEXT("WORLDOBJ_ALIGN_B5_1_Test");
	Definition->SupportedType = EGridLevelObjectType::Decoration;
	Runtime->WorldObjectDefinitions.Add(Definition);

	FGridWorldObjectInstance Object;
	Object.InstanceId = FGuid::NewGuid();
	Object.Type = EGridLevelObjectType::Decoration;
	Object.WorldObjectDefinitionId = Definition->DefinitionId;
	Object.CellX = 1;
	Object.CellY = 2;
	Object.WallSide = EGridEdge::North;
	Object.LocalTransformOverride = FTransform(FRotator(10.0f, 30.0f, 20.0f), FVector(41.0f, 42.0f, 43.0f), FVector(2.0f));

	const auto CheckTransform = [&](const TCHAR* Label, const FVector& Location, float Yaw)
	{
		FTransform Transform;
		if (TestTrue(*FString::Printf(TEXT("%s resolves directly from the authored surface"), Label),
			GridPlacementTransformResolver::ResolveWorldObject(*Runtime, Object, Transform)))
		{
			TestTrue(*FString::Printf(TEXT("%s preserves the full transform"), Label),
				Transform.Equals(FTransform(FRotator(0.0f, Yaw, 0.0f), Location, FVector::OneVector), 0.01f));
		}
	};

	Definition->PlacementSurface = EGridObjectPlacementKind::Floor;
	Definition->DefaultLocalPosition.U = 0.0f;
	Definition->DefaultLocalPosition.V = 0.0f;
	Definition->DefaultLocalPosition.N = 12.0f;
	CheckTransform(TEXT("Floor N=12"), FVector(300.0f, 500.0f, 12.0f), 0.0f);

	// Nonzero U/V remain ignored; only the enabled override's yaw affects centered placement.
	Definition->DefaultLocalPosition.U = 25.0f;
	Definition->DefaultLocalPosition.V = 110.0f;
	CheckTransform(TEXT("Floor ignores U/V and disabled override"), FVector(300.0f, 500.0f, 12.0f), 0.0f);
	Object.bHasLocalTransformOverride = true;
	CheckTransform(TEXT("Floor keeps override yaw only"), FVector(300.0f, 500.0f, 12.0f), 30.0f);

	Definition->PlacementSurface = EGridObjectPlacementKind::Ceiling;
	CheckTransform(TEXT("Ceiling ignores U/V and keeps override yaw only"), FVector(300.0f, 500.0f, 188.0f), 30.0f);
	Object.bHasLocalTransformOverride = false;
	CheckTransform(TEXT("Ceiling ignores disabled override"), FVector(300.0f, 500.0f, 188.0f), 0.0f);
	Runtime->LevelAsset->CellSize = 300.0f;
	CheckTransform(TEXT("Ceiling plane stays 200 cm with a different cell size"), FVector(450.0f, 750.0f, 188.0f), 0.0f);
	Runtime->LevelAsset->CellSize = 200.0f;

	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	Definition->DefaultLocalPosition.N = 6.0f;
	Object.bHasLocalTransformOverride = true;
	CheckTransform(TEXT("North Wall reads U/V/N and ignores override"), FVector(325.0f, 594.0f, 110.0f), 90.0f);
	Object.bHasLocalTransformOverride = false;
	CheckTransform(TEXT("North Wall without override keeps the same transform"), FVector(325.0f, 594.0f, 110.0f), 90.0f);

	// Unsupported authoring surfaces must still be rejected.
	FTransform Transform;
	Definition->PlacementSurface = EGridObjectPlacementKind::Center;
	TestFalse(TEXT("Center is rejected"), GridPlacementTransformResolver::ResolveWorldObject(*Runtime, Object, Transform));
	Definition->PlacementSurface = EGridObjectPlacementKind::Edge;
	TestFalse(TEXT("Edge is rejected"), GridPlacementTransformResolver::ResolveWorldObject(*Runtime, Object, Transform));

	Object.Type = EGridLevelObjectType::Door;
	Object.bHasLocalTransformOverride = true;
	Definition->SupportedType = EGridLevelObjectType::Door;
	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	CheckTransform(TEXT("North Door ignores nonzero U/V/N and override"), FVector(300.0f, 600.0f, 0.0f), 0.0f);
	Definition->DefaultLocalPosition.U = -45.0f;
	Definition->DefaultLocalPosition.V = 240.0f;
	Definition->DefaultLocalPosition.N = 19.0f;
	CheckTransform(TEXT("North Door stays on boundary after U/V/N change"), FVector(300.0f, 600.0f, 0.0f), 0.0f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
