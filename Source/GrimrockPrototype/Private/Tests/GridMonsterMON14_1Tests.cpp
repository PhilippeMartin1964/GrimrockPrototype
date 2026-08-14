#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimInstance.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridMonsterEncounterComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridAutomaticPerceptionEngagementSubsystem.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"

namespace
{
    struct FGridMON141TestWorld
    {
        UWorld* World = nullptr;

        FGridMON141TestWorld ()
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
                    TEXT ("MON141TestWorld_%s"),
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

        ~FGridMON141TestWorld ()
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

    struct FGridMON141Fixture
    {
        FGridMON141TestWorld TestWorld;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* Level = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
        UGridTurnManagerComponent* TurnManager = nullptr;
        UGridAutomaticPerceptionEngagementSubsystem* Engagement = nullptr;

        bool Initialize (FIntPoint PartyCell = FIntPoint (1, 1))
        {
            UWorld* World = TestWorld.World;
            if (!World)
            {
                return false;
            }

            Runtime = World->SpawnActor<AGridLevelRuntimeActor> ();
            Party = World->SpawnActor<AGrimrockPartyPawn> ();
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
            Character.DisplayName = FText::FromString (TEXT ("MON14 Party"));
            Character.DerivedStats.MaxHealth = 20;
            Character.DerivedStats.CurrentHealth = 20;
            Character.DerivedStats.PhysicalArmor = 0;
            Character.DerivedStats.MagicalArmor = 0;
            Character.DerivedStats.Evasion = 0;
            Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };

            TurnManager = NewObject<UGridTurnManagerComponent> (
                Runtime,
                TEXT ("MON141TurnManager"));
            TurnManager->bAutoInitialize = false;
            Runtime->AddInstanceComponent (TurnManager);
            TurnManager->RegisterComponent ();
            if (!TurnManager->InitializeTurnManager (Runtime, Party))
            {
                return false;
            }

            if (UGridDoorSystemComponent* Doors =
                Runtime->FindComponentByClass<UGridDoorSystemComponent> ())
            {
                Doors->Initialize (Runtime);
                Doors->RebuildIndexes ();
            }
            if (UGridMonsterEncounterComponent* Encounters =
                Runtime->FindComponentByClass<UGridMonsterEncounterComponent> ())
            {
                Encounters->Initialize (Runtime);
            }

            Engagement = World->GetSubsystem<
                UGridAutomaticPerceptionEngagementSubsystem> ();
            return Engagement != nullptr;
        }

        UGridMonsterDefinitionAsset* MakeDefinition (
            FName MonsterId,
            int32 SightRange,
            int32 HearingRange,
            bool bSharesAggro = false,
            int32 AggroRange = 0)
        {
            UGridMonsterDefinitionAsset* Definition =
                NewObject<UGridMonsterDefinitionAsset> (Runtime);
            Definition->MonsterId = MonsterId;
            Definition->DisplayName = FText::FromName (MonsterId);
            Definition->CategoryId = TEXT ("MON14_Test");
            Definition->MaxHealth = 10;
            Definition->ActionPointsPerTurn = 2;
            Definition->SightRangeCells = SightRange;
            Definition->HearingRangeCells = HearingRange;
            Definition->bSharesAggroWithGroup = bSharesAggro;
            Definition->AggroPropagationRange = AggroRange;
            Definition->DeathExpectedDuration = 1.0f;
            return Definition;
        }

