#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridRuntimeObjectActor.h"

namespace
{
	struct FALIGNB52EditorTestWorld
	{
		UWorld* World = nullptr;

		FALIGNB52EditorTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
				FName(*FString::Printf(TEXT("ALIGNB52EditorWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FALIGNB52EditorTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorALIGNB52DirectPlacementTest,
	"Grimrock.Editor.ALIGN_B5_2.DirectPlacementEditor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorALIGNB52DirectPlacementTest::RunTest(const FString& Parameters)
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

	FALIGNB52EditorTestWorld TestWorld;
	if (!TestNotNull(TEXT("Editor test world exists"), TestWorld.World)) return false;
	AGridLevelEditorActor* Editor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	if (!TestNotNull(TEXT("Editor actor exists"), Editor)) return false;
	Editor->SetActorLocation(FVector::ZeroVector);

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
	Level->Width = 4;
	Level->Height = 4;
	Level->CellSize = 200.0f;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Editor->LevelAsset = Level;

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Editor);
	Definition->DefinitionId = TEXT("ALIGN_B5_2_Decoration");
	Definition->SupportedType = EGridLevelObjectType::Decoration;
	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	Definition->DefaultLocalPosition.U = 25.0f;
	Definition->DefaultLocalPosition.V = 110.0f;
	Definition->DefaultLocalPosition.N = 6.0f;

	Editor->ObjectPalette = NewObject<UGridObjectPaletteAsset>(Editor);
	FGridObjectPaletteEntry& Entry = Editor->ObjectPalette->Entries.AddDefaulted_GetRef();
	Entry.EntryId = Definition->DefinitionId;
	Entry.DefaultWorldObjectDefinition = Definition;
	Editor->SelectedCellX = 1;
	Editor->SelectedCellY = 2;
	Editor->SelectedEdge = EGridEdge::North;
	if (!TestTrue(TEXT("Palette entry applies"), Editor->ApplyPaletteEntry(Entry.EntryId))) return false;
	Editor->PlaceSelectedObject();
	if (!TestEqual(TEXT("Wall placement creates one object"), Level->WorldObjectInstances.Num(), 1)) return false;
	FGridWorldObjectInstance& Object = Level->WorldObjectInstances[0];
	const FGuid ObjectId = Object.InstanceId;
	TestEqual(TEXT("Placement uses the authored Wall surface"), Object.WallSide, EGridEdge::North);
	TestTrue(TEXT("Placed wall can be selected"), Editor->SelectObjectById(ObjectId));
	TestTrue(TEXT("Wall orientation can be changed"), Editor->SetSelectedObjectOrientation(EGridEdge::East));
	TestEqual(TEXT("Wall classification changes WallSide"), Object.WallSide, EGridEdge::East);
	TestFalse(TEXT("Wall orientation does not create a local transform override"), Object.bHasLocalTransformOverride);

	const auto CheckCenter = [&](const TCHAR* Label, const FVector& Expected)
	{
		TestTrue(TEXT("PreviewRuntimeActor remains absent"), Editor->PreviewRuntimeActor == nullptr);
		FVector Center;
		if (TestTrue(*FString::Printf(TEXT("%s center resolves"), Label), Editor->GetObjectEditorWorldCenter(ObjectId, Center)))
		{
			TestTrue(*FString::Printf(TEXT("%s center preserves placement parity"), Label), Center.Equals(Expected, 0.01f));
		}
	};
	Object.bHasLocalTransformOverride = true;
	Object.LocalTransformOverride = FTransform(FRotator(10.0f, 30.0f, 20.0f), FVector(41.0f, 42.0f, 43.0f), FVector(2.0f));
	Object.WallSide = EGridEdge::North;
	CheckCenter(TEXT("North Wall"), FVector(325.0f, 594.0f, 110.0f));
	Object.WallSide = EGridEdge::South;
	CheckCenter(TEXT("South Wall"), FVector(275.0f, 406.0f, 110.0f));
	Object.WallSide = EGridEdge::East;
	CheckCenter(TEXT("East Wall"), FVector(394.0f, 475.0f, 110.0f));
	Object.WallSide = EGridEdge::West;
	CheckCenter(TEXT("West Wall"), FVector(206.0f, 525.0f, 110.0f));
	Object.WallSide = EGridEdge::North;
	Object.bHasLocalTransformOverride = false;
	CheckCenter(TEXT("Wall without override keeps the same center"), FVector(325.0f, 594.0f, 110.0f));
	Object.bHasLocalTransformOverride = true;

	Definition->PlacementSurface = EGridObjectPlacementKind::Floor;
	Definition->DefaultLocalPosition.N = 12.0f;
	CheckCenter(TEXT("Floor ignores nonzero U/V and transform override"), FVector(300.0f, 500.0f, 12.0f));
	TestTrue(TEXT("Floor orientation can be changed"), Editor->SetSelectedObjectOrientation(EGridEdge::East));
	TestEqual(TEXT("Floor classification does not change WallSide"), Object.WallSide, EGridEdge::North);
	TestFalse(TEXT("Floor orientation changes the local yaw"), FMath::IsNearlyEqual(Object.LocalTransformOverride.Rotator().Yaw, 30.0));

	Definition->PlacementSurface = EGridObjectPlacementKind::Ceiling;
	CheckCenter(TEXT("Ceiling ignores nonzero U/V and transform override"), FVector(300.0f, 500.0f, 188.0f));
	Level->CellSize = 300.0f;
	CheckCenter(TEXT("Ceiling plane remains 200 cm with a different cell size"), FVector(450.0f, 750.0f, 188.0f));
	Level->CellSize = 200.0f;

	Object.Type = EGridLevelObjectType::Door;
	Definition->SupportedType = EGridLevelObjectType::Door;
	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	CheckCenter(TEXT("North Door ignores U/V/N and override"), FVector(300.0f, 600.0f, 150.0f));
	Definition->DefaultLocalPosition.U = -45.0f;
	Definition->DefaultLocalPosition.V = 240.0f;
	Definition->DefaultLocalPosition.N = 19.0f;
	CheckCenter(TEXT("North Door after changing coordinates"), FVector(300.0f, 600.0f, 150.0f));
	Object.WallSide = EGridEdge::South;
	CheckCenter(TEXT("South Door"), FVector(300.0f, 400.0f, 150.0f));
	Object.WallSide = EGridEdge::East;
	CheckCenter(TEXT("East Door"), FVector(400.0f, 500.0f, 150.0f));
	Object.WallSide = EGridEdge::West;
	CheckCenter(TEXT("West Door"), FVector(200.0f, 500.0f, 150.0f));

	Object.WorldObjectDefinitionId = TEXT("ALIGN_B5_2_Missing");
	Object.WallSide = EGridEdge::North;
	CheckCenter(TEXT("Door without definition"), FVector(300.0f, 600.0f, 150.0f));
	Object.Type = EGridLevelObjectType::Button;
	CheckCenter(TEXT("Button without definition"), FVector(300.0f, 594.0f, 12.0f));
	Object.Type = EGridLevelObjectType::Decoration;
	CheckCenter(TEXT("Decoration without definition"), FVector(300.0f, 500.0f, 12.0f));

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor exists for spawn classification"), Runtime)) return false;
	Runtime->LevelAsset = Level;
	Runtime->WorldObjectDefinitions.Add(Definition);
	Definition->RuntimeActorClass = AGridRuntimeObjectActor::StaticClass();
	FGridWorldObjectInstance RuntimeObject = Object;
	RuntimeObject.WorldObjectDefinitionId = Definition->DefinitionId;
	RuntimeObject.WallSide = EGridEdge::None;
	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	TestFalse(TEXT("Wall spawn requires WallSide"), Runtime->IsRuntimeSpawnableObject(RuntimeObject));
	RuntimeObject.WallSide = EGridEdge::North;
	TestTrue(TEXT("Wall spawn accepts a cardinal WallSide"), Runtime->IsRuntimeSpawnableObject(RuntimeObject));
	RuntimeObject.WallSide = EGridEdge::None;
	Definition->PlacementSurface = EGridObjectPlacementKind::Floor;
	TestTrue(TEXT("Floor spawn needs no WallSide"), Runtime->IsRuntimeSpawnableObject(RuntimeObject));
	Definition->PlacementSurface = EGridObjectPlacementKind::Ceiling;
	TestTrue(TEXT("Ceiling spawn needs no WallSide"), Runtime->IsRuntimeSpawnableObject(RuntimeObject));
	TestTrue(TEXT("Placed world-object presence is implied by the placement itself"), Runtime->IsRuntimeSpawnableObject(RuntimeObject));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
