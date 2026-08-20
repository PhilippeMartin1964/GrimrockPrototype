#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridItemDefinitionAsset.h"
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
#include "Save/GrimrockPartySaveGame.h"

namespace
{
    constexpr int32 MON176GoblinExperienceReward = 125;
    const FName MON176GoblinId (TEXT ("MON_GoblinThrower"));

    struct FGridMON176TestWorld
    {
        UWorld* World = nullptr;

        explicit FGridMON176TestWorld (
            EWorldType::Type WorldType = EWorldType::Game)
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
                WorldType,
                false,
                FName (*FString::Printf (
                    TEXT ("MON176TestWorld_%s"),
                    *FGuid::NewGuid ().ToString (
                        EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &InitializationValues);
            if (!World || !GEngine)
            {
                return;
            }

            FWorldContext& Context =
                GEngine->CreateNewWorldContext (WorldType);
            Context.SetCurrentWorld (World);
        }

        ~FGridMON176TestWorld ()
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

    UGridLevelAsset* MakeMON176Floor (
        AGridLevelRuntimeActor* Runtime,
        int32 Width = 6,
        int32 Height = 6)
    {
        if (!Runtime)
        {
            return nullptr;
        }

        UGridLevelAsset* Level = NewObject<UGridLevelAsset> (Runtime);
        Level->Width = Width;
        Level->Height = Height;
        Level->EnsureCellCount ();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
        }
        Runtime->LevelAsset = Level;
        return Level;
    }

    FGridCharacterInventoryState MakeMON176Character ()
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid (17, 6, 1, 1);
        Character.DisplayName = FText::FromString (TEXT ("MON17.6 Hero"));
        Character.Level = 1;
        Character.Experience = 0;
        Character.DerivedStats.MaxHealth = 20;
        Character.DerivedStats.CurrentHealth = 20;
        Character.DerivedStats.Initiative = 10;
        return Character;
    }

    AGrimrockPartyPawn* SpawnMON176Party (
        UWorld* World,
        AGridLevelRuntimeActor* Runtime)
    {
        if (!World)
        {
            return nullptr;
        }

        AGrimrockPartyPawn* Party =
            World->SpawnActor<AGrimrockPartyPawn> ();
        if (!Party || !Party->PartyInventoryComponent)
        {
            return nullptr;
        }

        Party->LevelRuntimeActor = Runtime;
        Party->CurrentCellX = 5;
        Party->CurrentCellY = 5;
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = {
            MakeMON176Character ()
        };
        Party->PartyInventoryComponent->PartyInventoryState.CharacterPool.Reset ();
        return Party;
    }

    UGridMonsterDefinitionAsset* LoadMON176ProductionGoblin (
        FAutomationTestBase& Test)
    {
        UGridMonsterDefinitionAsset* Definition =
            LoadObject<UGridMonsterDefinitionAsset> (
                nullptr,
                TEXT ("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower"));
        Test.TestNotNull (
            TEXT ("Production DA_MON_GoblinThrower loads"),
            Definition);
        return Definition;
    }

    UGridItemDefinitionAsset* LoadMON176ProductionStone (
        FAutomationTestBase& Test)
    {
        UGridItemDefinitionAsset* Definition =
            LoadObject<UGridItemDefinitionAsset> (
                nullptr,
                TEXT ("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Stone.DA_Item_Stone"));
        Test.TestNotNull (
            TEXT ("Production DA_Item_Stone loads"),
            Definition);
        return Definition;
    }

    UGridItemDefinitionAsset* MakeMON176ItemDefinition (
        UObject* Outer,
        FName ItemId)
    {
        UGridItemDefinitionAsset* Item =
            NewObject<UGridItemDefinitionAsset> (Outer);
        Item->ItemDefinitionId = ItemId;
        Item->DisplayName = FText::FromName (ItemId);
        Item->Weight = 1.0f;
        return Item;
    }