        AGridMonsterActor* AddMonster (
            UGridMonsterDefinitionAsset* Definition,
            FIntPoint Cell,
            FName GroupId = NAME_None,
            FGuid Id = FGuid ())
        {
            if (!TestWorld.World || !Runtime || !Definition)
            {
                return nullptr;
            }

            FActorSpawnParameters Params;
            Params.Owner = Runtime;
            AGridMonsterActor* Monster =
                TestWorld.World->SpawnActor<AGridMonsterActor> (
                    AGridMonsterActor::StaticClass (),
                    Runtime->GetCellCenterWorld (Cell.X, Cell.Y),
                    FRotator::ZeroRotator,
                    Params);
            if (!Monster)
            {
                return nullptr;
            }

            const FGuid StableId = Id.IsValid () ? Id : FGuid::NewGuid ();
            if (!Monster->InitializeMonster (
                    Definition,
                    StableId,
                    Cell,
                    EGridEdge::North,
                    GroupId))
            {
                return nullptr;
            }

            UGridMonsterMovementComponent* Movement =
                NewObject<UGridMonsterMovementComponent> (
                    Monster,
                    TEXT ("MON141Movement"));
            Movement->bAutoInitialize = false;
            Movement->bInferCellFromActorLocation = false;
            Monster->AddInstanceComponent (Movement);
            Movement->RegisterComponent ();

            UGridMonsterBehaviorComponent* Behavior =
                NewObject<UGridMonsterBehaviorComponent> (
                    Monster,
                    TEXT ("MON141Behavior"));
            Behavior->bAutoInitialize = false;
            Monster->AddInstanceComponent (Behavior);
            Behavior->RegisterComponent ();

            if (!Movement->InitializeMovement (Runtime) ||
                !Behavior->InitializeBehavior (Runtime, Party) ||
                !Monster->CombatComponent ||
                !Monster->CombatComponent->InitializeCombat (Party))
            {
                return nullptr;
            }
            return Monster;
        }

        bool Evaluate (FName Reason = TEXT ("MON141Test"))
        {
            if (!Engagement)
            {
                return false;
            }
            Engagement->RequestEvaluation (Runtime, Reason);
            return Engagement->ProcessPendingEvaluationNow ();
        }
    };

    int32 CountParticipant (
        const UGridTurnManagerComponent* TurnManager,
        const AGridMonsterActor* Monster)
    {
        if (!TurnManager || !Monster)
        {
            return 0;
        }
        int32 Count = 0;
        for (const TObjectPtr<AGridMonsterActor>& Candidate :
            TurnManager->CombatMonsters)
        {
            Count += Candidate.Get () == Monster ? 1 : 0;
        }
        return Count;
    }

    UGridMonsterDefinitionAsset* MakeAssetBackedDefinition (
        FAutomationTestBase& Test,
        UObject* Outer,
        FName MonsterId,
        int32 SightRange,
        int32 HearingRange)
    {
        USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh> (
            nullptr,
            TEXT ("/Game/GrimrockPrototype/Monsters/RatGiant/Meshes/SK_RatGiant.SK_RatGiant"));
        UClass* AnimationClass = LoadClass<UAnimInstance> (
            nullptr,
            TEXT ("/Game/GrimrockPrototype/Monsters/RatGiant/Animation/ABP_MON_RatGiant.ABP_MON_RatGiant_C"));
        UClass* MonsterActorClass = LoadClass<AGridMonsterActor> (
            nullptr,
            TEXT ("/Game/GrimrockPrototype/Monsters/RatGiant/Blueprints/BP_MON_RatGiant.BP_MON_RatGiant_C"));

        Test.TestNotNull (TEXT ("Rat Giant skeletal mesh loads"), SkeletalMesh);
        Test.TestNotNull (TEXT ("Rat Giant animation class loads"), AnimationClass);
        Test.TestNotNull (TEXT ("Rat Giant actor class loads"), MonsterActorClass);
        if (!SkeletalMesh || !AnimationClass || !MonsterActorClass)
        {
            return nullptr;
        }

        UGridMonsterDefinitionAsset* Definition =
            NewObject<UGridMonsterDefinitionAsset> (Outer);
        Definition->MonsterId = MonsterId;
        Definition->DisplayName = FText::FromString (TEXT ("Rat géant MON14.1"));
        Definition->CategoryId = TEXT ("Vermin");
        Definition->DangerLevel = 1;
        Definition->MaxHealth = 12;
        Definition->ActionPointsPerTurn = 2;
        Definition->SightRangeCells = SightRange;
        Definition->HearingRangeCells = HearingRange;
        Definition->GridFootprint = FIntPoint (1, 1);
        Definition->DeathExpectedDuration = 1.0f;
        Definition->SkeletalMesh = TSoftObjectPtr<USkeletalMesh> (SkeletalMesh);
        Definition->AnimationClass = AnimationClass;
        Definition->MonsterActorClass = MonsterActorClass;
        return Definition;
    }

