#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
    struct FGridMON11TestWorld
    {
        UWorld* World = nullptr;

        FGridMON11TestWorld ()
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
                    TEXT ("MON11TestWorld_%s"),
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

        ~FGridMON11TestWorld ()
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

    struct FGridMON11Fixture
    {
        FGridMON11TestWorld TestWorld;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* LevelAsset = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
        UGridMonsterDefinitionAsset* MonsterDefinition = nullptr;
        AGridMonsterActor* FrontMonster = nullptr;
        UGridMonsterOccupancySubsystem* Occupancy = nullptr;
        UGridTurnManagerComponent* TurnManager = nullptr;
        FGuid FrontMonsterId = FGuid (11, 1, 0, 1);

        FGridMON11Fixture ()
        {
            if (!TestWorld.World)
            {
                return;
            }

            Runtime =
                TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
            LevelAsset = NewObject<UGridLevelAsset> (Runtime);
            LevelAsset->Width = 4;
            LevelAsset->Height = 4;
            LevelAsset->EnsureCellCount ();
            for (FGridLevelCellData& Cell : LevelAsset->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
            }
            Runtime->LevelAsset = LevelAsset;

            Party =
                TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 1;
            Party->CurrentCellY = 1;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation (Runtime->GetCellCenterWorld (
                Party->CurrentCellX,
                Party->CurrentCellY,
                Party->EyeHeight));
            Party->SetActorRotation (FRotator (
                0.0f,
                GridDirectionUtils::ToYaw (Party->Facing),
                0.0f));

            FGridCharacterInventoryState FirstCharacter;
            FirstCharacter.CharacterId = FGuid (11, 2, 0, 1);
            FirstCharacter.DisplayName =
                FText::FromString (TEXT ("Elias"));
            FirstCharacter.DerivedStats.MaxHealth = 12;
            FirstCharacter.DerivedStats.CurrentHealth = 12;

            FGridCharacterInventoryState SecondCharacter;
            SecondCharacter.CharacterId = FGuid (11, 2, 0, 2);
            SecondCharacter.DisplayName =
                FText::FromString (TEXT ("Mina"));
            SecondCharacter.DerivedStats.MaxHealth = 10;
            SecondCharacter.DerivedStats.CurrentHealth = 10;

            Party->PartyInventoryComponent->PartyInventoryState
                .ActiveCharacters = { FirstCharacter, SecondCharacter };
            Party->PartyInventoryComponent->PartyInventoryState
                .SelectedCharacterIndex = 0;

            MonsterDefinition =
                NewObject<UGridMonsterDefinitionAsset> (Runtime);
            MonsterDefinition->MonsterId = TEXT ("MON11_Rat");
            MonsterDefinition->DisplayName =
                FText::FromString (TEXT ("Rat de test MON11"));
            MonsterDefinition->CategoryId = TEXT ("Vermin");
            MonsterDefinition->MaxHealth = 8;
            MonsterDefinition->PhysicalArmor = 2;
            MonsterDefinition->MagicalArmor = 1;
            MonsterDefinition->ActionPointsPerTurn = 2;

            FrontMonster =
                TestWorld.World->SpawnActor<AGridMonsterActor> ();
            FrontMonster->InitializeMonster (
                MonsterDefinition,
                FrontMonsterId,
                FIntPoint (1, 2),
                EGridEdge::South);

            Occupancy = TestWorld.World
                ->GetSubsystem<UGridMonsterOccupancySubsystem> ();
            if (Occupancy)
            {
                Occupancy->RegisterMonster (
                    FrontMonster,
                    FrontMonster->CurrentCell);
            }

            TurnManager = NewObject<UGridTurnManagerComponent> (
                Runtime,
                TEXT ("MON11TurnManager"));
            TurnManager->bAutoInitialize = false;
            Runtime->AddInstanceComponent (TurnManager);
            TurnManager->RegisterComponent ();
            TurnManager->InitializeTurnManager (Runtime, Party);
            TurnManager->bCombatActive = true;
            TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
            TurnManager->RoundNumber = 1;
            TurnManager->CombatMonsters = { FrontMonster };
        }

        bool IsReady () const
        {
            return TestWorld.World &&
                Runtime &&
                LevelAsset &&
                Party &&
                Party->PartyInventoryComponent &&
                MonsterDefinition &&
                FrontMonster &&
                Occupancy &&
                TurnManager;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11TargetingTest,
    "Grimrock.Monsters.MON11.Targeting",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11TargetingTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FGridMON11Fixture Fixture;
    if (!TestTrue (TEXT ("The targeting fixture is ready"), Fixture.IsReady ()))
    {
        return false;
    }

    AGridMonsterActor* LateralMonster =
        Fixture.TestWorld.World->SpawnActor<AGridMonsterActor> ();
    const FGuid LateralMonsterId (11, 1, 0, 2);
    LateralMonster->InitializeMonster (
        Fixture.MonsterDefinition,
        LateralMonsterId,
        FIntPoint (2, 1),
        EGridEdge::West);
    TestTrue (
        TEXT ("The lateral monster registers"),
        Fixture.Occupancy->RegisterMonster (
            LateralMonster,
            LateralMonster->CurrentCell));
    Fixture.TurnManager->CombatMonsters.Add (LateralMonster);

    FGridPlayerAttackRequest Request;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    TestTrue (
        TEXT ("The exact occupant in the facing cell is targeted"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("North facing transforms into the north neighbor"),
        Request.TargetCell,
        FIntPoint (1, 2));
    TestEqual (
        TEXT ("The front occupant is returned"),
        Request.TargetMonsterId,
        Fixture.FrontMonsterId);
    TestNotEqual (
        TEXT ("The lateral occupant is ignored"),
        Request.TargetMonsterId,
        LateralMonsterId);

    Fixture.TurnManager->ResetPlayerAttackPhaseState ();
    Fixture.LevelAsset->GetCellMutable (1, 1).NorthWall =
        EGridWallType::Solid;
    Request = FGridPlayerAttackRequest ();
    TestFalse (
        TEXT ("A solid wall blocks the attack request"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("A wall reports PassageBlocked"),
        RejectReason,
        EGridPlayerAttackRejectReason::PassageBlocked);

    Fixture.LevelAsset->GetCellMutable (1, 1).NorthWall =
        EGridWallType::None;
    Fixture.Occupancy->UnregisterMonster (Fixture.FrontMonster);
    Request = FGridPlayerAttackRequest ();
    TestFalse (
        TEXT ("A lateral monster is not selected when the front cell is empty"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("An empty front cell reports NoMonsterInFront"),
        RejectReason,
        EGridPlayerAttackRejectReason::NoMonsterInFront);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11RequestValidationTest,
    "Grimrock.Monsters.MON11.RequestValidation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11RequestValidationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON11Fixture Fixture;
    if (!TestTrue (TEXT ("The validation fixture is ready"), Fixture.IsReady ()))
    {
        return false;
    }

    FGridPlayerAttackRequest Request;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    Fixture.TurnManager->bCombatActive = false;
    Fixture.TurnManager->CurrentPhase = EGridCombatPhase::Exploration;
    TestFalse (
        TEXT ("Exploration rejects a player attack"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("Exploration reports inactive combat"),
        RejectReason,
        EGridPlayerAttackRejectReason::CombatInactive);

    Fixture.TurnManager->bCombatActive = true;
    Fixture.TurnManager->CurrentPhase = EGridCombatPhase::EnemyPhase;
    TestFalse (
        TEXT ("EnemyPhase rejects a player attack"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("EnemyPhase reports NotPlayerPhase"),
        RejectReason,
        EGridPlayerAttackRejectReason::NotPlayerPhase);

    Fixture.TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
    TestFalse (
        TEXT ("An invalid character index is rejected"),
        Fixture.TurnManager->RequestCharacterAttack (
            99,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("An invalid index reports InvalidAttacker"),
        RejectReason,
        EGridPlayerAttackRejectReason::InvalidAttacker);

    FGridCharacterInventoryState& Attacker =
        Fixture.Party->PartyInventoryComponent->PartyInventoryState
            .ActiveCharacters[0];
    const FGuid CharacterId = Attacker.CharacterId;
    Attacker.CharacterId.Invalidate ();
    TestFalse (
        TEXT ("An attacker without CharacterId is rejected"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("A missing CharacterId reports InvalidAttacker"),
        RejectReason,
        EGridPlayerAttackRejectReason::InvalidAttacker);

    Attacker.CharacterId = CharacterId;
    const int32 Health = Attacker.DerivedStats.CurrentHealth;
    Attacker.DerivedStats.CurrentHealth = 0;
    TestFalse (
        TEXT ("A defeated attacker is rejected"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("A zero-health attacker reports AttackerDefeated"),
        RejectReason,
        EGridPlayerAttackRejectReason::AttackerDefeated);
    Attacker.DerivedStats.CurrentHealth = Health;

    Fixture.TurnManager->CombatMonsters.Reset ();
    TestFalse (
        TEXT ("A target outside the encounter is rejected"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("An external target reports TargetNotInEncounter"),
        RejectReason,
        EGridPlayerAttackRejectReason::TargetNotInEncounter);
    Fixture.TurnManager->CombatMonsters = { Fixture.FrontMonster };

    Fixture.FrontMonster->bMonsterEnabled = false;
    TestFalse (
        TEXT ("A disabled target is rejected"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("A disabled target reports TargetInactive"),
        RejectReason,
        EGridPlayerAttackRejectReason::TargetInactive);
    Fixture.FrontMonster->bMonsterEnabled = true;

    Fixture.FrontMonster->bRuntimeLevelActive = false;
    TestFalse (
        TEXT ("An inactive-level target is rejected"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("An inactive-level target reports TargetInactive"),
        RejectReason,
        EGridPlayerAttackRejectReason::TargetInactive);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11RequestAcceptedWithoutDamageTest,
    "Grimrock.Monsters.MON11.RequestAcceptedWithoutDamage",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11RequestAcceptedWithoutDamageTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON11Fixture Fixture;
    if (!TestTrue (TEXT ("The acceptance fixture is ready"), Fixture.IsReady ()))
    {
        return false;
    }

    const FGridCharacterInventoryState& Attacker =
        Fixture.Party->PartyInventoryComponent->PartyInventoryState
            .ActiveCharacters[0];
    const int32 Health = Fixture.FrontMonster->CurrentHealth;
    const int32 PhysicalArmor =
        Fixture.FrontMonster->CurrentPhysicalArmor;
    const int32 MagicalArmor =
        Fixture.FrontMonster->CurrentMagicalArmor;
    const EGridMonsterState State =
        Fixture.FrontMonster->MonsterState;

    FGridPlayerAttackRequest Request;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::TargetCellUnavailable;
    TestTrue (
        TEXT ("A valid PlayerPhase request is accepted"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestTrue (TEXT ("The accepted request is valid"), Request.IsValid ());
    TestTrue (
        TEXT ("The request has a unique valid id"),
        Request.RequestId.IsValid ());
    TestEqual (TEXT ("Round number is captured"), Request.RoundNumber, 1);
    TestEqual (
        TEXT ("Attacker index is captured"),
        Request.AttackerCharacterIndex,
        0);
    TestEqual (
        TEXT ("CharacterId is captured"),
        Request.AttackerCharacterId,
        Attacker.CharacterId);
    TestEqual (
        TEXT ("TargetMonsterId is captured"),
        Request.TargetMonsterId,
        Fixture.FrontMonsterId);
    TestEqual (
        TEXT ("Party cell is captured"),
        Request.PartyCell,
        FIntPoint (1, 1));
    TestEqual (
        TEXT ("Target cell is captured"),
        Request.TargetCell,
        FIntPoint (1, 2));
    TestEqual (
        TEXT ("Party facing is captured"),
        Request.PartyFacing,
        EGridEdge::North);
    TestEqual (TEXT ("MON11.1 range is one cell"), Request.RangeCells, 1);
    TestEqual (
        TEXT ("MON11.1 has no weapon attack id"),
        Request.AttackId,
        FName (NAME_None));
    TestEqual (
        TEXT ("Acceptance reports no rejection"),
        RejectReason,
        EGridPlayerAttackRejectReason::None);
    TestEqual (
        TEXT ("The last request is retained transiently"),
        Fixture.TurnManager->LastPlayerAttackRequest.RequestId,
        Request.RequestId);

    TestEqual (
        TEXT ("Requesting does not change monster health"),
        Fixture.FrontMonster->CurrentHealth,
        Health);
    TestEqual (
        TEXT ("Requesting does not change physical armor"),
        Fixture.FrontMonster->CurrentPhysicalArmor,
        PhysicalArmor);
    TestEqual (
        TEXT ("Requesting does not change magical armor"),
        Fixture.FrontMonster->CurrentMagicalArmor,
        MagicalArmor);
    TestEqual (
        TEXT ("Requesting does not change monster state"),
        Fixture.FrontMonster->MonsterState,
        State);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11PerCharacterActionGateTest,
    "Grimrock.Monsters.MON11.PerCharacterActionGate",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11PerCharacterActionGateTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON11Fixture Fixture;
    if (!TestTrue (TEXT ("The action-gate fixture is ready"), Fixture.IsReady ()))
    {
        return false;
    }

    Fixture.TurnManager->PhaseState.StartCombat ();
    Fixture.TurnManager->PhaseState.BeginRound ();
    Fixture.TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
    Fixture.TurnManager->RoundNumber =
        Fixture.TurnManager->PhaseState.GetRoundNumber ();

    FGridPlayerAttackRequest Request;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    TestTrue (
        TEXT ("The first character commits its first request"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestTrue (
        TEXT ("The first character is marked for this phase"),
        Fixture.TurnManager->HasCharacterCommittedAttackThisPhase (0));

    Request = FGridPlayerAttackRequest ();
    TestFalse (
        TEXT ("The same character cannot request twice"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));
    TestEqual (
        TEXT ("The second request reports AttackerAlreadyActed"),
        RejectReason,
        EGridPlayerAttackRejectReason::AttackerAlreadyActed);
    TestFalse (
        TEXT ("A rejected request stays invalid"),
        Request.IsValid ());

    TestTrue (
        TEXT ("A second living character can request an attack"),
        Fixture.TurnManager->RequestCharacterAttack (
            1,
            Request,
            RejectReason));
    TestTrue (
        TEXT ("The second character has its independent gate"),
        Fixture.TurnManager->HasCharacterCommittedAttackThisPhase (1));

    TestTrue (
        TEXT ("The test phase state enters EnemyPhase"),
        Fixture.TurnManager->PhaseState.EndPlayerPhase ());
    Fixture.TurnManager->CurrentPhase = EGridCombatPhase::EnemyPhase;
    Fixture.TurnManager->FinishEnemyPhase ();
    TestEqual (
        TEXT ("Finishing EnemyPhase opens the next PlayerPhase"),
        Fixture.TurnManager->CurrentPhase,
        EGridCombatPhase::PlayerPhase);
    TestFalse (
        TEXT ("The first character gate resets for the next phase"),
        Fixture.TurnManager->HasCharacterCommittedAttackThisPhase (0));
    TestTrue (
        TEXT ("The first character can request in the next PlayerPhase"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            RejectReason));

    Fixture.TurnManager->ResetPlayerAttackPhaseState ();
    TestFalse (
        TEXT ("An invalid attacker request is refused"),
        Fixture.TurnManager->RequestCharacterAttack (
            INDEX_NONE,
            Request,
            RejectReason));
    TestFalse (
        TEXT ("A refused request consumes no valid character action"),
        Fixture.TurnManager->HasCharacterCommittedAttackThisPhase (1));
    TestTrue (
        TEXT ("A valid request remains possible after a refusal"),
        Fixture.TurnManager->RequestCharacterAttack (
            1,
            Request,
            RejectReason));
    return true;
}

#endif
