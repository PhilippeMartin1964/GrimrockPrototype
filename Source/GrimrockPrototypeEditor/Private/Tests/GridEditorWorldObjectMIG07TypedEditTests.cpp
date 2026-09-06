#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	struct FMIG07CEditorTestWorld
	{
		UWorld* World = nullptr;

		FMIG07CEditorTestWorld()
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
				FName(*FString::Printf(TEXT("MIG07CEditorWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FMIG07CEditorTestWorld()
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
	FGridEditorWorldObjectMIG07TypedWriteThroughTest,
	"Grimrock.WorldObjects.MIG07.EditorTypedWriteThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorWorldObjectMIG07TypedWriteThroughTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FMIG07CEditorTestWorld TestWorld;
	TestNotNull(TEXT("MIG07-C editor world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* EditorActor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNotNull(TEXT("MIG07-C editor actor exists"), EditorActor);
	if (!EditorActor)
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(EditorActor);
	Level->Width = 12;
	Level->Height = 12;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}

	UGridItemDefinitionAsset* ItemDefinition = NewObject<UGridItemDefinitionAsset>(EditorActor);
	ItemDefinition->ItemDefinitionId = TEXT("MIG07C_EditorItem");

	FGridLevelObjectData Item;
	Item.ObjectId = FGuid::NewGuid();
	Item.Type = EGridLevelObjectType::Item;
	Item.ItemDefinitionAsset = ItemDefinition;
	Item.CellX = 1;
	Item.CellY = 1;
	Level->Objects.Add(Item);

	FGridLevelObjectData Monster;
	Monster.ObjectId = FGuid::NewGuid();
	Monster.Type = EGridLevelObjectType::MonsterSpawn;
	Monster.CellX = 5;
	Monster.CellY = 5;
	Monster.InitialFacing = EGridEdge::North;
	Monster.bInitiallyEnabled = true;
	Level->Objects.Add(Monster);

	Level->EnableTypedPlacementStorageFromLegacy();
	Level->LooseItemInstances[0].Quantity = 9;
	Level->LooseItemInstances[0].LocalOffset = FVector(3.0f, 4.0f, 5.0f);
	Level->RefreshLegacyObjectMirrorFromTyped();
	EditorActor->LevelAsset = Level;

	TestTrue(TEXT("Typed loose item can be selected through compatibility editor"), EditorActor->SelectObjectById(Item.ObjectId));
	TestTrue(TEXT("Tag edit succeeds"), EditorActor->SetSelectedObjectTag(TEXT("EditedItem")));
	TestEqual(TEXT("Tag edit writes through to typed item"), Level->LooseItemInstances[0].Tag, FName(TEXT("EditedItem")));
	TestEqual(TEXT("Tag edit preserves typed-only item quantity"), Level->LooseItemInstances[0].Quantity, 9);
	TestTrue(TEXT("Tag edit preserves typed-only item local offset"), Level->LooseItemInstances[0].LocalOffset.Equals(FVector(3.0f, 4.0f, 5.0f)));

	EditorActor->SelectedCellX = 7;
	EditorActor->SelectedCellY = 8;
	EditorActor->SelectedEdge = EGridEdge::None;
	TestTrue(TEXT("Move selected item succeeds"), EditorActor->MoveSelectedObjectToCurrentSelection());
	TestEqual(TEXT("Move writes CellX to typed item"), Level->LooseItemInstances[0].CellX, 7);
	TestEqual(TEXT("Move writes CellY to typed item"), Level->LooseItemInstances[0].CellY, 8);
	TestEqual(TEXT("Move still preserves typed-only quantity"), Level->LooseItemInstances[0].Quantity, 9);

	EditorActor->RemoveObjectsAtSelection();
	TestEqual(TEXT("Erase path removes typed loose item"), Level->LooseItemInstances.Num(), 0);
	TestFalse(TEXT("Removed item disappears from compatibility mirror"), Level->GetObjectCompatibilityView().ContainsByPredicate(
		[&Item](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Item.ObjectId;
		}));

	TestTrue(TEXT("Typed monster can be selected through compatibility editor"), EditorActor->SelectObjectById(Monster.ObjectId));
	EditorActor->HoveredCellX = 5;
	EditorActor->HoveredCellY = 6;
	TestTrue(TEXT("First patrol waypoint can be added"), EditorActor->AddOrSelectPatrolWaypointAtHoveredCell());
	EditorActor->HoveredCellX = 6;
	EditorActor->HoveredCellY = 6;
	TestTrue(TEXT("Second patrol waypoint can be added"), EditorActor->AddOrSelectPatrolWaypointAtHoveredCell());
	TestEqual(TEXT("Patrol write-through stores two typed waypoints"), Level->MonsterSpawns[0].PatrolWaypoints.Num(), 2);
	TestEqual(TEXT("Two waypoints automatically enable typed Loop patrol"), Level->MonsterSpawns[0].PatrolMode, EGridMonsterPatrolMode::Loop);

	TestTrue(TEXT("Patrol wait edit succeeds"), EditorActor->SetSelectedPatrolWaypointWaitSeconds(1.5f));
	TestTrue(TEXT("Patrol wait writes through to typed spawn"),
		Level->MonsterSpawns[0].PatrolWaypoints.IsValidIndex(EditorActor->SelectedPatrolWaypointIndex) &&
			FMath::IsNearlyEqual(Level->MonsterSpawns[0].PatrolWaypoints[EditorActor->SelectedPatrolWaypointIndex].WaitSeconds, 1.5f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
