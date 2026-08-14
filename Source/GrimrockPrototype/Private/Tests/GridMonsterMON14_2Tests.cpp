#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace
{
    struct FGridMON142TestWorld
    {
        UWorld* World = nullptr;

        FGridMON142TestWorld ()
        {
            const UWorld::InitializationValues InitializationValues =
                UWorld::InitializationValues ()
                    .AllowAudioPlayback (false)
                    .RequiresHitProxies (false)
                    .CreatePhysicsScene (false)
                    .CreateNavigation (false)
                    .CreateAISystem (false)
                    .ShouldSimulatePhysics (false)
                    .SetTransactional (false);
            World = UWorld::CreateWorld (
                EWorldType::Game,
                false,
                FName (*FString::Printf (
                    TEXT ("MON142TestWorld_%s"),
                    *FGuid::NewGuid ().ToString (EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &InitializationValues);
            if (World && GEngine)
            {
                FWorldContext& Context =
                    GEngine->CreateNewWorldContext (EWorldType::Game);
                Context.SetCurrentWorld (World);
            }
        }

        ~FGridMON142TestWorld ()
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

    UGridLevelAsset* MON142MakeFloorLevel (
        UObject* Outer,
        int32 Width = 8,
        int32 Height = 8)
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset> (Outer);
        Level->Width = Width;
        Level->Height = Height;
        Level->EnsureCellCount ();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
            Cell.NorthWall = EGridWallType::None;
            Cell.EastWall = EGridWallType::None;
            Cell.SouthWall = EGridWallType::None;
            Cell.WestWall = EGridWallType::None;
        }
        return Level;
    }

    UGridMonsterDefinitionAsset* MON142MakeDefinition (
        UObject* Outer,
        FName MonsterId)
    {
        UGridMonsterDefinitionAsset* Definition =
            NewObject<UGridMonsterDefinitionAsset> (Outer);
        Definition->MonsterId = MonsterId;
        Definition->DisplayName = FText::FromName (MonsterId);
        Definition->CategoryId = TEXT ("MON14_Test");
        Definition->MaxHealth = 10;
        Definition->ActionPointsPerTurn = 2;
        Definition->SightRangeCells = 6;
        Definition->HearingRangeCells = 0;
        Definition->DeathExpectedDuration = 1.0f;
        return Definition;
    }

    FGridLevelObjectData MON142MakeSpawn (
        UGridMonsterDefinitionAsset* Definition,
        FGuid SpawnId,
        FIntPoint Cell = FIntPoint (2, 2))
    {
        FGridLevelObjectData Spawn;
        Spawn.ObjectId = SpawnId;
        Spawn.Type = EGridLevelObjectType::MonsterSpawn;
        Spawn.CellX = Cell.X;
        Spawn.CellY = Cell.Y;
        Spawn.Edge = EGridEdge::None;
        Spawn.InitialFacing = EGridEdge::North;
        Spawn.InitialMonsterState = EGridMonsterState::Idle;
        Spawn.MonsterDefinitionAsset = Definition;
        Spawn.MonsterDefinitionId = Definition
            ? Definition->MonsterId
            : NAME_None;
        Spawn.bInitiallyEnabled = true;
        return Spawn;
    }

    bool MON142HasErrorContaining (
        const TArray<FString>& Errors,
        const TCHAR* Expected)
    {
        return Errors.ContainsByPredicate (
            [Expected] (const FString& Error)
            {
                return Error.Contains (Expected);
            });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON142DirectionalSightTest,
    "Grimrock.Monsters.MON14.2.DirectionalSight",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON142DirectionalSightTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FIntPoint Observer (2, 2);
    const auto OpenTraversal = [] (
        const FIntPoint& From,
        const FIntPoint& To)
    {
        return FGridMonsterPathfinder::ManhattanDistance (From, To) == 1;
    };

    TestTrue (TEXT ("North sees only the north axial ray"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::North, FIntPoint (2, 5), 5, OpenTraversal));
    TestFalse (TEXT ("North does not see behind itself"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::North, FIntPoint (2, 0), 5, OpenTraversal));
    TestFalse (TEXT ("North does not see sideways"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::North, FIntPoint (5, 2), 5, OpenTraversal));

    TestTrue (TEXT ("East sees east"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::East, FIntPoint (5, 2), 5, OpenTraversal));
    TestTrue (TEXT ("South sees south"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::South, FIntPoint (2, 0), 5, OpenTraversal));
    TestTrue (TEXT ("West sees west"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::West, FIntPoint (0, 2), 5, OpenTraversal));
    TestFalse (TEXT ("Facing None never creates a visual ray"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::None, FIntPoint (2, 3), 5, OpenTraversal));
    TestFalse (TEXT ("Directional sight still enforces range"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::North, FIntPoint (2, 7), 4, OpenTraversal));

    const auto BlockNorthEdge = [] (
        const FIntPoint& From,
        const FIntPoint& To)
    {
        return !(From == FIntPoint (2, 3) && To == FIntPoint (2, 4));
    };
    TestFalse (TEXT ("Directional sight still respects blocking edges"),
        FGridMonsterPerception::HasDirectionalLineOfSight (
            Observer, EGridEdge::North, FIntPoint (2, 5), 5, BlockNorthEdge));

    TestTrue (TEXT ("Legacy straight LOS remains geometry-only"),
        FGridMonsterPerception::HasStraightLineOfSight (
            Observer, FIntPoint (2, 0), 5, OpenTraversal));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON142SpawnModelValidationTest,
    "Grimrock.Monsters.MON14.2.SpawnModelValidation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON142SpawnModelValidationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridLevelAsset* Level = MON142MakeFloorLevel (GetTransientPackage ());
    UGridMonsterDefinitionAsset* Definition = MON142MakeDefinition (
        Level,
        TEXT ("MON14_2_ValidationRat"));

    FGridLevelObjectData Spawn = MON142MakeSpawn (
        Definition,
        FGuid (14, 2, 1, 1));
    Spawn.InitialMonsterState = EGridMonsterState::Dormant;
    Spawn.PatrolMode = EGridMonsterPatrolMode::Loop;

    FGridMonsterPatrolWaypoint First;
    First.Cell = FIntPoint (2, 2);
    First.Facing = EGridEdge::East;
    First.WaitSeconds = 0.5f;
    FGridMonsterPatrolWaypoint Second;
    Second.Cell = FIntPoint (5, 2);
    Second.Facing = EGridEdge::West;
    Second.WaitSeconds = 1.0f;
    Spawn.PatrolWaypoints = { First, Second };
    Level->Objects.Add (Spawn);

    TArray<FString> Errors;
    TestTrue (TEXT ("Dormant spawn with a two-point loop validates"),
        Level->ValidateMonsterSpawns (Errors));
    TestTrue (TEXT ("Valid MON14.2 model has no errors"), Errors.IsEmpty ());

    Level->Objects[0].InitialMonsterState = EGridMonsterState::Alert;
    TestFalse (TEXT ("Alert is not an authored fresh-spawn state"),
        Level->ValidateMonsterSpawns (Errors));
    TestTrue (TEXT ("Invalid initial state is reported"),
        MON142HasErrorContaining (Errors, TEXT ("InitialMonsterState Idle or Dormant")));

    Level->Objects[0].InitialMonsterState = EGridMonsterState::Dormant;
    Level->Objects[0].PatrolWaypoints.SetNum (1);
    TestFalse (TEXT ("A live patrol requires two waypoints"),
        Level->ValidateMonsterSpawns (Errors));
    TestTrue (TEXT ("Short patrol is reported"),
        MON142HasErrorContaining (Errors, TEXT ("requires at least two waypoints")));

    Level->Objects[0].PatrolWaypoints = { First, Second };
    Level->Objects[0].PatrolWaypoints[1].Cell = FIntPoint (99, 99);
    TestFalse (TEXT ("Out-of-bounds patrol waypoint is rejected"),
        Level->ValidateMonsterSpawns (Errors));
    TestTrue (TEXT ("Out-of-bounds waypoint is reported"),
        MON142HasErrorContaining (Errors, TEXT ("patrol waypoint 1 is outside grid bounds")));

    Level->Objects[0].PatrolWaypoints[1] = Second;
    Level->Objects[0].PatrolWaypoints[1].WaitSeconds = -1.0f;
    TestFalse (TEXT ("Negative waypoint wait is rejected"),
        Level->ValidateMonsterSpawns (Errors));
    TestTrue (TEXT ("Invalid wait is reported"),
        MON142HasErrorContaining (Errors, TEXT ("finite non-negative WaitSeconds")));

    Level->Objects[0].PatrolWaypoints[1] = Second;
    Level->Objects[0].PatrolWaypoints[1].Facing =
        static_cast<EGridEdge> (255);
    TestFalse (TEXT ("Invalid waypoint facing is rejected"),
        Level->ValidateMonsterSpawns (Errors));
    TestTrue (TEXT ("Invalid waypoint facing is reported"),
        MON142HasErrorContaining (Errors, TEXT ("Facing=None or a cardinal direction")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON142FreshSpawnConfigurationTest,
    "Grimrock.Monsters.MON14.2.FreshSpawnConfiguration",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON142FreshSpawnConfigurationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON142TestWorld TestWorld;
    if (!TestWorld.World)
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    TestNotNull (TEXT ("Runtime exists"), Runtime);
    if (!Runtime)
    {
        return false;
    }
    Runtime->bApplyLevelStartOnBeginPlay = false;
    Runtime->LevelAsset = MON142MakeFloorLevel (Runtime);

    UGridMonsterDefinitionAsset* Definition = MON142MakeDefinition (
        Runtime,
        TEXT ("MON14_2_FreshRat"));
    const FGuid SpawnId (14, 2, 2, 1);
    FGridLevelObjectData Spawn = MON142MakeSpawn (
        Definition,
        SpawnId,
        FIntPoint (3, 3));
    Spawn.InitialFacing = EGridEdge::South;
    Spawn.InitialMonsterState = EGridMonsterState::Dormant;
    Spawn.EncounterGroupId = TEXT ("PatrolRoom_A");
    Spawn.PatrolMode = EGridMonsterPatrolMode::PingPong;

    FGridMonsterPatrolWaypoint A;
    A.Cell = FIntPoint (3, 3);
    A.Facing = EGridEdge::East;
    A.WaitSeconds = 0.25f;
    FGridMonsterPatrolWaypoint B;
    B.Cell = FIntPoint (6, 3);
    B.Facing = EGridEdge::West;
    B.WaitSeconds = 0.75f;
    Spawn.PatrolWaypoints = { A, B };
    Runtime->LevelAsset->Objects.Add (Spawn);

    FActorSpawnParameters Params;
    Params.Owner = Runtime;
    AGridMonsterActor* Monster =
        TestWorld.World->SpawnActor<AGridMonsterActor> (
            AGridMonsterActor::StaticClass (),
            Runtime->GetCellCenterWorld (3, 3),
            FRotator::ZeroRotator,
            Params);
    TestNotNull (TEXT ("Fresh monster exists"), Monster);
    if (!Monster)
    {
        return false;
    }

    TestTrue (TEXT ("Fresh monster initializes from its MonsterSpawn"),
        Monster->InitializeMonster (
            Definition,
            SpawnId,
            FIntPoint (3, 3),
            EGridEdge::South,
            Spawn.EncounterGroupId));
    TestEqual (TEXT ("Authored Dormant state reaches the Actor"),
        Monster->MonsterState,
        EGridMonsterState::Dormant);
    TestEqual (TEXT ("Authored facing remains authoritative"),
        Monster->Facing,
        EGridEdge::South);
    TestEqual (TEXT ("Encounter group remains intact"),
        Monster->EncounterGroupId,
        FName (TEXT ("PatrolRoom_A")));
    TestEqual (TEXT ("Patrol mode reaches the Actor"),
        Monster->PatrolMode,
        EGridMonsterPatrolMode::PingPong);
    TestEqual (TEXT ("Both patrol waypoints reach the Actor"),
        Monster->PatrolWaypoints.Num (),
        2);
    if (Monster->PatrolWaypoints.Num () == 2)
    {
        TestTrue (TEXT ("First patrol cell is preserved"),
            Monster->PatrolWaypoints[0].Cell == FIntPoint (3, 3));
        TestEqual (TEXT ("Second patrol facing is preserved"),
            Monster->PatrolWaypoints[1].Facing,
            EGridEdge::West);
        TestEqual (TEXT ("Waypoint wait is preserved"),
            Monster->PatrolWaypoints[1].WaitSeconds,
            0.75f);
    }

    TestFalse (TEXT ("MON14.2 route data does not start movement"),
        Monster->bIsMoving);
    TestFalse (TEXT ("MON14.2 route data does not start turning"),
        Monster->bIsTurning);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON142BehaviorFacingIntegrationTest,
    "Grimrock.Monsters.MON14.2.BehaviorFacingIntegration",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON142BehaviorFacingIntegrationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON142TestWorld TestWorld;
    if (!TestWorld.World)
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    AGrimrockPartyPawn* Party =
        TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
    if (!Runtime || !Party)
    {
        return false;
    }
    Runtime->bApplyLevelStartOnBeginPlay = false;
    Runtime->LevelAsset = MON142MakeFloorLevel (Runtime);

    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 2;
    Party->CurrentCellY = 5;
    Party->SetActorLocation (Runtime->GetCellCenterWorld (2, 5));

    UGridMonsterDefinitionAsset* Definition = MON142MakeDefinition (
        Runtime,
        TEXT ("MON14_2_FacingRat"));
    const FGuid SpawnId (14, 2, 3, 1);
    FGridLevelObjectData Spawn = MON142MakeSpawn (
        Definition,
        SpawnId,
        FIntPoint (2, 2));
    Spawn.InitialFacing = EGridEdge::North;
    Runtime->LevelAsset->Objects.Add (Spawn);

    FActorSpawnParameters Params;
    Params.Owner = Runtime;
    AGridMonsterActor* Monster =
        TestWorld.World->SpawnActor<AGridMonsterActor> (
            AGridMonsterActor::StaticClass (),
            Runtime->GetCellCenterWorld (2, 2),
            FRotator::ZeroRotator,
            Params);
    if (!Monster ||
        !Monster->InitializeMonster (
            Definition,
            SpawnId,
            FIntPoint (2, 2),
            EGridEdge::North))
    {
        return false;
    }

    UGridMonsterBehaviorComponent* Behavior =
        NewObject<UGridMonsterBehaviorComponent> (
            Monster,
            TEXT ("MON142Behavior"));
    Behavior->bAutoInitialize = false;
    Monster->AddInstanceComponent (Behavior);
    Behavior->RegisterComponent ();
    TestTrue (TEXT ("Behavior initializes"),
        Behavior->InitializeBehavior (Runtime, Party));

    TestTrue (TEXT ("North-facing monster sees party to the north"),
        Behavior->RefreshPerception ());
    TestTrue (TEXT ("Behavior records north-facing visual contact"),
        Behavior->bCanSeeParty);

    Monster->Facing = EGridEdge::South;
    TestFalse (TEXT ("Turning away removes all perception when hearing is zero"),
        Behavior->RefreshPerception ());
    TestFalse (TEXT ("Behavior no longer sees behind the monster"),
        Behavior->bCanSeeParty);
    TestFalse (TEXT ("Zero-range hearing does not mask the facing test"),
        Behavior->bCanHearParty);

    Party->CurrentCellX = 2;
    Party->CurrentCellY = 0;
    Party->SetActorLocation (Runtime->GetCellCenterWorld (2, 0));
    TestTrue (TEXT ("South-facing monster sees party after it moves south"),
        Behavior->RefreshPerception ());
    TestTrue (TEXT ("Behavior records south-facing visual contact"),
        Behavior->bCanSeeParty);
    return true;
}

#endif
