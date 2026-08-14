#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
    struct FGridMON1431EditorTestWorld
    {
        UWorld* World = nullptr;

        FGridMON1431EditorTestWorld ()
        {
            const UWorld::InitializationValues Values =
                UWorld::InitializationValues ()
                    .AllowAudioPlayback (false)
                    .RequiresHitProxies (false)
                    .CreatePhysicsScene (false)
                    .CreateNavigation (false)
                    .CreateAISystem (false)
                    .ShouldSimulatePhysics (false)
                    .SetTransactional (false);
            World = UWorld::CreateWorld (
                EWorldType::EditorPreview,
                false,
                FName (*FString::Printf (
                    TEXT ("MON1431EditorTestWorld_%s"),
                    *FGuid::NewGuid ().ToString (EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &Values);
            if (World && GEngine)
            {
                FWorldContext& Context =
                    GEngine->CreateNewWorldContext (EWorldType::EditorPreview);
                Context.SetCurrentWorld (World);
            }
        }

        ~FGridMON1431EditorTestWorld ()
        {
            if (!World)
            {
                return;
            }
            World->DestroyWorld (false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext (World);
            }
        }
    };

    struct FGridMON1431EditorFixture
    {
        FGridMON1431EditorTestWorld TestWorld;
        AGridLevelEditorActor* EditorActor = nullptr;
        UGridLevelAsset* LevelAsset = nullptr;
        FGuid MonsterSpawnId;

        bool Initialize ()
        {
            if (!TestWorld.World)
            {
                return false;
            }

            EditorActor = TestWorld.World->SpawnActor<AGridLevelEditorActor> ();
            if (!EditorActor)
            {
                return false;
            }

            LevelAsset = NewObject<UGridLevelAsset> (EditorActor);
            LevelAsset->Width = 8;
            LevelAsset->Height = 8;
            LevelAsset->EnsureCellCount ();
            for (FGridLevelCellData& Cell : LevelAsset->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
                Cell.NorthWall = EGridWallType::None;
                Cell.EastWall = EGridWallType::None;
                Cell.SouthWall = EGridWallType::None;
                Cell.WestWall = EGridWallType::None;
            }
            EditorActor->LevelAsset = LevelAsset;

            FGridLevelObjectData MonsterSpawn;
            MonsterSpawnId = FGuid::NewGuid ();
            MonsterSpawn.ObjectId = MonsterSpawnId;
            MonsterSpawn.Type = EGridLevelObjectType::MonsterSpawn;
            MonsterSpawn.CellX = 2;
            MonsterSpawn.CellY = 2;
            MonsterSpawn.InitialFacing = EGridEdge::North;
            MonsterSpawn.bInitiallyEnabled = true;
            LevelAsset->Objects.Add (MonsterSpawn);

            return EditorActor->SelectObjectById (MonsterSpawnId);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1431PatrolRouteEditingModelTest,
    "Grimrock.Editor.MON14.3.1.PatrolRouteEditingModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1431PatrolRouteEditingModelTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1431EditorFixture Fixture;
    TestTrue (TEXT ("Fixture initializes"), Fixture.Initialize ());
    if (!Fixture.EditorActor || !Fixture.LevelAsset)
    {
        return false;
    }

    TestTrue (TEXT ("Selected MonsterSpawn supports patrol editing"),
        Fixture.EditorActor->CanEditSelectedMonsterPatrolRoute ());

    Fixture.EditorActor->ToggleSelectedMonsterPatrolRouteEditing ();
    TestTrue (TEXT ("Patrol route edit mode activates"),
        Fixture.EditorActor->IsPatrolRouteEditModeActive ());

    Fixture.EditorActor->HoveredCellX = 1;
    Fixture.EditorActor->HoveredCellY = 1;
    TestTrue (TEXT ("First waypoint is added"),
        Fixture.EditorActor->AddOrSelectPatrolWaypointAtHoveredCell ());

    const FGridLevelObjectData* Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestNotNull (TEXT ("Selected spawn remains available"), Spawn);
    if (!Spawn)
    {
        return false;
    }
    TestEqual (TEXT ("One waypoint stored"), Spawn->PatrolWaypoints.Num (), 1);
    TestEqual (TEXT ("Single waypoint keeps patrol disabled"),
        Spawn->PatrolMode, EGridMonsterPatrolMode::None);

    Fixture.EditorActor->HoveredCellX = 4;
    Fixture.EditorActor->HoveredCellY = 1;
    TestTrue (TEXT ("Second waypoint is added"),
        Fixture.EditorActor->AddOrSelectPatrolWaypointAtHoveredCell ());
    Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestEqual (TEXT ("Two waypoints stored"), Spawn->PatrolWaypoints.Num (), 2);
    TestEqual (TEXT ("Second waypoint enables Loop by default"),
        Spawn->PatrolMode, EGridMonsterPatrolMode::Loop);
    TestEqual (TEXT ("Newest waypoint becomes selected"),
        Fixture.EditorActor->SelectedPatrolWaypointIndex, 1);

    Fixture.EditorActor->HoveredCellX = 1;
    Fixture.EditorActor->HoveredCellY = 1;
    TestTrue (TEXT ("Clicking existing waypoint selects it"),
        Fixture.EditorActor->AddOrSelectPatrolWaypointAtHoveredCell ());
    Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestEqual (TEXT ("Existing waypoint is not duplicated"),
        Spawn->PatrolWaypoints.Num (), 2);
    TestEqual (TEXT ("Existing first waypoint becomes selected"),
        Fixture.EditorActor->SelectedPatrolWaypointIndex, 0);

    Fixture.EditorActor->CycleSelectedPatrolWaypointFacing ();
    Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestEqual (TEXT ("Facing cycles None to North"),
        Spawn->PatrolWaypoints[0].Facing, EGridEdge::North);

    Fixture.EditorActor->IncreaseSelectedPatrolWaypointWait ();
    Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestTrue (TEXT ("Wait increases by half a second"),
        FMath::IsNearlyEqual (Spawn->PatrolWaypoints[0].WaitSeconds, 0.5f));

    TestTrue (TEXT ("Waypoint can move later"),
        Fixture.EditorActor->MoveSelectedPatrolWaypoint (1));
    Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestEqual (TEXT ("Moved waypoint keeps its cell"),
        Spawn->PatrolWaypoints[1].Cell, FIntPoint (1, 1));
    TestEqual (TEXT ("Moved waypoint remains selected"),
        Fixture.EditorActor->SelectedPatrolWaypointIndex, 1);

    TestTrue (TEXT ("Mode changes to PingPong"),
        Fixture.EditorActor->SetSelectedMonsterPatrolMode (
            EGridMonsterPatrolMode::PingPong));
    Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestEqual (TEXT ("PingPong stored"),
        Spawn->PatrolMode, EGridMonsterPatrolMode::PingPong);

    TestTrue (TEXT ("Selected waypoint is removed"),
        Fixture.EditorActor->RemoveSelectedPatrolWaypoint ());
    Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestEqual (TEXT ("One waypoint remains"), Spawn->PatrolWaypoints.Num (), 1);
    TestEqual (TEXT ("Patrol disables when fewer than two remain"),
        Spawn->PatrolMode, EGridMonsterPatrolMode::None);

    TestTrue (TEXT ("Route can be cleared"),
        Fixture.EditorActor->ClearSelectedMonsterPatrolRoute ());
    Spawn = Fixture.EditorActor->GetSelectedObjectData ();
    TestEqual (TEXT ("Route is empty"), Spawn->PatrolWaypoints.Num (), 0);
    TestEqual (TEXT ("No waypoint remains selected"),
        Fixture.EditorActor->SelectedPatrolWaypointIndex, INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1431PatrolRouteGuardTest,
    "Grimrock.Editor.MON14.3.1.PatrolRouteGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1431PatrolRouteGuardTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON1431EditorFixture Fixture;
    TestTrue (TEXT ("Fixture initializes"), Fixture.Initialize ());
    if (!Fixture.EditorActor)
    {
        return false;
    }

    TestFalse (TEXT ("Loop cannot be enabled without two waypoints"),
        Fixture.EditorActor->SetSelectedMonsterPatrolMode (
            EGridMonsterPatrolMode::Loop));

    Fixture.EditorActor->HoveredCellX = -1;
    Fixture.EditorActor->HoveredCellY = 99;
    TestFalse (TEXT ("Invalid hovered cell cannot create waypoint"),
        Fixture.EditorActor->AddOrSelectPatrolWaypointAtHoveredCell ());

    TestFalse (TEXT ("No waypoint can be removed before one is selected"),
        Fixture.EditorActor->RemoveSelectedPatrolWaypoint ());
    return true;
}

#endif
