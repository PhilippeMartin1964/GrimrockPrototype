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
    struct FGridMON1751TestWorld
    {
        UWorld* World = nullptr;

        FGridMON1751TestWorld ()
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
                    TEXT ("MON1751TestWorld_%s"),
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

        ~FGridMON1751TestWorld ()
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

    struct FGridMON1751Fixture
    {
        FGridMON1751TestWorld TestWorld;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* Level = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
        UGridTurnManagerComponent* TurnManager = nullptr;
        UGridMonsterPatrolSubsystem* Patrol = nullptr;

        bool Initialize (FIntPoint PartyCell = FIntPoint (8, 8))
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
            Level->Width = 10;
            Level->Height = 10;
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
            Character.DisplayName = FText::FromString (TEXT ("MON17.5.1 Party"));
            Character.DerivedStats.MaxHealth = 20;
            Character.DerivedStats.CurrentHealth = 20;
            Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters =
                { Character };

            TurnManager = NewObject<UGridTurnManagerComponent> (
                Runtime,
                TEXT ("MON1751TurnManager"));
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

        UGridMonsterDefinitionAsset* MakeGoblinDefinition (
            int32 SightRange,
            int32 HearingRange,
            bool bSharesAggro,
            int32 AggroRange)
        {
            UGridMonsterDefinitionAsset* Definition =
                NewObject<UGridMonsterDefinitionAsset> (Runtime);
            Definition->MonsterId = TEXT ("MON_GoblinThrower");
            Definition->DisplayName = FText::FromString (TEXT ("Gobelin lanceur"));
            Definition->CategoryId = TEXT ("Goblin");
            Definition->DangerLevel = 3;
            Definition->MaxHealth = 10;
            Definition->Initiative = 12;
            Definition->Accuracy = 2;
            Definition->Evasion = 3;
            Definition->ActionPointsPerTurn = 3;
            Definition->SightRangeCells = SightRange;
            Definition->HearingRangeCells = HearingRange;
            Definition->AggroPropagationRange = AggroRange;
            Definition->bSharesAggroWithGroup = bSharesAggro;
            Definition->PrimaryAIProfile = EGridMonsterAIProfile::RangedKeeper;
            Definition->PreferredMinDistance = 3;
            Definition->PreferredMaxDistance = 5;
            Definition->GridFootprint = FIntPoint (1, 1);
            Definition->MoveDuration = 1.0f;
            Definition->TurnDuration = 1.0f;
            Definition->DeathExpectedDuration = 1.0f;
            return Definition;
        }

        AGridMonsterActor* AddGoblin (
            UGridMonsterDefinitionAsset* Definition,
            FIntPoint Cell,
            EGridEdge Facing,
            EGridMonsterState InitialState,
            FName EncounterGroupId = NAME_None,
            EGridMonsterPatrolMode PatrolMode = EGridMonsterPatrolMode::None,
            const TArray<FGridMonsterPatrolWaypoint>& Waypoints = {})
        {
            if (!Definition || !Runtime || !Level || !TestWorld.World)
            {
                return nullptr;
            }

            const FGuid StableId = FGuid::NewGuid ();
            FGridLevelObjectData Spawn;
            Spawn.ObjectId = StableId;
            Spawn.Type = EGridLevelObjectType::MonsterSpawn;
            Spawn.CellX = Cell.X;
            Spawn.CellY = Cell.Y;
            Spawn.Edge = EGridEdge::None;
            Spawn.InitialFacing = Facing;
            Spawn.InitialMonsterState = InitialState;
            Spawn.MonsterDefinitionAsset = Definition;
            Spawn.MonsterDefinitionId = Definition->MonsterId;
            Spawn.EncounterGroupId = EncounterGroupId;
            Spawn.PatrolMode = PatrolMode;
            Spawn.PatrolWaypoints = Waypoints;
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
                    Facing,
                    EncounterGroupId))
            {
                return nullptr;
            }

            UGridMonsterMovementComponent* Movement =
                NewObject<UGridMonsterMovementComponent> (
                    Monster,
                    TEXT ("MON1751Movement"));
            Movement->bAutoInitialize = false;
            Movement->bInferCellFromActorLocation = false;
            Monster->AddInstanceComponent (Movement);
            Movement->RegisterComponent ();

            UGridMonsterBehaviorComponent* Behavior =
                NewObject<UGridMonsterBehaviorComponent> (
                    Monster,
                    TEXT ("MON1751Behavior"));
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

    TArray<FGridMonsterPatrolWaypoint> MakeGoblinPatrolRoute ()
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
    FGridMonsterMON1751PatrolRangedKeeperTest,
    "Grimrock.Monsters.MON17.5.1.PatrolRangedKeeper",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1751PatrolRangedKeeperTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1751Fixture Fixture;
    TestTrue (TEXT ("Fixture initializes"), Fixture.Initialize ());
    if (!Fixture.Patrol)
    {
        return false;
    }

    UGridMonsterDefinitionAsset* Definition =
        Fixture.MakeGoblinDefinition (0, 0, false, 0);
    AGridMonsterActor* Goblin = Fixture.AddGoblin (
        Definition,
        FIntPoint (2, 1),
        EGridEdge::West,
        EGridMonsterState::Idle,
        NAME_None,
        EGridMonsterPatrolMode::Loop,
        MakeGoblinPatrolRoute ());

    TestNotNull (TEXT ("RangedKeeper Goblin exists"), Goblin);
    if (!Goblin)
    {
        return false;
    }

    UGridMonsterMovementComponent* Movement =
        Goblin->FindComponentByClass<UGridMonsterMovementComponent> ();
    TestTrue (TEXT ("Definition keeps RangedKeeper profile"),
        Definition->HasAIProfile (EGridMonsterAIProfile::RangedKeeper));
    TestTrue (TEXT ("RangedKeeper patrol processing succeeds"),
        Fixture.Patrol->ProcessMonsterNow (
            Goblin,
            TEXT ("MON1751Patrol")));
    TestTrue (TEXT ("RangedKeeper starts grid patrol movement"),
        Movement && Movement->IsBusy ());
    TestEqual (TEXT ("RangedKeeper patrol activity is Patrolling"),
        Fixture.Patrol->GetMonsterActivity (Goblin->ResolvePersistenceId ()),
        EGridMonsterExplorationActivity::Patrolling);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1751DirectionalPerceptionTest,
    "Grimrock.Monsters.MON17.5.1.DirectionalPerception",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1751DirectionalPerceptionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1751Fixture Fixture;
    TestTrue (TEXT ("Fixture initializes"),
        Fixture.Initialize (FIntPoint (1, 4)));

    UGridMonsterDefinitionAsset* Definition =
        Fixture.MakeGoblinDefinition (8, 4, false, 0);
    AGridMonsterActor* Goblin = Fixture.AddGoblin (
        Definition,
        FIntPoint (1, 1),
        EGridEdge::North,
        EGridMonsterState::Idle);
    TestNotNull (TEXT ("RangedKeeper Goblin exists"), Goblin);
    if (!Goblin)
    {
        return false;
    }

    UGridMonsterBehaviorComponent* Behavior =
        Goblin->FindComponentByClass<UGridMonsterBehaviorComponent> ();
    TestNotNull (TEXT ("Behavior exists"), Behavior);
    if (!Behavior)
    {
        return false;
    }

    TestTrue (TEXT ("North-facing Goblin perceives party"),
        Behavior->RefreshPerception ());
    TestTrue (TEXT ("North-facing Goblin sees party"),
        Behavior->bCanSeeParty);
    TestTrue (TEXT ("Party is also inside hearing range"),
        Behavior->bCanHearParty);

    Goblin->Facing = EGridEdge::East;
    TestTrue (TEXT ("East-facing Goblin still perceives by hearing"),
        Behavior->RefreshPerception ());
    TestFalse (TEXT ("East-facing Goblin no longer sees party"),
        Behavior->bCanSeeParty);
    TestTrue (TEXT ("Hearing remains omnidirectional"),
        Behavior->bCanHearParty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1751HearingAlarmTest,
    "Grimrock.Monsters.MON17.5.1.HearingAlarm",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1751HearingAlarmTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1751Fixture Fixture;
    TestTrue (TEXT ("Fixture initializes"),
        Fixture.Initialize (FIntPoint (1, 4)));
    if (!Fixture.Patrol || !Fixture.TurnManager)
    {
        return false;
    }

    UGridMonsterDefinitionAsset* SourceDefinition =
        Fixture.MakeGoblinDefinition (8, 4, true, 2);
    UGridMonsterDefinitionAsset* AllyDefinition =
        Fixture.MakeGoblinDefinition (8, 0, false, 0);

    AGridMonsterActor* Source = Fixture.AddGoblin (
        SourceDefinition,
        FIntPoint (1, 1),
        EGridEdge::East,
        EGridMonsterState::Idle,
        TEXT ("GoblinAlarm_A"));
    AGridMonsterActor* Ally = Fixture.AddGoblin (
        AllyDefinition,
        FIntPoint (2, 1),
        EGridEdge::West,
        EGridMonsterState::Dormant,
        TEXT ("GoblinAlarm_A"));

    TestNotNull (TEXT ("Alarm source exists"), Source);
    TestNotNull (TEXT ("Alarm ally exists"), Ally);
    if (!Source || !Ally)
    {
        return false;
    }

    TestTrue (TEXT ("Hearing source is processed"),
        Fixture.Patrol->ProcessMonsterNow (
            Source,
            TEXT ("MON1751HearingAlarm")));

    UGridMonsterBehaviorComponent* SourceBehavior =
        Source->FindComponentByClass<UGridMonsterBehaviorComponent> ();
    UGridMonsterBehaviorComponent* AllyBehavior =
        Ally->FindComponentByClass<UGridMonsterBehaviorComponent> ();

    TestFalse (TEXT ("Source does not see party while facing away"),
        SourceBehavior && SourceBehavior->bCanSeeParty);
    TestTrue (TEXT ("Source hears party"),
        SourceBehavior && SourceBehavior->bCanHearParty);
    TestEqual (TEXT ("Dormant RangedKeeper ally wakes to Alert"),
        Ally->MonsterState,
        EGridMonsterState::Alert);
    TestTrue (TEXT ("Alarm ally receives last known party cell"),
        AllyBehavior && AllyBehavior->bHasLastKnownPartyCell);
    TestTrue (TEXT ("Alarm ally receives correct party cell"),
        AllyBehavior && AllyBehavior->LastKnownPartyCell == FIntPoint (1, 4));
    TestEqual (TEXT ("Alarm ally starts investigation"),
        Fixture.Patrol->GetMonsterActivity (Ally->ResolvePersistenceId ()),
        EGridMonsterExplorationActivity::Investigating);
    TestFalse (TEXT ("Hearing/alarm alone does not start combat"),
        Fixture.TurnManager->bCombatActive);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1751VisionEngagementHandoffTest,
    "Grimrock.Monsters.MON17.5.1.VisionEngagementHandoff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1751VisionEngagementHandoffTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1751Fixture Fixture;
    TestTrue (TEXT ("Fixture initializes"),
        Fixture.Initialize (FIntPoint (1, 4)));
    if (!Fixture.Patrol)
    {
        return false;
    }

    UGridMonsterDefinitionAsset* Definition =
        Fixture.MakeGoblinDefinition (8, 4, false, 0);
    AGridMonsterActor* Goblin = Fixture.AddGoblin (
        Definition,
        FIntPoint (1, 1),
        EGridEdge::North,
        EGridMonsterState::Idle,
        NAME_None,
        EGridMonsterPatrolMode::Loop,
        MakeGoblinPatrolRoute ());
    TestNotNull (TEXT ("Vision Goblin exists"), Goblin);
    if (!Goblin)
    {
        return false;
    }

    TestTrue (TEXT ("Visual RangedKeeper processing succeeds"),
        Fixture.Patrol->ProcessMonsterNow (
            Goblin,
            TEXT ("MON1751Vision")));

    UGridMonsterBehaviorComponent* Behavior =
        Goblin->FindComponentByClass<UGridMonsterBehaviorComponent> ();
    TestTrue (TEXT ("Visual source sees party"),
        Behavior && Behavior->bCanSeeParty);
    TestEqual (TEXT ("Visual source enters Engaging exploration activity"),
        Fixture.Patrol->GetMonsterActivity (Goblin->ResolvePersistenceId ()),
        EGridMonsterExplorationActivity::Engaging);
    TestEqual (TEXT ("Visual source state becomes Alert"),
        Goblin->MonsterState,
        EGridMonsterState::Alert);
    return true;
}

#endif
