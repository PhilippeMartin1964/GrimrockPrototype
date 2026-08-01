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
                .ActiveEquipment.SetNum (2);
            Party->PartyInventoryComponent->PartyInventoryState
                .SelectedCharacterIndex = 0;

            MonsterDefinition =
                NewObject<UGridMonsterDefinitionAsset> (Runtime);
            MonsterDefinition->MonsterId = TEXT ("MON11_Rat");
            MonsterDefinition->DisplayName =
                FText::FromString (TEXT ("Rat de test MON11"));
            MonsterDefinition->CategoryId = TEXT ("Vermin");
            MonsterDefinition->MaxHealth = 100;
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
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    TestTrue (
        TEXT ("The exact occupant in the facing cell is targeted"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
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

    Fixture.TurnManager->BeginPlayerCharacterPhase ();
    Fixture.LevelAsset->GetCellMutable (1, 1).NorthWall =
        EGridWallType::Solid;
    Request = FGridPlayerAttackRequest ();
    TestFalse (
        TEXT ("A solid wall blocks the attack request"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
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
            Result,
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
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    Fixture.TurnManager->bCombatActive = false;
    Fixture.TurnManager->CurrentPhase = EGridCombatPhase::Exploration;
    TestFalse (
        TEXT ("Exploration rejects a player attack"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
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
            Result,
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
            Result,
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
            Result,
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
            Result,
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
            Result,
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
            Result,
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
            Result,
            RejectReason));
    TestEqual (
        TEXT ("An inactive-level target reports TargetInactive"),
        RejectReason,
        EGridPlayerAttackRejectReason::TargetInactive);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11RequestAcceptedAndResolvedTest,
    "Grimrock.Monsters.MON11.RequestAcceptedAndResolved",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11RequestAcceptedAndResolvedTest::RunTest (
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
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::TargetCellUnavailable;
    TestTrue (
        TEXT ("A valid PlayerPhase request is accepted"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
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
    TestEqual (TEXT ("MON11.2 range is one cell"), Request.RangeCells, 1);
    TestEqual (
        TEXT ("MON11.2 uses the provisional unarmed attack"),
        Request.AttackId,
        FName (TEXT ("Attack_Unarmed")));
    TestTrue (
        TEXT ("An unarmed request has no offensive item"),
        Request.OffensiveItemDefinitionId.IsNone ());
    TestEqual (
        TEXT ("An unarmed request has no equipment slot"),
        Request.OffensiveEquipmentSlot,
        EGridEquipmentSlot::None);
    TestEqual (
        TEXT ("Acceptance reports no rejection"),
        RejectReason,
        EGridPlayerAttackRejectReason::None);
    TestEqual (
        TEXT ("The last request is retained transiently"),
        Fixture.TurnManager->LastPlayerAttackRequest.RequestId,
        Request.RequestId);
    TestTrue (
        TEXT ("The result contains a natural attack roll"),
        Result.NaturalAttackRoll >= 1 &&
            Result.NaturalAttackRoll <= 20);
    TestEqual (
        TEXT ("The result captures target health before resolution"),
        Result.TargetHealthBefore,
        Health);
    TestEqual (
        TEXT ("The last result retains the natural roll"),
        Fixture.TurnManager->LastPlayerAttackResult.NaturalAttackRoll,
        Result.NaturalAttackRoll);
    TestEqual (
        TEXT ("The last result retains raw damage"),
        Fixture.TurnManager->LastPlayerAttackResult.RawDamage,
        Result.RawDamage);
    TestTrue (
        TEXT ("The accepted attack consumes the character action"),
        Fixture.TurnManager->HasCharacterCommittedAttackThisPhase (0));
    TestEqual (
        TEXT ("Exactly one player resolution is broadcast"),
        Fixture.TurnManager->PlayerAttackResolvedBroadcastCount,
        1);
    TestEqual (
        TEXT ("Physical armor matches the applied result"),
        Fixture.FrontMonster->CurrentPhysicalArmor,
        PhysicalArmor - Result.PhysicalArmorDamage);
    TestEqual (
        TEXT ("Magical armor matches the applied result"),
        Fixture.FrontMonster->CurrentMagicalArmor,
        MagicalArmor - Result.MagicalArmorDamage);
    TestEqual (
        TEXT ("Health matches the applied result"),
        Fixture.FrontMonster->CurrentHealth,
        Health - Result.HealthDamage);
    if (!Result.bHit)
    {
        TestEqual (
            TEXT ("A miss leaves the monster state unchanged"),
            Fixture.FrontMonster->MonsterState,
            State);
    }
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
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    TestTrue (
        TEXT ("The first character commits its first request"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestTrue (
        TEXT ("The first character has spent action points"),
        Fixture.TurnManager->HasCharacterCommittedAttackThisPhase (0));

    Request = FGridPlayerAttackRequest ();
    TestTrue (
        TEXT ("The same character can spend its remaining two AP"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));

    Request = FGridPlayerAttackRequest ();
    TestFalse (
        TEXT ("The same character cannot request a third attack"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestEqual (
        TEXT ("The third request reports InsufficientActionPoints"),
        RejectReason,
        EGridPlayerAttackRejectReason::InsufficientActionPoints);
    TestFalse (
        TEXT ("A rejected request stays invalid"),
        Request.IsValid ());

    TestTrue (
        TEXT ("A second living character can request an attack"),
        Fixture.TurnManager->RequestCharacterAttack (
            1,
            Request,
            Result,
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
        TEXT ("The first character PA budget resets for the next phase"),
        Fixture.TurnManager->HasCharacterCommittedAttackThisPhase (0));
    TestTrue (
        TEXT ("The first character can request in the next PlayerPhase"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));

    Fixture.TurnManager->BeginPlayerCharacterPhase ();
    TestFalse (
        TEXT ("An invalid attacker request is refused"),
        Fixture.TurnManager->RequestCharacterAttack (
            INDEX_NONE,
            Request,
            Result,
            RejectReason));
    TestFalse (
        TEXT ("A refused request consumes no valid character action"),
        Fixture.TurnManager->HasCharacterCommittedAttackThisPhase (1));
    TestTrue (
        TEXT ("A valid request remains possible after a refusal"),
        Fixture.TurnManager->RequestCharacterAttack (
            1,
            Request,
            Result,
            RejectReason));
    return true;
}

#endif