    void AddMON176GuaranteedLoot (
        UGridMonsterDefinitionAsset* Definition,
        UGridItemDefinitionAsset* Item)
    {
        if (!Definition || !Item)
        {
            return;
        }

        FGridMonsterLootEntry Entry;
        Entry.ItemDefinitionId = Item->ItemDefinitionId;
        Entry.ItemDefinitionAsset = Item;
        Entry.DropChance = 1.0f;
        Entry.MinQuantity = 1;
        Entry.MaxQuantity = 1;
        Definition->LootTable.Add (Entry);
    }

    UGridMonsterDefinitionAsset* MakeMON176SyntheticGoblin (
        UObject* Outer,
        const UGridMonsterDefinitionAsset* PresentationSource = nullptr)
    {
        UGridMonsterDefinitionAsset* Definition =
            NewObject<UGridMonsterDefinitionAsset> (Outer);
        Definition->MonsterId = MON176GoblinId;
        Definition->DisplayName =
            FText::FromString (TEXT ("Gobelin lanceur"));
        Definition->CategoryId = TEXT ("Goblin");
        Definition->DangerLevel = 3;
        Definition->MaxHealth = 10;
        Definition->Initiative = 12;
        Definition->Accuracy = 2;
        Definition->ActionPointsPerTurn = 3;
        Definition->SightRangeCells = 8;
        Definition->HearingRangeCells = 4;
        Definition->PrimaryAIProfile = EGridMonsterAIProfile::RangedKeeper;
        Definition->PreferredMinDistance = 3;
        Definition->PreferredMaxDistance = 5;
        Definition->ExperienceReward = MON176GoblinExperienceReward;
        Definition->DeathExpectedDuration = 0.01f;

        if (PresentationSource)
        {
            Definition->SkeletalMesh = PresentationSource->SkeletalMesh;
            Definition->AnimationClass = PresentationSource->AnimationClass;
            Definition->MonsterActorClass = PresentationSource->MonsterActorClass;
            Definition->VisualScale = PresentationSource->VisualScale;
            Definition->VisualOffset = PresentationSource->VisualOffset;
            Definition->VisualRotationOffset =
                PresentationSource->VisualRotationOffset;
        }
        return Definition;
    }

    void AddMON176SyntheticCanonicalLoot (
        UGridMonsterDefinitionAsset* Definition)
    {
        if (!Definition)
        {
            return;
        }

        // The ArtBook contract is knife / shaped stone / empty vial.
        // Knife and vial production DataAssets do not exist yet, so these
        // MON17.6-only ids deliberately stay synthetic and non-authoritative.
        AddMON176GuaranteedLoot (
            Definition,
            MakeMON176ItemDefinition (
                Definition,
                TEXT ("MON176_GoblinKnife")));
        AddMON176GuaranteedLoot (
            Definition,
            MakeMON176ItemDefinition (
                Definition,
                TEXT ("MON176_ShapedStone")));
        AddMON176GuaranteedLoot (
            Definition,
            MakeMON176ItemDefinition (
                Definition,
                TEXT ("MON176_EmptyVial")));
    }

