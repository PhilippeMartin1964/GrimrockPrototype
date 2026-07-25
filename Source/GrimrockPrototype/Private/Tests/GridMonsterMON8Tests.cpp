#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
    struct FGridMON8TestWorld
    {
        UWorld* World = nullptr;

        FGridMON8TestWorld ()
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
                    TEXT ("MON8TestWorld_%s"),
                    *FGuid::NewGuid ().ToString (EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &InitializationValues);
            if (!World || !GEngine)
            {
                return;
            }

            FWorldContext& Context =
                GEngine->CreateNewWorldContext (EWorldType::Game);
            Context.SetCurrentWorld (World);
        }

        ~FGridMON8TestWorld ()
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

    UGridLevelAsset* ConfigureMON8Floor (
        AGridLevelRuntimeActor* RuntimeActor,
        int32 Width = 5,
        int32 Height = 5)
    {
        if (!RuntimeActor)
        {
            return nullptr;
        }

        UGridLevelAsset* LevelAsset =
            NewObject<UGridLevelAsset> (RuntimeActor);
        LevelAsset->Width = Width;
        LevelAsset->Height = Height;
        LevelAsset->EnsureCellCount ();
        for (FGridLevelCellData& Cell : LevelAsset->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
        }
        RuntimeActor->LevelAsset = LevelAsset;
        return LevelAsset;
    }

    UGridMonsterDefinitionAsset* MakeMON8MonsterDefinition (
        UObject* Outer,
        FName MonsterId)
    {
        UGridMonsterDefinitionAsset* Definition =
            NewObject<UGridMonsterDefinitionAsset> (Outer);
        Definition->MonsterId = MonsterId;
        Definition->DisplayName = FText::FromString (TEXT ("MON8 Test Rat"));
        Definition->CategoryId = TEXT ("Vermin");
        Definition->MaxHealth = 8;
        Definition->ActionPointsPerTurn = 2;
        Definition->DeathExpectedDuration = 1.0f;
        return Definition;
    }

    AGridMonsterActor* SpawnMON8Monster (
        UWorld* World,
        UGridMonsterDefinitionAsset* Definition,
        FGuid SpawnId,
        FIntPoint Cell,
        bool bAddMovementAndBehavior)
    {
        if (!World || !Definition)
        {
            return nullptr;
        }

        AGridMonsterActor* Monster =
            World->SpawnActor<AGridMonsterActor> ();
        if (!Monster ||
            !Monster->InitializeMonster (
                Definition,
                SpawnId,
                Cell,
                EGridEdge::North))
        {
            return nullptr;
        }

        if (bAddMovementAndBehavior)
        {
            UGridMonsterMovementComponent* Movement =
                NewObject<UGridMonsterMovementComponent> (
                    Monster,
                    TEXT ("MON8TestMovement"));
            Movement->bAutoInitialize = false;
            Movement->bInferCellFromActorLocation = false;
            Monster->AddInstanceComponent (Movement);
            Movement->RegisterComponent ();

            UGridMonsterBehaviorComponent* Behavior =
                NewObject<UGridMonsterBehaviorComponent> (
                    Monster,
                    TEXT ("MON8TestBehavior"));
            Behavior->bAutoInitialize = false;
            Monster->AddInstanceComponent (Behavior);
            Behavior->RegisterComponent ();
        }
        return Monster;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON8LootResolverTest,
    "Grimrock.Monsters.MON8.LootResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8LootResolverTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    TArray<FGridMonsterLootEntry> LootTable;
    FGridMonsterLootRollResult Result =
        FGridMonsterLootResolver::ResolveLootFromRolls (LootTable, 0.0f, 0);
    TestFalse (TEXT ("An empty table drops nothing"), Result.bHasLoot);

    FGridMonsterLootEntry EntryA;
    EntryA.ItemDefinitionId = TEXT ("Item_A");
    EntryA.DropChance = 0.0f;
    LootTable.Add (EntryA);
    Result = FGridMonsterLootResolver::ResolveLootFromRolls (
        LootTable,
        0.0f,
        0);
    TestFalse (TEXT ("A total chance of zero drops nothing"), Result.bHasLoot);

    LootTable[0].DropChance = 1.0f;
    LootTable[0].MinQuantity = 2;
    LootTable[0].MaxQuantity = 4;
    Result = FGridMonsterLootResolver::ResolveLootFromRolls (
        LootTable,
        0.999f,
        7);
    TestTrue (TEXT ("A full chance selects the entry"), Result.bHasLoot);
    TestTrue (
        TEXT ("The resolved quantity remains inside its range"),
        Result.Quantity >= 2 && Result.Quantity <= 4);

    FGridMonsterLootEntry EntryB;
    EntryB.ItemDefinitionId = TEXT ("Item_B");
    EntryB.DropChance = 0.25f;
    LootTable[0].DropChance = 0.40f;
    LootTable.Add (EntryB);

    Result = FGridMonsterLootResolver::ResolveLootFromRolls (
        LootTable,
        0.20f,
        0);
    TestEqual (
        TEXT ("The first cumulative slice selects A"),
        Result.SelectedEntryIndex,
        0);
    Result = FGridMonsterLootResolver::ResolveLootFromRolls (
        LootTable,
        0.50f,
        0);
    TestEqual (
        TEXT ("The second cumulative slice selects B"),
        Result.SelectedEntryIndex,
        1);
    Result = FGridMonsterLootResolver::ResolveLootFromRolls (
        LootTable,
        0.80f,
        0);
    TestFalse (
        TEXT ("The residual probability means no drop"),
        Result.bHasLoot);

    FRandomStream FirstStream (8675309);
    FRandomStream SecondStream (8675309);
    const FGridMonsterLootRollResult First =
        FGridMonsterLootResolver::ResolveLoot (LootTable, FirstStream);
    const FGridMonsterLootRollResult Second =
        FGridMonsterLootResolver::ResolveLoot (LootTable, SecondStream);
    TestEqual (
        TEXT ("An identical seed produces the same selection roll"),
        First.SelectionRoll,
        Second.SelectionRoll);
    TestEqual (
        TEXT ("An identical seed produces the same selected index"),
        First.SelectedEntryIndex,
        Second.SelectedEntryIndex);
    TestEqual (
        TEXT ("An identical seed produces the same quantity"),
        First.Quantity,
        Second.Quantity);

    FGridMonsterLootEntry InvalidEntry;
    InvalidEntry.ItemDefinitionId = NAME_None;
    InvalidEntry.DropChance = 1.0f;
    LootTable.Insert (InvalidEntry, 0);
    Result = FGridMonsterLootResolver::ResolveLootFromRolls (
        LootTable,
        0.20f,
        0);
    TestEqual (
        TEXT ("An invalid entry is ignored without changing table order"),
        Result.ItemDefinitionId,
        FName (TEXT ("Item_A")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON8DeathExactlyOnceTest,
    "Grimrock.Monsters.MON8.DeathExactlyOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8DeathExactlyOnceTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON8TestWorld TestWorld;
    TestNotNull (TEXT ("The transient test world exists"), TestWorld.World);
    if (!TestWorld.World)
    {
        return false;
    }

    UGridMonsterDefinitionAsset* Definition =
        MakeMON8MonsterDefinition (TestWorld.World, TEXT ("MON8_DeathRat"));
    AGridMonsterActor* Monster = SpawnMON8Monster (
        TestWorld.World,
        Definition,
        FGuid (1, 2, 3, 4),
        FIntPoint (2, 2),
        false);
    TestNotNull (TEXT ("The death-test monster exists"), Monster);
    if (!Monster || !Monster->DeathComponent)
    {
        return false;
    }

    Monster->MarkDead ();
    TestEqual (TEXT ("CurrentHealth becomes zero"), Monster->CurrentHealth, 0);
    TestEqual (
        TEXT ("MonsterState becomes Dead"),
        Monster->MonsterState,
        EGridMonsterState::Dead);
    TestTrue (
        TEXT ("The logical death is committed"),
        Monster->DeathComponent->bDeathCommitted);
    TestTrue (
        TEXT ("The loot attempt is guarded even for an empty table"),
        Monster->DeathComponent->bLootGenerated);

    const int32 LogicalEvents =
        Monster->DeathComponent->LogicalDeathEventCount;
    const int32 PlacedLoot = Monster->DeathComponent->PlacedLootCount;
    const int32 FailedLoot = Monster->DeathComponent->FailedLootCount;
    Monster->MarkDead ();
    TestEqual (
        TEXT ("A second MarkDead does not broadcast another logical event"),
        Monster->DeathComponent->LogicalDeathEventCount,
        LogicalEvents);
    TestEqual (
        TEXT ("A second MarkDead does not place more loot"),
        Monster->DeathComponent->PlacedLootCount,
        PlacedLoot);
    TestEqual (
        TEXT ("A second MarkDead does not retry failed loot"),
        Monster->DeathComponent->FailedLootCount,
        FailedLoot);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON8OccupancyReleaseTest,
    "Grimrock.Monsters.MON8.OccupancyRelease",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8OccupancyReleaseTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON8TestWorld TestWorld;
    if (!TestNotNull (
        TEXT ("The transient occupancy world exists"),
        TestWorld.World))
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    ConfigureMON8Floor (Runtime);
    UGridMonsterDefinitionAsset* Definition =
        MakeMON8MonsterDefinition (Runtime, TEXT ("MON8_OccupancyRat"));
    AGridMonsterActor* Monster = SpawnMON8Monster (
        TestWorld.World,
        Definition,
        FGuid (10, 0, 0, 1),
        FIntPoint (2, 2),
        true);
    UGridMonsterMovementComponent* Movement = Monster
        ? Monster->FindComponentByClass<UGridMonsterMovementComponent> ()
        : nullptr;
    UGridMonsterOccupancySubsystem* Occupancy =
        TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem> ();
    TestNotNull (TEXT ("MonsterMovement exists"), Movement);
    TestNotNull (TEXT ("The MON3 occupancy subsystem exists"), Occupancy);
    if (!Monster || !Movement || !Occupancy)
    {
        return false;
    }

    TestTrue (
        TEXT ("The monster registers its occupied cell"),
        Movement->InitializeMovement (Runtime));
    TestTrue (
        TEXT ("A pending reservation can be created"),
        Occupancy->TryReserveCell (Monster, FIntPoint (3, 2)));
    Monster->MarkDead ();
    TestFalse (
        TEXT ("Death immediately releases occupation"),
        Occupancy->IsCellOccupied (FIntPoint (2, 2)));
    TestFalse (
        TEXT ("Death immediately releases reservation"),
        Occupancy->IsCellReserved (FIntPoint (3, 2)));

    AGridMonsterActor* Other = SpawnMON8Monster (
        TestWorld.World,
        Definition,
        FGuid (10, 0, 0, 2),
        FIntPoint (1, 2),
        false);
    TestTrue (
        TEXT ("Another monster can register beside the released cell"),
        Occupancy->RegisterMonster (Other, FIntPoint (1, 2)));
    TestTrue (
        TEXT ("Another monster can reserve the released death cell"),
        Occupancy->TryReserveCell (Other, FIntPoint (2, 2)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON8VictoryOnLastDeathTest,
    "Grimrock.Monsters.MON8.VictoryOnLastDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8VictoryOnLastDeathTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON8TestWorld TestWorld;
    if (!TestNotNull (
        TEXT ("The transient combat world exists"),
        TestWorld.World))
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    ConfigureMON8Floor (Runtime);
    AGrimrockPartyPawn* Party =
        TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 4;
    Party->CurrentCellY = 4;
    FGridCharacterInventoryState LivingCharacter;
    LivingCharacter.DerivedStats.CurrentHealth = 10;
    Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = {
        LivingCharacter
    };

    UGridMonsterDefinitionAsset* Definition =
        MakeMON8MonsterDefinition (Runtime, TEXT ("MON8_VictoryRat"));
    AGridMonsterActor* First = SpawnMON8Monster (
        TestWorld.World,
        Definition,
        FGuid (20, 0, 0, 1),
        FIntPoint (1, 1),
        true);
    AGridMonsterActor* Last = SpawnMON8Monster (
        TestWorld.World,
        Definition,
        FGuid (20, 0, 0, 2),
        FIntPoint (2, 1),
        true);

    UGridTurnManagerComponent* TurnManager =
        NewObject<UGridTurnManagerComponent> (
            Runtime,
            TEXT ("MON8TestTurnManager"));
    TurnManager->bAutoInitialize = false;
    Runtime->AddInstanceComponent (TurnManager);
    TurnManager->RegisterComponent ();

    TestTrue (
        TEXT ("The TurnManager initializes"),
        TurnManager->InitializeTurnManager (Runtime, Party));
    TestTrue (
        TEXT ("The encounter starts with both rats"),
        TurnManager->StartCombatWithAllMonsters ());
    First->MarkDead ();
    TestTrue (
        TEXT ("The encounter remains active after the first death"),
        TurnManager->bCombatActive);
    Last->MarkDead ();
    TestFalse (
        TEXT ("The encounter ends immediately after the last death"),
        TurnManager->bCombatActive);
    TestEqual (
        TEXT ("The terminal phase is Victory"),
        TurnManager->CurrentPhase,
        EGridCombatPhase::Victory);
    TestTrue (
        TEXT ("No pending enemy action remains"),
        TurnManager->PendingActions.IsEmpty ());
    TestFalse (
        TEXT ("Party input is re-enabled"),
        TurnManager->IsPartyInputLocked ());

    Last->MarkDead ();
    TestEqual (
        TEXT ("A duplicate death leaves Victory unchanged"),
        TurnManager->CurrentPhase,
        EGridCombatPhase::Victory);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON8MonsterDiedEventTest,
    "Grimrock.Monsters.MON8.MonsterDiedEvent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON8MonsterDiedEventTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    TestEqual (
        TEXT ("Activated keeps its historic numeric value"),
        static_cast<uint8> (EGridObjectEvent::Activated),
        static_cast<uint8> (0));
    TestEqual (
        TEXT ("Disabled keeps its historic numeric value"),
        static_cast<uint8> (EGridObjectEvent::Disabled),
        static_cast<uint8> (11));
    TestEqual (
        TEXT ("MonsterDied is appended after all historic events"),
        static_cast<uint8> (EGridObjectEvent::MonsterDied),
        static_cast<uint8> (12));

    FGridMON8TestWorld TestWorld;
    if (!TestWorld.World)
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    UGridLevelAsset* LevelAsset = ConfigureMON8Floor (Runtime);
    const FGuid SourceId (30, 0, 0, 1);
    const FGuid TargetId (30, 0, 0, 2);

    FGridLevelObjectData Source;
    Source.ObjectId = SourceId;
    Source.Type = EGridLevelObjectType::MonsterSpawn;
    Source.CellX = 1;
    Source.CellY = 1;
    LevelAsset->Objects.Add (Source);

    FGridLevelObjectData Target;
    Target.ObjectId = TargetId;
    Target.Type = EGridLevelObjectType::Trigger;
    Target.CellX = 3;
    Target.CellY = 3;
    LevelAsset->Objects.Add (Target);

    FGridObjectLink Link;
    Link.SourceObjectId = SourceId;
    Link.TargetObjectId = TargetId;
    Link.SourceEvent = EGridObjectEvent::MonsterDied;
    Link.Command = EGridObjectCommand::Activate;
    LevelAsset->Links.Add (Link);
    Runtime->RebuildLevel ();

    UGridMonsterDefinitionAsset* Definition =
        MakeMON8MonsterDefinition (Runtime, TEXT ("MON8_EventRat"));
    AGridMonsterActor* Monster = SpawnMON8Monster (
        TestWorld.World,
        Definition,
        SourceId,
        FIntPoint (1, 1),
        false);
    Monster->DeathComponent->InitializeDeathComponent (Runtime);
    Monster->MarkDead ();
    TestEqual (
        TEXT ("The MonsterDied link is attempted once"),
        Monster->DeathComponent->LinkExecutionAttemptCount,
        1);
    Monster->MarkDead ();
    TestEqual (
        TEXT ("A second MarkDead does not re-execute the link"),
        Monster->DeathComponent->LinkExecutionAttemptCount,
        1);

    AGridMonsterActor* UnlinkedMonster = SpawnMON8Monster (
        TestWorld.World,
        Definition,
        FGuid (30, 0, 0, 3),
        FIntPoint (2, 2),
        false);
    UnlinkedMonster->DeathComponent->InitializeDeathComponent (Runtime);
    UnlinkedMonster->MarkDead ();
    TestTrue (
        TEXT ("A missing LevelAsset link does not prevent death"),
        UnlinkedMonster->DeathComponent->bDeathCommitted);
    return true;
}

#endif
