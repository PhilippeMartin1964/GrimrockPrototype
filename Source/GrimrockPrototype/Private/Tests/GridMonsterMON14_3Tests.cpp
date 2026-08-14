#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterPatrolSubsystem.h"

namespace
{
    struct FGridMON143TestWorld
    {
        UWorld* World = nullptr;

        FGridMON143TestWorld ()
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
                EWorldType::Game,
                false,
                FName (*FString::Printf (
                    TEXT ("MON143TestWorld_%s"),
                    *FGuid::NewGuid ().ToString (EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &Values);
            if (World && GEngine)
            {
                FWorldContext& Context =
                    GEngine->CreateNewWorldContext (EWorldType::Game);
                Context.SetCurrentWorld (World);
            }
        }

        ~FGridMON143TestWorld ()
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

    struct FGridMON143Fixture
    {
        FGridMON143TestWorld TestWorld;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* Level = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
        UGridTurnManagerComponent* TurnManager = nullptr;
        UGridMonsterPatrolSubsystem* Patrol = nullptr;

        bool Initialize (FIntPoint PartyCell = FIntPoint (7, 7))
        {
            if (!TestWorld.World)
            {
                return false;
            }

            Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
            Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
            if (!Runtime || !Party || !Party->PartyInventoryComponent)
            {
                return false;
            }

            Runtime->bApplyLevelStartOnBeginPlay = false;
            Level = NewObject<UGridLevelAsset> (Runtime);
            Level->Width = 8;
            Level->Height = 8;
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
            Runtime->LevelAsset = Level;

            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = PartyCell.X;
            Party->CurrentCellY = PartyCell.Y;
            Party->SetActorLocation (
                Runtime->GetCellCenterWorld (PartyCell.X, PartyCell.Y));

            FGridCharacterInventoryState Character;
            Character.CharacterId = FGuid::NewGuid ();
            Character.DisplayName = FText::FromString (TEXT ("MON14.3 Party"));
            Character.DerivedStats.MaxHealth = 20;
            Character.DerivedStats.CurrentHealth = 20;
            Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters =
                { Character };

            TurnManager = NewObject<UGridTurnManagerComponent> (
                Runtime,
                TEXT ("MON143TurnManager"));
            TurnManager->bAutoInitialize = false;
            Runtime->AddInstanceComponent (TurnManager);
            TurnManager->RegisterComponent ();
            if (!TurnManager->InitializeTurnManager (Runtime, Party))
            {
                return false;
            }

            Patrol = TestWorld.World->GetSubsystem<UGridMonsterPatrolSubsystem> ();
            if (!Patrol)
            {
                return false;
            }
            Patrol->RegisterRuntime (Runtime);
            return true;
        }

        UGridMonsterDefinitionAsset* MakeDefinition (
            FName MonsterId,
            int32 SightRange = 6,
            int32 HearingRange = 0)
        {
            UGridMonsterDefinitionAsset* Definition =
                NewObject<UGridMonsterDefinitionAsset> (Runtime);
            Definition->MonsterId = MonsterId;
            Definition->DisplayName = FText::FromName (MonsterId);
            Definition->CategoryId = TEXT ("MON14_3_Test");
            Definition->MaxHealth = 10;
            Definition->ActionPointsPerTurn = 2;
            Definition->SightRangeCells = SightRange;
            Definition->HearingRangeCells = HearingRange;
            Definition->GridFootprint = FIntPoint (1, 1);
            Definition->MoveDuration = 1.0f;
            Definition->TurnDuration = 1.0f;
            Definition->DeathExpectedDuration = 1.0f;
            return Definition;
        }

        AGridMonsterActor* AddMonster (
            UGridMonsterDefinitionAsset* Definition,
            FIntPoint Cell,
            EGridEdge Facing,
            EGridMonsterState InitialState,
            EGridMonsterPatrolMode PatrolMode,
            const TArray<FGridMonsterPatrolWaypoint>& Waypoints,
            FGuid SpawnId = FGuid ())
        {
            if (!Definition || !Runtime || !Level || !TestWorld.World)
            {
                return nullptr;
            }

            const FGuid StableId = SpawnId.IsValid ()
                ? SpawnId
                : FGuid::NewGuid ();
            FGridLevelObjectData Spawn;
            Spawn.ObjectId = StableId;
            Spawn.Type = EGridLevelObjectType::MonsterSpawn;
            Spawn.CellX = Cell.X;
            Spawn.CellY = Cell.Y;
            Spawn.Edge = EGridEdge::None;
            Spawn.InitialFacing = Facing;
            Spawn.InitialMonsterState = InitialState;
            Spawn.PatrolMode = PatrolMode;
            Spawn.PatrolWaypoints = Waypoints;
            Spawn.MonsterDefinitionAsset = Definition;
            Spawn.MonsterDefinitionId = Definition->MonsterId;
            Spawn.bInitiallyEnabled = true;
            Level->Objects.Add (Spawn);

            FActorSpawnParameters Params;
            Params.Owner = Runtime;
            AGridMonsterActor* Monster =
                TestWorld.World->SpawnActor<AGridMonsterActor> (
                    AGridMonsterActor::StaticClass (),
                    Runtime->GetCellCenterWorld (Cell.X, Cell.Y),
                    FRotator::ZeroRotator,
                    Params);
            if (!Monster ||
                !Monster->InitializeMonster (
                    Definition,
                    StableId,
                    Cell,
                    Facing))
            {
                return nullptr;
            }

            UGridMonsterMovementComponent* Movement =
                NewObject<UGridMonsterMovementComponent> (
                    Monster,
                    TEXT ("MON143Movement"));
            Movement->bAutoInitialize = false;
            Movement->bInferCellFromActorLocation = false;
            Monster->AddInstanceComponent (Movement);
            Movement->RegisterComponent ();

            UGridMonsterBehaviorComponent* Behavior =
                NewObject<UGridMonsterBehaviorComponent> (
                    Monster,
                    TEXT ("MON143Behavior"));
            Behavior->bAutoInitialize = false;
            Monster->AddInstanceComponent (Behavior);
            Behavior->RegisterComponent ();

            if (!Movement->InitializeMovement (Runtime) ||
                !Behavior->InitializeBehavior (Runtime, Party))
            {
                return nullptr;
            }
            return Monster;
        }
    };

    TArray<FGridMonsterPatrolWaypoint> MON143MakeHorizontalRoute ()
    {
        FGridMonsterPatrolWaypoint First;
        First.Cell = FIntPoint (1, 1);
        First.Facing = EGridEdge::East;
        First.WaitSeconds = 0.0f;

        FGridMonsterPatrolWaypoint Second;
        Second.Cell = FIntPoint (4, 1);
        Second.Facing = EGridEdge::West;
        Second.WaitSeconds = 0.0f;
        return { First, Second };
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON143CursorRulesTest,
    "Grimrock.Monsters.MON14.3.CursorRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON143CursorRulesTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    TArray<FGridMonsterPatrolWaypoint> Route = MON143MakeHorizontalRoute ();
    FGridMonsterPatrolWaypoint Third;
    Third.Cell = FIntPoint (6, 1);
    Route.Add (Third);

    int32 Target = INDEX_NONE;
    int32 Direction = 0;
    TestTrue (TEXT ("Loop cursor initializes on exact waypoint"),
        FGridMonsterPatrolCursorRules::Initialize (
            FIntPoint (1, 1), EGridMonsterPatrolMode::Loop,
            Route, Target, Direction));
    TestEqual (TEXT ("Exact first waypoint selected"), Target, 0);

    int32 Next = INDEX_NONE;
    TestTrue (TEXT ("Loop advances"),
        FGridMonsterPatrolCursorRules::Advance (
            EGridMonsterPatrolMode::Loop, 3, 2, Direction, Next));
    TestEqual (TEXT ("Loop wraps last to first"), Next, 0);

    Direction = 1;
    TestTrue (TEXT ("PingPong reverses at last"),
        FGridMonsterPatrolCursorRules::Advance (
            EGridMonsterPatrolMode::PingPong, 3, 2, Direction, Next));
    TestEqual (TEXT ("PingPong last goes backward"), Next, 1);
    TestEqual (TEXT ("PingPong direction becomes backward"), Direction, -1);

    TestTrue (TEXT ("PingPong reverses at first"),
        FGridMonsterPatrolCursorRules::Advance (
            EGridMonsterPatrolMode::PingPong, 3, 0, Direction, Next));
    TestEqual (TEXT ("PingPong first goes forward"), Next, 1);
    TestEqual (TEXT ("PingPong direction becomes forward"), Direction, 1);

    Target = INDEX_NONE;
    TestTrue (TEXT ("Rejoin cursor initializes from off-route cell"),
        FGridMonsterPatrolCursorRules::Initialize (
            FIntPoint (5, 2), EGridMonsterPatrolMode::Loop,
            Route, Target, Direction));
    TestEqual (TEXT ("Nearest waypoint chosen deterministically"), Target, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON143PatrolMovementTest,
    "Grimrock.Monsters.MON14.3.PatrolMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON143PatrolMovementTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON143Fixture Fixture;
    TestTrue (TEXT ("Fixture initializes"), Fixture.Initialize ());
    if (!Fixture.Patrol)
    {
        return false;
    }

    AGridMonsterActor* Monster = Fixture.AddMonster (
        Fixture.MakeDefinition (TEXT ("MON14_3_PatrolRat")),
        FIntPoint (2, 1),
        EGridEdge::West,
        EGridMonsterState::Idle,
        EGridMonsterPatrolMode::Loop,
        MON143MakeHorizontalRoute ());
    TestNotNull (TEXT ("Patrol monster exists"), Monster);
    if (!Monster)
    {
        return false;
    }

    UGridMonsterMovementComponent* Movement =
        Monster->FindComponentByClass<UGridMonsterMovementComponent> ();
    TestNotNull (TEXT ("Movement exists"), Movement);
    TestTrue (TEXT ("Off-route patrol starts rejoin motion"),
        Fixture.Patrol->ProcessMonsterNow (Monster, TEXT ("MON143Patrol")));
    TestEqual (TEXT ("Nearest waypoint becomes target 0"),
        Fixture.Patrol->GetMonsterTargetWaypointIndex (
            Monster->ResolvePersistenceId ()),
        0);
    TestTrue (TEXT ("Monster movement is active"), Movement && Movement->IsBusy ());
    TestEqual (TEXT ("Activity is Patrolling"),
        Fixture.Patrol->GetMonsterActivity (Monster->ResolvePersistenceId ()),
        EGridMonsterExplorationActivity::Patrolling);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON143HearingInvestigationTest,
    "Grimrock.Monsters.MON14.3.HearingInvestigation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON143HearingInvestigationTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON143Fixture Fixture;
    TestTrue (TEXT ("Fixture initializes"),
        Fixture.Initialize (FIntPoint (1, 4)));
    if (!Fixture.Patrol)
    {
        return false;
    }

    AGridMonsterActor* Monster = Fixture.AddMonster (
        Fixture.MakeDefinition (TEXT ("MON14_3_HearingRat"), 6, 4),
        FIntPoint (1, 1),
        EGridEdge::East,
        EGridMonsterState::Dormant,
        EGridMonsterPatrolMode::None,
        TArray<FGridMonsterPatrolWaypoint> ());
    TestNotNull (TEXT ("Investigating monster exists"), Monster);
    if (!Monster)
    {
        return false;
    }

    TestTrue (TEXT ("Hearing step is processed"),
        Fixture.Patrol->ProcessMonsterNow (Monster, TEXT ("MON143Hearing")));
    UGridMonsterBehaviorComponent* Behavior =
        Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ();
    TestTrue (TEXT ("Party is heard"), Behavior && Behavior->bCanHearParty);
    TestFalse (TEXT ("Party is not seen sideways"), Behavior && Behavior->bCanSeeParty);
    TestTrue (TEXT ("Last known party cell is stored"),
        Behavior && Behavior->bHasLastKnownPartyCell);
    TestEqual (TEXT ("Dormant monster wakes to Alert"),
        Monster->MonsterState,
        EGridMonsterState::Alert);
    TestEqual (TEXT ("Activity becomes Investigating"),
        Fixture.Patrol->GetMonsterActivity (Monster->ResolvePersistenceId ()),
        EGridMonsterExplorationActivity::Investigating);
    TestFalse (TEXT ("Hearing alone does not start combat"),
        Fixture.TurnManager->bCombatActive);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON143DormantPatrolTest,
    "Grimrock.Monsters.MON14.3.DormantPatrol",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON143DormantPatrolTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON143Fixture Fixture;
    TestTrue (TEXT ("Fixture initializes"), Fixture.Initialize ());
    AGridMonsterActor* Monster = Fixture.AddMonster (
        Fixture.MakeDefinition (TEXT ("MON14_3_DormantRat")),
        FIntPoint (1, 1),
        EGridEdge::East,
        EGridMonsterState::Dormant,
        EGridMonsterPatrolMode::Loop,
        MON143MakeHorizontalRoute ());
    TestNotNull (TEXT ("Dormant patrol monster exists"), Monster);
    if (!Monster || !Fixture.Patrol)
    {
        return false;
    }

    UGridMonsterMovementComponent* Movement =
        Monster->FindComponentByClass<UGridMonsterMovementComponent> ();
    TestTrue (TEXT ("Dormant state is processed"),
        Fixture.Patrol->ProcessMonsterNow (Monster));
    TestFalse (TEXT ("Dormant monster does not patrol"),
        Movement && Movement->IsBusy ());
    TestEqual (TEXT ("Dormant activity remains inactive"),
        Fixture.Patrol->GetMonsterActivity (Monster->ResolvePersistenceId ()),
        EGridMonsterExplorationActivity::Inactive);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON143CombatSuspensionTest,
    "Grimrock.Monsters.MON14.3.CombatSuspension",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON143CombatSuspensionTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON143Fixture Fixture;
    TestTrue (TEXT ("Fixture initializes"), Fixture.Initialize ());
    AGridMonsterActor* Monster = Fixture.AddMonster (
        Fixture.MakeDefinition (TEXT ("MON14_3_SuspendRat")),
        FIntPoint (2, 1),
        EGridEdge::West,
        EGridMonsterState::Idle,
        EGridMonsterPatrolMode::Loop,
        MON143MakeHorizontalRoute ());
    if (!Monster || !Fixture.Patrol)
    {
        return false;
    }

    UGridMonsterMovementComponent* Movement =
        Monster->FindComponentByClass<UGridMonsterMovementComponent> ();
    Fixture.Patrol->ProcessMonsterNow (Monster);
    TestTrue (TEXT ("Patrol move is active before suspension"),
        Movement && Movement->IsBusy ());

    Fixture.Patrol->SuspendAllForCombat ();
    TestFalse (TEXT ("Exploration motion is cancelled atomically"),
        Movement && Movement->IsBusy ());
    TestEqual (TEXT ("Activity is suspended"),
        Fixture.Patrol->GetMonsterActivity (Monster->ResolvePersistenceId ()),
        EGridMonsterExplorationActivity::Suspended);
    TestTrue (TEXT ("Cancelled move keeps authoritative start cell"),
        Monster->CurrentCell == FIntPoint (2, 1));
    return true;
}

#endif