    FGridLevelObjectData MakeMON176Spawn (
        UGridMonsterDefinitionAsset* Definition,
        FGuid SpawnId,
        FIntPoint Cell,
        FName EncounterId = NAME_None,
        int32 WaveIndex = 0,
        bool bInitiallyEnabled = true)
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
        Spawn.EncounterGroupId = EncounterId;
        Spawn.EncounterWaveIndex = WaveIndex;
        Spawn.bInitiallyEnabled = bInitiallyEnabled;
        return Spawn;
    }

    AGridMonsterActor* SpawnMON176ManualGoblin (
        UWorld* World,
        AGridLevelRuntimeActor* Runtime,
        UGridMonsterDefinitionAsset* Definition,
        FGuid SpawnId,
        FIntPoint Cell)
    {
        if (!World || !Runtime || !Definition)
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

        UGridMonsterMovementComponent* Movement =
            Monster->FindComponentByClass<UGridMonsterMovementComponent> ();
        if (!Movement)
        {
            Movement = NewObject<UGridMonsterMovementComponent> (
                Monster,
                TEXT ("MON176Movement"));
            Movement->bAutoInitialize = false;
            Movement->bInferCellFromActorLocation = false;
            Monster->AddInstanceComponent (Movement);
            Movement->RegisterComponent ();
        }
        if (!Movement->IsInitialized () &&
            !Movement->InitializeMovement (Runtime))
        {
            Monster->Destroy ();
            return nullptr;
        }

        UGridMonsterBehaviorComponent* Behavior =
            Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ();
        if (!Behavior)
        {
            Behavior = NewObject<UGridMonsterBehaviorComponent> (
                Monster,
                TEXT ("MON176Behavior"));
            Behavior->bAutoInitialize = false;
            Behavior->bRefreshPerceptionOnBeginPlay = false;
            Monster->AddInstanceComponent (Behavior);
            Behavior->RegisterComponent ();
        }

        if (Monster->DeathComponent)
        {
            Monster->DeathComponent->InitializeDeathComponent (Runtime);
        }
        return Monster;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1761ProductionRewardContractTest,
    "Grimrock.Monsters.MON17.6.1.ProductionRewardContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1761ProductionRewardContractTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    UGridMonsterDefinitionAsset* ProductionGoblin =
        LoadMON176ProductionGoblin (*this);
    UGridItemDefinitionAsset* ProductionStone =
        LoadMON176ProductionStone (*this);
    if (!ProductionGoblin || !ProductionStone)
    {
        return false;
    }

    TestEqual (TEXT ("Production monster id remains MON_GoblinThrower"),
        ProductionGoblin->MonsterId,
        MON176GoblinId);
    TestEqual (TEXT ("Production Goblin reward remains 125 XP"),
        ProductionGoblin->ExperienceReward,
        MON176GoblinExperienceReward);
    TestEqual (TEXT ("Existing production stone resolves to Stone"),
        ProductionStone->ItemDefinitionId,
        FName (TEXT ("Stone")));

    UGridMonsterDefinitionAsset* Contract =
        MakeMON176SyntheticGoblin (GetTransientPackage ());
    AddMON176SyntheticCanonicalLoot (Contract);
    FString Error;
    TestTrue (TEXT ("Canonical synthetic reward contract validates"),
        Contract->ValidateDefinition (Error));
    TestEqual (TEXT ("ArtBook contract exposes three independent loot entries"),
        Contract->LootTable.Num (),
        3);

    const TArray<FGridMonsterLootRollResult> Results =
        FGridMonsterLootResolver::ResolveLoot (
            Contract->LootTable,
            1761);
    TestEqual (TEXT ("Every canonical entry is evaluated independently"),
        Results.Num (),
        3);
    for (const FGridMonsterLootRollResult& Result : Results)
    {
        TestTrue (TEXT ("Guaranteed contract entry drops"), Result.bDropped);
        TestEqual (TEXT ("Guaranteed contract entry quantity is one"),
            Result.Quantity,
            1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1761EncounterWaveParticipationTest,
    "Grimrock.Monsters.MON17.6.1.EncounterWaveParticipation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1761EncounterWaveParticipationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON176TestWorld TestWorld (EWorldType::PIE);
    if (!TestNotNull (TEXT ("MON17.6 encounter world exists"), TestWorld.World))
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    UGridLevelAsset* Level = MakeMON176Floor (Runtime);
    AGrimrockPartyPawn* Party = SpawnMON176Party (TestWorld.World, Runtime);
    UGridMonsterDefinitionAsset* Definition =
        LoadMON176ProductionGoblin (*this);
    if (!Runtime || !Level || !Party || !Definition)
    {
        return false;
    }

    const FName EncounterId (TEXT ("Encounter_MON176_GoblinWaves"));
    const FGuid Wave0AId (17, 6, 2, 1);
    const FGuid Wave0BId (17, 6, 2, 2);
    const FGuid Wave1Id (17, 6, 2, 3);

    Level->Objects.Add (MakeMON176Spawn (
        Definition, Wave0AId, FIntPoint (1, 1), EncounterId, 0, false));
    Level->Objects.Add (MakeMON176Spawn (
        Definition, Wave0BId, FIntPoint (2, 1), EncounterId, 0, false));
    Level->Objects.Add (MakeMON176Spawn (
        Definition, Wave1Id, FIntPoint (3, 1), EncounterId, 1, false));

    Runtime->RebuildLevel ();
    TestEqual (TEXT ("Encounter Goblins start absent"),
        Runtime->GetSpawnedMonsterActorCount (),
        0);
    TestTrue (TEXT ("Goblin encounter starts through the generic MON13 API"),
        Runtime->StartMonsterEncounter (Wave0AId));
    TestEqual (TEXT ("Wave zero spawns both Goblins"),
        Runtime->GetSpawnedMonsterActorCount (),
        2);
    TestEqual (TEXT ("Wave zero is active"),
        Runtime->GetMonsterEncounterActiveWave (EncounterId),
        0);
    TestNull (TEXT ("Wave one remains deferred"),
        Runtime->FindSpawnedMonsterActor (Wave1Id));

    AGridMonsterActor* Wave0A =
        Runtime->FindSpawnedMonsterActor (Wave0AId);
    AGridMonsterActor* Wave0B =
        Runtime->FindSpawnedMonsterActor (Wave0BId);
    if (!TestNotNull (TEXT ("First wave-zero Goblin exists"), Wave0A) ||
        !TestNotNull (TEXT ("Second wave-zero Goblin exists"), Wave0B))
    {
        return false;
    }

    Wave0A->MarkDead ();
    TestEqual (TEXT ("One death does not advance an incomplete wave"),
        Runtime->GetMonsterEncounterActiveWave (EncounterId),
        0);
    TestNull (TEXT ("Future wave is still absent after one death"),
        Runtime->FindSpawnedMonsterActor (Wave1Id));

    Wave0B->MarkDead ();
    AGridMonsterActor* Wave1 =
        Runtime->FindSpawnedMonsterActor (Wave1Id);
    TestNotNull (TEXT ("Completing wave zero spawns wave one"), Wave1);
    TestEqual (TEXT ("Wave one becomes active"),
        Runtime->GetMonsterEncounterActiveWave (EncounterId),
        1);
    if (!Wave1)
    {
        return false;
    }

    Wave1->MarkDead ();
    TestTrue (TEXT ("Final Goblin death completes the encounter"),
        Runtime->IsMonsterEncounterCompleted (EncounterId));
    TestEqual (TEXT ("Completed Goblin encounter has no active wave"),
        Runtime->GetMonsterEncounterActiveWave (EncounterId),
        INDEX_NONE);
    TestEqual (TEXT ("Three production Goblins award three times 125 XP"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        3 * MON176GoblinExperienceReward);

    const FGridLevelRuntimeState* State =
        Runtime->FindRuntimeStateForCurrentLevel ();
    const FGridRuntimeMonsterEncounterState* EncounterState =
        State ? State->MonsterEncounters.Find (EncounterId) : nullptr;
    TestTrue (TEXT ("All three spawn ids are persisted as defeated"),
        EncounterState && EncounterState->DefeatedSpawnIds.Num () == 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1761DeathRewardsExactlyOnceTest,
    "Grimrock.Monsters.MON17.6.1.DeathRewardsExactlyOnce",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1761DeathRewardsExactlyOnceTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON176TestWorld TestWorld;
    if (!TestNotNull (TEXT ("MON17.6 reward world exists"), TestWorld.World))
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    UGridLevelAsset* Level = MakeMON176Floor (Runtime);
    AGrimrockPartyPawn* Party = SpawnMON176Party (TestWorld.World, Runtime);
    UGridMonsterDefinitionAsset* Definition =
        MakeMON176SyntheticGoblin (Runtime);
    AddMON176SyntheticCanonicalLoot (Definition);
    if (!Runtime || !Level || !Party || !Definition)
    {
        return false;
    }

    AGridMonsterActor* Monster = SpawnMON176ManualGoblin (
        TestWorld.World,
        Runtime,
        Definition,
        FGuid (17, 6, 3, 1),
        FIntPoint (2, 2));
    UGridMonsterOccupancySubsystem* Occupancy =
        TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem> ();
    if (!TestNotNull (TEXT ("Reward Goblin exists"), Monster) ||
        !Monster->DeathComponent ||
        !TestNotNull (TEXT ("Occupancy subsystem exists"), Occupancy))
    {
        return false;
    }

    UGridTurnManagerComponent* TurnManager =
        NewObject<UGridTurnManagerComponent> (
            Runtime,
            TEXT ("MON176TurnManager"));
    TurnManager->bAutoInitialize = false;
    Runtime->AddInstanceComponent (TurnManager);
    TurnManager->RegisterComponent ();
    TestTrue (TEXT ("TurnManager initializes for the Goblin"),
        TurnManager->InitializeTurnManager (Runtime, Party));
    TestTrue (TEXT ("Combat starts with the reward Goblin"),
        TurnManager->StartCombatWithAllMonsters ());
    TestTrue (TEXT ("Goblin initially occupies its cell"),
        Occupancy->IsCellOccupied (FIntPoint (2, 2)));

    Monster->MarkDead ();
    TestEqual (TEXT ("Goblin death awards exactly 125 XP"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        MON176GoblinExperienceReward);
    TestEqual (TEXT ("All three guaranteed canonical loot entries are placed"),
        Monster->DeathComponent->PlacedLootCount,
        3);
    TestEqual (TEXT ("No canonical loot placement fails"),
        Monster->DeathComponent->FailedLootCount,
        0);
    TestEqual (TEXT ("Logical death is emitted exactly once"),
        Monster->DeathComponent->LogicalDeathEventCount,
        1);
    TestFalse (TEXT ("Death releases Goblin occupancy immediately"),
        Occupancy->IsCellOccupied (FIntPoint (2, 2)));
    TestFalse (TEXT ("Last Goblin death closes combat"),
        TurnManager->bCombatActive);
    TestEqual (TEXT ("Last Goblin death reaches Victory"),
        TurnManager->CurrentPhase,
        EGridCombatPhase::Victory);

    TestTrue (TEXT ("Loot can be captured into runtime persistence"),
        Runtime->CaptureCurrentLevelRuntimeState ());
    const FGridLevelRuntimeState* FirstState =
        Runtime->FindRuntimeStateForCurrentLevel ();
    TestEqual (TEXT ("Three independent world items are persisted"),
        FirstState ? FirstState->Items.Num () : 0,
        3);

    Monster->MarkDead ();
    TestEqual (TEXT ("Second MarkDead does not replay XP"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        MON176GoblinExperienceReward);
    TestEqual (TEXT ("Second MarkDead does not replay logical death"),
        Monster->DeathComponent->LogicalDeathEventCount,
        1);
    TestEqual (TEXT ("Second MarkDead does not place more loot"),
        Monster->DeathComponent->PlacedLootCount,
        3);
    TestTrue (TEXT ("Runtime state can be recaptured after duplicate death"),
        Runtime->CaptureCurrentLevelRuntimeState ());
    const FGridLevelRuntimeState* SecondState =
        Runtime->FindRuntimeStateForCurrentLevel ();
    TestEqual (TEXT ("Duplicate death leaves persisted loot count unchanged"),
        SecondState ? SecondState->Items.Num () : 0,
        3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1761PersistenceNoReplayTest,
    "Grimrock.Monsters.MON17.6.1.PersistenceNoReplay",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1761PersistenceNoReplayTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON176TestWorld TestWorld;
    if (!TestNotNull (TEXT ("MON17.6 persistence world exists"), TestWorld.World))
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    UGridLevelAsset* Level = MakeMON176Floor (Runtime);
    AGrimrockPartyPawn* Party = SpawnMON176Party (TestWorld.World, Runtime);
    UGridMonsterDefinitionAsset* ProductionGoblin =
        LoadMON176ProductionGoblin (*this);
    UGridItemDefinitionAsset* ProductionStone =
        LoadMON176ProductionStone (*this);
    if (!Runtime || !Level || !Party || !ProductionGoblin || !ProductionStone)
    {
        return false;
    }

    UGridMonsterDefinitionAsset* Definition =
        MakeMON176SyntheticGoblin (Runtime, ProductionGoblin);
    AddMON176GuaranteedLoot (Definition, ProductionStone);
    const FGuid SpawnId (17, 6, 4, 1);
    Level->Objects.Add (MakeMON176Spawn (
        Definition,
        SpawnId,
        FIntPoint (2, 2)));

    Runtime->RebuildLevel ();
    AGridMonsterActor* Monster =
        Runtime->FindSpawnedMonsterActor (SpawnId);
    if (!TestNotNull (TEXT ("Persistent Goblin placement spawns"), Monster) ||
        !Monster->DeathComponent)
    {
        return false;
    }

    Monster->MarkDead ();
    TestEqual (TEXT ("First committed death awards 125 XP"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        MON176GoblinExperienceReward);
    TestEqual (TEXT ("First committed death places one production Stone"),
        Monster->DeathComponent->PlacedLootCount,
        1);
    TestTrue (TEXT ("Dead Goblin and Stone capture into runtime state"),
        Runtime->CaptureCurrentLevelRuntimeState ());

    const FGridLevelRuntimeState* CapturedState =
        Runtime->FindRuntimeStateForCurrentLevel ();
    const FGridRuntimeMonsterPlacementState* CapturedPlacement =
        CapturedState
            ? CapturedState->MonsterPlacements.Find (SpawnId)
            : nullptr;
    TestTrue (TEXT ("Placement persists a committed dead Goblin"),
        CapturedPlacement &&
        CapturedPlacement->bHasMonsterState &&
        CapturedPlacement->MonsterState.bIsDead);
    TestEqual (TEXT ("Exactly one production loot item is persisted"),
        CapturedState ? CapturedState->Items.Num () : 0,
        1);

    UGrimrockPartySaveGame* SourceSave =
        NewObject<UGrimrockPartySaveGame> (Runtime);
    SourceSave->DungeonRuntimeState = Runtime->DungeonRuntimeState;
    TArray<uint8> SaveBytes;
    TestTrue (TEXT ("Goblin death state serializes through SaveGame"),
        UGameplayStatics::SaveGameToMemory (SourceSave, SaveBytes));
    const UGrimrockPartySaveGame* LoadedSave =
        Cast<UGrimrockPartySaveGame> (
            UGameplayStatics::LoadGameFromMemory (SaveBytes));
    if (!TestNotNull (TEXT ("Goblin SaveGame round trip loads"), LoadedSave))
    {
        return false;
    }

    Runtime->DungeonRuntimeState = LoadedSave->DungeonRuntimeState;
    Runtime->RebuildLevel ();
    TestTrue (TEXT ("Continue applies persisted runtime state"),
        Runtime->ApplyCurrentLevelRuntimeState ());
    AGridMonsterActor* RestoredMonster =
        Runtime->FindSpawnedMonsterActor (SpawnId);
    if (!TestNotNull (TEXT ("Continue restores the Goblin placement"), RestoredMonster) ||
        !RestoredMonster->DeathComponent)
    {
        return false;
    }
    TestEqual (TEXT ("Continue restores Goblin state as Dead"),
        RestoredMonster->MonsterState,
        EGridMonsterState::Dead);
    TestTrue (TEXT ("Continue restores the exactly-once death guard"),
        RestoredMonster->DeathComponent->bDeathCommitted);

    RestoredMonster->MarkDead ();
    TestEqual (TEXT ("MarkDead after Continue does not replay XP"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        MON176GoblinExperienceReward);
    TestTrue (TEXT ("Post-Continue state recaptures"),
        Runtime->CaptureCurrentLevelRuntimeState ());
    const FGridLevelRuntimeState* RestoredState =
        Runtime->FindRuntimeStateForCurrentLevel ();
    TestEqual (TEXT ("Continue does not duplicate persisted Stone loot"),
        RestoredState ? RestoredState->Items.Num () : 0,
        1);
    return true;
}

#endif