    FGridLevelObjectData MakeEncounterSpawn (
        UGridMonsterDefinitionAsset* Definition,
        FGuid SpawnId,
        FIntPoint Cell,
        FName GroupId,
        int32 WaveIndex)
    {
        FGridLevelObjectData Spawn;
        Spawn.ObjectId = SpawnId;
        Spawn.Type = EGridLevelObjectType::MonsterSpawn;
        Spawn.CellX = Cell.X;
        Spawn.CellY = Cell.Y;
        Spawn.Edge = EGridEdge::None;
        Spawn.InitialFacing = EGridEdge::North;
        Spawn.MonsterDefinitionAsset = Definition;
        Spawn.MonsterDefinitionId = Definition ? Definition->MonsterId : NAME_None;
        Spawn.EncounterGroupId = GroupId;
        Spawn.EncounterWaveIndex = WaveIndex;
        Spawn.bInitiallyEnabled = true;
        return Spawn;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON141DirectSightTest,
    "Grimrock.Monsters.MON14.1.DirectSight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON141DirectSightTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON141Fixture Fixture;
    if (!Fixture.Initialize ())
    {
        return false;
    }
    UGridMonsterDefinitionAsset* Definition =
        Fixture.MakeDefinition (TEXT ("MON14_Direct"), 5, 0);
    AGridMonsterActor* Monster =
        Fixture.AddMonster (Definition, FIntPoint (1, 4));
    TestNotNull (TEXT ("Direct-sight monster exists"), Monster);
    TestTrue (TEXT ("Direct sight starts combat automatically"), Fixture.Evaluate ());
    TestTrue (TEXT ("Combat is active"), Fixture.TurnManager->bCombatActive);
    TestEqual (TEXT ("Only the direct monster participates"),
        Fixture.TurnManager->CombatMonsters.Num (), 1);
    TestEqual (TEXT ("Direct monster is deduplicated"),
        CountParticipant (Fixture.TurnManager, Monster), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON141RangeWallDoorTest,
    "Grimrock.Monsters.MON14.1.RangeWallDoor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON141RangeWallDoorTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    {
        FGridMON141Fixture Fixture;
        if (!Fixture.Initialize ()) return false;
        AGridMonsterActor* Monster = Fixture.AddMonster (
            Fixture.MakeDefinition (TEXT ("MON14_Range"), 3, 0),
            FIntPoint (1, 7));
        TestNotNull (TEXT ("Out-of-range monster exists"), Monster);
        TestFalse (TEXT ("Out of sight range does not engage"), Fixture.Evaluate ());
        TestFalse (TEXT ("Combat remains inactive out of range"), Fixture.TurnManager->bCombatActive);
    }

    {
        FGridMON141Fixture Fixture;
        if (!Fixture.Initialize ()) return false;
        Fixture.Level->Cells[Fixture.Level->GetIndex (1, 3)].SouthWall = EGridWallType::Solid;
        AGridMonsterActor* Monster = Fixture.AddMonster (
            Fixture.MakeDefinition (TEXT ("MON14_Wall"), 6, 0),
            FIntPoint (1, 4));
        TestNotNull (TEXT ("Wall-test monster exists"), Monster);
        TestFalse (TEXT ("A solid wall blocks automatic engagement"), Fixture.Evaluate ());
    }

    {
        FGridMON141Fixture Fixture;
        if (!Fixture.Initialize ()) return false;
        UGridDoorSystemComponent* Doors =
            Fixture.Runtime->FindComponentByClass<UGridDoorSystemComponent> ();
        TestNotNull (TEXT ("Door system exists"), Doors);
        if (!Doors) return false;
        Doors->SetDoorPassageBlocked (1, 3, EGridEdge::South, true);
        AGridMonsterActor* Monster = Fixture.AddMonster (
            Fixture.MakeDefinition (TEXT ("MON14_Door"), 6, 0),
            FIntPoint (1, 4));
        TestNotNull (TEXT ("Door-test monster exists"), Monster);
        TestFalse (TEXT ("A closed door blocks automatic engagement"), Fixture.Evaluate ());
        Doors->SetDoorPassageBlocked (1, 3, EGridEdge::South, false);
        TestTrue (TEXT ("Opening the logical door passage allows engagement"), Fixture.Evaluate (TEXT ("DoorOpened")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON141HearingDormantManualTest,
    "Grimrock.Monsters.MON14.1.HearingDormantManual",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON141HearingDormantManualTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    {
        FGridMON141Fixture Fixture;
        if (!Fixture.Initialize ()) return false;
        AGridMonsterActor* Monster = Fixture.AddMonster (
            Fixture.MakeDefinition (TEXT ("MON14_Hearing"), 8, 3),
            FIntPoint (2, 2));
        UGridMonsterBehaviorComponent* Behavior = Monster
            ? Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ()
            : nullptr;
        TestNotNull (TEXT ("Hearing behavior exists"), Behavior);
        TestFalse (TEXT ("Hearing alone does not auto-start combat"), Fixture.Evaluate ());
        TestFalse (TEXT ("Hearing-alone combat remains inactive"), Fixture.TurnManager->bCombatActive);
        if (Behavior && Monster)
        {
            TestFalse (TEXT ("Diagonal target is not seen"), Behavior->bCanSeeParty);
            TestTrue (TEXT ("Diagonal target is heard"), Behavior->bCanHearParty);
            TestTrue (TEXT ("Hearing stores the last known party cell"), Behavior->bHasLastKnownPartyCell);
            TestTrue (TEXT ("Last known cell matches the party"),
                Behavior->LastKnownPartyCell == FIntPoint (1, 1));
            TestEqual (TEXT ("Hearing changes Idle to Alert"), Monster->MonsterState, EGridMonsterState::Alert);
        }
        TestTrue (TEXT ("Historical manual perception start still accepts hearing"),
            Fixture.TurnManager->StartCombatFromPerception ());
    }

    {
        FGridMON141Fixture Fixture;
        if (!Fixture.Initialize ()) return false;
        AGridMonsterActor* Monster = Fixture.AddMonster (
            Fixture.MakeDefinition (TEXT ("MON14_Dormant"), 5, 0),
            FIntPoint (1, 4));
        TestNotNull (TEXT ("Dormant monster exists"), Monster);
        if (!Monster) return false;
        Monster->MonsterState = EGridMonsterState::Dormant;
        TestTrue (TEXT ("Dormant monster seeing the party engages"), Fixture.Evaluate ());
        TestEqual (TEXT ("Dormant monster wakes to Alert before combat behavior"),
            Monster->MonsterState, EGridMonsterState::Alert);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON141ExclusionTest,
    "Grimrock.Monsters.MON14.1.Exclusions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON141ExclusionTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON141Fixture Fixture;
    if (!Fixture.Initialize ()) return false;

    UGridMonsterDefinitionAsset* Definition =
        Fixture.MakeDefinition (TEXT ("MON14_Excluded"), 6, 0);
    AGridMonsterActor* Dead = Fixture.AddMonster (Definition, FIntPoint (1, 3));
    AGridMonsterActor* Disabled = Fixture.AddMonster (Definition, FIntPoint (1, 4));
    TestNotNull (TEXT ("Dead test monster exists"), Dead);
    TestNotNull (TEXT ("Disabled test monster exists"), Disabled);
    if (!Dead || !Disabled) return false;
    Dead->CurrentHealth = 0;
    Dead->MonsterState = EGridMonsterState::Dead;
    Disabled->bMonsterEnabled = false;

    const FGuid FutureSpawnId = FGuid::NewGuid ();
    FGridLevelObjectData FutureSpawn = MakeEncounterSpawn (
        Definition,
        FutureSpawnId,
        FIntPoint (1, 5),
        TEXT ("FutureWave"),
        2);
    Fixture.Level->Objects.Add (FutureSpawn);

    TestFalse (TEXT ("Dead, disabled and future-wave-only placements do not engage"), Fixture.Evaluate ());
    TestFalse (TEXT ("Future wave has no Actor before activation"),
        IsValid (Fixture.Runtime->FindSpawnedMonsterActor (FutureSpawnId)));
    TestTrue (TEXT ("No excluded participant is admitted"), Fixture.TurnManager->CombatMonsters.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON141CoalescingAggroTest,
    "Grimrock.Monsters.MON14.1.CoalescingAggro",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON141CoalescingAggroTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON141Fixture Fixture;
    if (!Fixture.Initialize ()) return false;

    UGridMonsterDefinitionAsset* Definition =
        Fixture.MakeDefinition (TEXT ("MON14_GroupRat"), 6, 0, true, 4);
    AGridMonsterActor* Source = Fixture.AddMonster (
        Definition,
        FIntPoint (1, 3),
        TEXT ("RoomA"),
        FGuid (14, 1, 1, 1));
    AGridMonsterActor* Propagated = Fixture.AddMonster (
        Definition,
        FIntPoint (4, 3),
        TEXT ("RoomA"),
        FGuid (14, 1, 1, 2));
    TestNotNull (TEXT ("Aggro source exists"), Source);
    TestNotNull (TEXT ("Aggro target exists"), Propagated);
    if (!Source || !Propagated) return false;

    const int32 InitialQueued = Fixture.Engagement->GetQueuedRequestCount ();
    Fixture.Engagement->RequestEvaluation (Fixture.Runtime, TEXT ("DuplicateA"));
    Fixture.Engagement->RequestEvaluation (Fixture.Runtime, TEXT ("DuplicateB"));
    Fixture.Engagement->RequestEvaluation (Fixture.Runtime, TEXT ("DuplicateC"));
    TestEqual (TEXT ("All notifications are counted"),
        Fixture.Engagement->GetQueuedRequestCount (), InitialQueued + 3);
    TestTrue (TEXT ("Duplicate notifications are coalesced into one pending evaluation"),
        Fixture.Engagement->HasPendingEvaluation ());
    TestTrue (TEXT ("The coalesced visual evaluation starts combat"),
        Fixture.Engagement->ProcessPendingEvaluationNow ());
    TestEqual (TEXT ("Only one effective evaluation ran"),
        Fixture.Engagement->GetEffectiveEvaluationCount (), 1);
    TestEqual (TEXT ("Only one automatic start succeeded"),
        Fixture.Engagement->GetSuccessfulStartCount (), 1);
    TestEqual (TEXT ("MON7 adds the propagated group member"),
        Fixture.TurnManager->CombatMonsters.Num (), 2);
    TestEqual (TEXT ("Direct source is unique"), CountParticipant (Fixture.TurnManager, Source), 1);
    TestEqual (TEXT ("Propagated participant is unique"), CountParticipant (Fixture.TurnManager, Propagated), 1);
    TestEqual (TEXT ("Propagated member becomes Alert"),
        Propagated->MonsterState, EGridMonsterState::Alert);
    TestFalse (TEXT ("No second pending evaluation means no second start"),
        Fixture.Engagement->ProcessPendingEvaluationNow ());
    TestEqual (TEXT ("Successful start count remains one"),
        Fixture.Engagement->GetSuccessfulStartCount (), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON141EncounterVisibilityTest,
    "Grimrock.Monsters.MON14.1.EncounterVisibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON141EncounterVisibilityTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    {
        FGridMON141Fixture Fixture;
        if (!Fixture.Initialize ()) return false;
        UGridMonsterDefinitionAsset* Definition = MakeAssetBackedDefinition (
            *this, Fixture.Runtime, TEXT ("MON14_EncounterHidden"), 6, 0);
        if (!Definition) return false;
        const FGuid SpawnId = FGuid (14, 2, 1, 1);
        Fixture.Level->Objects.Add (MakeEncounterSpawn (
            Definition, SpawnId, FIntPoint (3, 3), TEXT ("EncounterHidden"), 0));
        TestTrue (TEXT ("StartEncounter without LOS still succeeds as a spawn transaction"),
            Fixture.Runtime->StartMonsterEncounter (SpawnId));
        TestNotNull (TEXT ("Encounter member was spawned"),
            Fixture.Runtime->FindSpawnedMonsterActor (SpawnId));
        TestFalse (TEXT ("Encounter without visual LOS does not auto-start combat"),
            Fixture.Engagement->ProcessPendingEvaluationNow ());
        TestFalse (TEXT ("Combat stays inactive after hidden encounter spawn"),
            Fixture.TurnManager->bCombatActive);
    }

    {
        FGridMON141Fixture Fixture;
        if (!Fixture.Initialize ()) return false;
        UGridMonsterDefinitionAsset* Definition = MakeAssetBackedDefinition (
            *this, Fixture.Runtime, TEXT ("MON14_EncounterVisible"), 6, 0);
        if (!Definition) return false;
        const FGuid SpawnId = FGuid (14, 2, 2, 1);
        Fixture.Level->Objects.Add (MakeEncounterSpawn (
            Definition, SpawnId, FIntPoint (1, 4), TEXT ("EncounterVisible"), 0));
        TestTrue (TEXT ("StartEncounter with LOS succeeds as a spawn transaction"),
            Fixture.Runtime->StartMonsterEncounter (SpawnId));
        TestNotNull (TEXT ("Visible encounter member was spawned"),
            Fixture.Runtime->FindSpawnedMonsterActor (SpawnId));
        TestTrue (TEXT ("Visible encounter starts combat only in deferred perception evaluation"),
            Fixture.Engagement->ProcessPendingEvaluationNow ());
        TestTrue (TEXT ("Combat is active after visible encounter evaluation"),
            Fixture.TurnManager->bCombatActive);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON141TransitionRebuildSafetyTest,
    "Grimrock.Monsters.MON14.1.TransitionRebuildSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON141TransitionRebuildSafetyTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON141Fixture Fixture;
    if (!Fixture.Initialize ()) return false;
    AGridMonsterActor* Monster = Fixture.AddMonster (
        Fixture.MakeDefinition (TEXT ("MON14_Transition"), 6, 0),
        FIntPoint (1, 4));
    TestNotNull (TEXT ("Transition guard monster exists"), Monster);

    Fixture.Runtime->bIsExecutingDungeonTransition = true;
    Fixture.Engagement->RequestEvaluation (Fixture.Runtime, TEXT ("ContinueRestore"));
    TestFalse (TEXT ("No combat starts while Continue/transition restoration is guarded"),
        Fixture.Engagement->ProcessPendingEvaluationNow ());
    TestFalse (TEXT ("Combat remains inactive during restoration"),
        Fixture.TurnManager->bCombatActive);
    TestTrue (TEXT ("Unsafe restoration evaluation is deferred rather than lost"),
        Fixture.Engagement->HasPendingEvaluation ());

    Fixture.Runtime->bIsExecutingDungeonTransition = false;
    TestTrue (TEXT ("Deferred evaluation runs after restoration becomes stable"),
        Fixture.Engagement->ProcessPendingEvaluationNow ());

    FGridMON141Fixture RebuildFixture;
    if (!RebuildFixture.Initialize ()) return false;
    AGridMonsterActor* RebuildMonster = RebuildFixture.AddMonster (
        RebuildFixture.MakeDefinition (TEXT ("MON14_Rebuild"), 6, 0),
        FIntPoint (1, 4));
    TestNotNull (TEXT ("Rebuild guard monster exists"), RebuildMonster);
    UGridDoorSystemComponent* Doors =
        RebuildFixture.Runtime->FindComponentByClass<UGridDoorSystemComponent> ();
    TestNotNull (TEXT ("Rebuild door system exists"), Doors);
    if (!Doors) return false;
    const int32 Before = RebuildFixture.Engagement->GetQueuedRequestCount ();
    Doors->RebuildIndexes ();
    TestTrue (TEXT ("Runtime rebuild requests a deferred perception evaluation"),
        RebuildFixture.Engagement->GetQueuedRequestCount () > Before);
    TestFalse (TEXT ("Rebuild notification itself does not synchronously start combat"),
        RebuildFixture.TurnManager->bCombatActive);
    TestTrue (TEXT ("Post-rebuild safe evaluation may engage visible monster"),
        RebuildFixture.Engagement->ProcessPendingEvaluationNow ());
    return true;
}

#endif
