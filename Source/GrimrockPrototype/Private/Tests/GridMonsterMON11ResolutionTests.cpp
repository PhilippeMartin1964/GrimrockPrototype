#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
    FGridOffensiveEquipmentProfile MakeMON11UnarmedProfile ()
    {
        FGridOffensiveEquipmentProfile Profile;
        Profile.AttackId = TEXT ("Attack_Unarmed");
        Profile.AttackDefinition.DamageType =
            EGridDamageType::Physical;
        Profile.AttackDefinition.PhysicalSubtype =
            EGridPhysicalDamageSubtype::Bludgeoning;
        Profile.AttackDefinition.MinDamage = 1;
        Profile.AttackDefinition.MaxDamage = 3;
        Profile.DamageScalingAttribute =
            EGridAttackScalingAttribute::Strength;
        Profile.RangeCells = 1;
        return Profile;
    }

    struct FGridMON11ResolutionWorld
    {
        UWorld* World = nullptr;

        FGridMON11ResolutionWorld ()
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
                    TEXT ("MON11ResolutionWorld_%s"),
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

        ~FGridMON11ResolutionWorld ()
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

    struct FGridMON11ResolutionFixture
    {
        FGridMON11ResolutionWorld TestWorld;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* LevelAsset = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
        UGridMonsterDefinitionAsset* MonsterDefinition = nullptr;
        AGridMonsterActor* Monster = nullptr;
        UGridMonsterOccupancySubsystem* Occupancy = nullptr;
        UGridTurnManagerComponent* TurnManager = nullptr;
        FGuid CharacterId = FGuid (112, 1, 0, 1);
        FGuid MonsterId = FGuid (112, 2, 0, 1);

        explicit FGridMON11ResolutionFixture (
            int32 MonsterHealth = 40,
            int32 PhysicalArmor = 4,
            int32 MagicalArmor = 3)
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
            Runtime->CurrentDungeonLevelId = TEXT ("MON11_TestLevel");

            Party =
                TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 1;
            Party->CurrentCellY = 1;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation (Runtime->GetCellCenterWorld (
                1,
                1,
                Party->EyeHeight));
            Party->SetActorRotation (FRotator (
                0.0f,
                GridDirectionUtils::ToYaw (Party->Facing),
                0.0f));

            FGridCharacterInventoryState Character;
            Character.CharacterId = CharacterId;
            Character.DisplayName =
                FText::FromString (TEXT ("Elias"));
            Character.Attributes.Strength = 16;
            Character.DerivedStats.MaxHealth = 20;
            Character.DerivedStats.CurrentHealth = 20;
            Character.DerivedStats.Accuracy = 7;
            Party->PartyInventoryComponent->PartyInventoryState
                .ActiveCharacters = { Character };
            Party->PartyInventoryComponent->PartyInventoryState
                .ActiveEquipment.SetNum (1);
            Party->PartyInventoryComponent->PartyInventoryState
                .SelectedCharacterIndex = 0;

            MonsterDefinition =
                NewObject<UGridMonsterDefinitionAsset> (Runtime);
            MonsterDefinition->MonsterId = TEXT ("MON11_ResolutionRat");
            MonsterDefinition->DisplayName =
                FText::FromString (TEXT ("Rat de résolution"));
            MonsterDefinition->CategoryId = TEXT ("Vermin");
            MonsterDefinition->MaxHealth = MonsterHealth;
            MonsterDefinition->PhysicalArmor = PhysicalArmor;
            MonsterDefinition->MagicalArmor = MagicalArmor;
            MonsterDefinition->Evasion = 4;
            MonsterDefinition->ActionPointsPerTurn = 2;

            FGridMonsterDamageModifier BludgeoningModifier;
            BludgeoningModifier.DamageType =
                EGridDamageType::Physical;
            BludgeoningModifier.PhysicalSubtype =
                EGridPhysicalDamageSubtype::Bludgeoning;
            BludgeoningModifier.DamageMultiplier = 1.5f;
            MonsterDefinition->DamageModifiers.Add (
                BludgeoningModifier);

            Monster =
                TestWorld.World->SpawnActor<AGridMonsterActor> ();
            Monster->InitializeMonster (
                MonsterDefinition,
                MonsterId,
                FIntPoint (1, 2),
                EGridEdge::South);

            Occupancy = TestWorld.World
                ->GetSubsystem<UGridMonsterOccupancySubsystem> ();
            if (Occupancy)
            {
                Occupancy->RegisterMonster (
                    Monster,
                    Monster->CurrentCell);
            }

            TurnManager = NewObject<UGridTurnManagerComponent> (
                Runtime,
                TEXT ("MON11ResolutionTurnManager"));
            TurnManager->bAutoInitialize = false;
            Runtime->AddInstanceComponent (TurnManager);
            TurnManager->RegisterComponent ();
            TurnManager->InitializeTurnManager (Runtime, Party);
            TurnManager->bCombatActive = true;
            TurnManager->CurrentPhase =
                EGridCombatPhase::PlayerPhase;
            TurnManager->RoundNumber = 1;
            TurnManager->CombatMonsters = { Monster };
        }

        bool IsReady () const
        {
            return TestWorld.World &&
                Runtime &&
                LevelAsset &&
                Party &&
                Party->PartyInventoryComponent &&
                MonsterDefinition &&
                Monster &&
                Occupancy &&
                TurnManager;
        }
    };

    int32 FindSeedForNaturalAttackRoll (int32 DesiredRoll)
    {
        for (int32 Seed = 1; Seed < 100000; ++Seed)
        {
            FRandomStream Candidate (Seed);
            if (Candidate.RandRange (1, 20) == DesiredRoll)
            {
                return Seed;
            }
        }
        return INDEX_NONE;
    }

    bool AreEquivalentAttackResults (
        const FGridAttackResult& Left,
        const FGridAttackResult& Right)
    {
        return Left.bHit == Right.bHit &&
            Left.bCriticalHit == Right.bCriticalHit &&
            Left.NaturalAttackRoll == Right.NaturalAttackRoll &&
            Left.AttackRoll == Right.AttackRoll &&
            Left.DefenseValue == Right.DefenseValue &&
            Left.RawDamage == Right.RawDamage &&
            Left.ResistancePercent == Right.ResistancePercent &&
            FMath::IsNearlyEqual (
                Left.DamageMultiplier,
                Right.DamageMultiplier) &&
            Left.DamageAfterModifiers ==
                Right.DamageAfterModifiers &&
            Left.PhysicalArmorDamage ==
                Right.PhysicalArmorDamage &&
            Left.MagicalArmorDamage ==
                Right.MagicalArmorDamage &&
            Left.HealthDamage == Right.HealthDamage &&
            Left.TargetHealthBefore == Right.TargetHealthBefore &&
            Left.TargetHealthAfter == Right.TargetHealthAfter &&
            Left.DamageType == Right.DamageType &&
            Left.PhysicalSubtype == Right.PhysicalSubtype;
    }

    int32 CountCombatLogEntries (
        const UGridTurnManagerComponent* TurnManager,
        EGridCombatLogEntryType Type)
    {
        int32 Count = 0;
        if (TurnManager)
        {
            for (const FGridCombatLogEntry& Entry :
                TurnManager->CombatLogEntries)
            {
                Count += Entry.Type == Type ? 1 : 0;
            }
        }
        return Count;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11PlayerResolutionMappingTest,
    "Grimrock.Monsters.MON11.PlayerResolutionMapping",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11PlayerResolutionMappingTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON11ResolutionFixture Fixture;
    if (!TestTrue (TEXT ("The mapping fixture is ready"), Fixture.IsReady ()))
    {
        return false;
    }

    FGridInventoryCharacterSummary Summary;
    TestTrue (
        TEXT ("The final character summary is available"),
        Fixture.Party->PartyInventoryComponent->GetCharacterSummary (
            0,
            Summary));

    FGridAttackSourceStats Source;
    FGridAttackTargetStats Target;
    FGridAttackDefinition Attack;
    const FGridOffensiveEquipmentProfile UnarmedProfile =
        MakeMON11UnarmedProfile ();
    TestTrue (
        TEXT ("The provisional resolution inputs are built"),
        Fixture.TurnManager->BuildPlayerAttackResolutionInputs (
            Summary,
            Fixture.Monster,
            UnarmedProfile,
            Source,
            Target,
            Attack));
    TestEqual (
        TEXT ("Unarmed damage is physical"),
        Attack.DamageType,
        EGridDamageType::Physical);
    TestEqual (
        TEXT ("Unarmed damage is bludgeoning"),
        Attack.PhysicalSubtype,
        EGridPhysicalDamageSubtype::Bludgeoning);
    TestEqual (TEXT ("Unarmed minimum damage is one"), Attack.MinDamage, 1);
    TestEqual (TEXT ("Unarmed maximum damage is three"), Attack.MaxDamage, 3);
    TestEqual (
        TEXT ("Accuracy comes from final derived stats"),
        Source.Accuracy,
        Summary.DerivedStats.Accuracy);
    TestEqual (
        TEXT ("Damage bonus comes from final Strength"),
        Source.DamageBonus,
        URPGCharacterRulesLibrary::GetAttributeModifier (
            Summary.Attributes.Strength));
    TestEqual (
        TEXT ("Defense uses monster definition Evasion"),
        10 + Target.Evasion,
        10 + Fixture.MonsterDefinition->Evasion);
    TestEqual (
        TEXT ("Target health comes from runtime state"),
        Target.CurrentHealth,
        Fixture.Monster->CurrentHealth);
    TestEqual (
        TEXT ("Physical armor comes from runtime state"),
        Target.PhysicalArmor,
        Fixture.Monster->CurrentPhysicalArmor);
    TestEqual (
        TEXT ("Magical armor comes from runtime state"),
        Target.MagicalArmor,
        Fixture.Monster->CurrentMagicalArmor);
    TestEqual (
        TEXT ("Bludgeoning multiplier comes from the definition"),
        Target.DamageMultiplier,
        Fixture.MonsterDefinition->GetDamageMultiplier (
            EGridDamageType::Physical,
            EGridPhysicalDamageSubtype::Bludgeoning));

    Fixture.TurnManager->CombatRandomStream.Initialize (12345);
    FGridPlayerAttackRequest Request;
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    TestTrue (
        TEXT ("The mapped unarmed request resolves"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestEqual (
        TEXT ("The accepted request carries Attack_Unarmed"),
        Request.AttackId,
        FName (TEXT ("Attack_Unarmed")));
    TestTrue (
        TEXT ("The unarmed request carries no item identity"),
        Request.OffensiveItemDefinitionId.IsNone ());
    TestEqual (
        TEXT ("The unarmed request carries no equipment slot"),
        Request.OffensiveEquipmentSlot,
        EGridEquipmentSlot::None);
    TestEqual (
        TEXT ("The resolved defense value is mapped exactly"),
        Result.DefenseValue,
        10 + Fixture.MonsterDefinition->Evasion);
    TestEqual (
        TEXT ("The result retains the mapped multiplier"),
        Result.DamageMultiplier,
        Target.DamageMultiplier);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11PlayerResolutionDeterminismTest,
    "Grimrock.Monsters.MON11.PlayerResolutionDeterminism",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11PlayerResolutionDeterminismTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON11ResolutionFixture FirstFixture;
    FGridMON11ResolutionFixture SecondFixture;
    if (!TestTrue (
        TEXT ("Both deterministic fixtures are ready"),
        FirstFixture.IsReady () && SecondFixture.IsReady ()))
    {
        return false;
    }

    constexpr int32 EncounterSeed = 271828;
    FirstFixture.TurnManager->CombatRandomStream.Initialize (
        EncounterSeed);
    SecondFixture.TurnManager->CombatRandomStream.Initialize (
        EncounterSeed);
    FirstFixture.TurnManager->ActiveEncounterRandomSeed =
        EncounterSeed;
    SecondFixture.TurnManager->ActiveEncounterRandomSeed =
        EncounterSeed;
    FirstFixture.TurnManager->bCollectRuntimeMetrics = true;
    SecondFixture.TurnManager->bCollectRuntimeMetrics = true;

    FGridPlayerAttackRequest FirstRequest;
    FGridPlayerAttackRequest SecondRequest;
    FGridAttackResult FirstResult;
    FGridAttackResult SecondResult;
    EGridPlayerAttackRejectReason FirstReject =
        EGridPlayerAttackRejectReason::None;
    EGridPlayerAttackRejectReason SecondReject =
        EGridPlayerAttackRejectReason::None;
    TestTrue (
        TEXT ("The first equivalent attack resolves"),
        FirstFixture.TurnManager->RequestCharacterAttack (
            0,
            FirstRequest,
            FirstResult,
            FirstReject));
    TestTrue (
        TEXT ("The second equivalent attack resolves"),
        SecondFixture.TurnManager->RequestCharacterAttack (
            0,
            SecondRequest,
            SecondResult,
            SecondReject));
    TestTrue (
        TEXT ("Equivalent encounters produce identical results"),
        AreEquivalentAttackResults (FirstResult, SecondResult));
    TestNotEqual (
        TEXT ("Request identity is deliberately not deterministic"),
        FirstRequest.RequestId,
        SecondRequest.RequestId);

    const int32 StreamAfterSuccess =
        FirstFixture.TurnManager->CombatRandomStream.GetCurrentSeed ();
    TestNotEqual (
        TEXT ("A resolved attack advances the encounter stream"),
        StreamAfterSuccess,
        EncounterSeed);
    FGridPlayerAttackRequest RejectedRequest;
    FGridAttackResult RejectedResult;
    TestFalse (
        TEXT ("A second attack by the same character is refused"),
        FirstFixture.TurnManager->RequestCharacterAttack (
            0,
            RejectedRequest,
            RejectedResult,
            FirstReject));
    TestEqual (
        TEXT ("The refusal reports AttackerAlreadyActed"),
        FirstReject,
        EGridPlayerAttackRejectReason::AttackerAlreadyActed);
    TestEqual (
        TEXT ("The refusal consumes no random draw"),
        FirstFixture.TurnManager->CombatRandomStream.GetCurrentSeed (),
        StreamAfterSuccess);

    FirstFixture.TurnManager->ResetPlayerAttackPhaseState ();
    TestTrue (
        TEXT ("A later valid resolution continues the same stream"),
        FirstFixture.TurnManager->RequestCharacterAttack (
            0,
            FirstRequest,
            FirstResult,
            FirstReject));
    TestNotEqual (
        TEXT ("The later resolution advances beyond the first result"),
        FirstFixture.TurnManager->CombatRandomStream.GetCurrentSeed (),
        StreamAfterSuccess);
    TestEqual (
        TEXT ("Exactly one resolved broadcast occurs per success"),
        FirstFixture.TurnManager->PlayerAttackResolvedBroadcastCount,
        2);
    TestEqual (
        TEXT ("Metrics count successful resolutions only"),
        FirstFixture.TurnManager->RuntimeMetrics.AttacksResolved,
        2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11PlayerResolutionArmorAndCriticalTest,
    "Grimrock.Monsters.MON11.PlayerResolutionArmorAndCritical",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11PlayerResolutionArmorAndCriticalTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON11ResolutionFixture Fixture (30, 4, 6);
    if (!TestTrue (
        TEXT ("The armor and critical fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    const int32 NaturalOneSeed =
        FindSeedForNaturalAttackRoll (1);
    const int32 NaturalTwentySeed =
        FindSeedForNaturalAttackRoll (20);
    TestTrue (
        TEXT ("A natural-one seed is found programmatically"),
        NaturalOneSeed != INDEX_NONE);
    TestTrue (
        TEXT ("A natural-twenty seed is found programmatically"),
        NaturalTwentySeed != INDEX_NONE);
    if (NaturalOneSeed == INDEX_NONE ||
        NaturalTwentySeed == INDEX_NONE)
    {
        return false;
    }

    const int32 InitialHealth = Fixture.Monster->CurrentHealth;
    const int32 InitialPhysicalArmor =
        Fixture.Monster->CurrentPhysicalArmor;
    const int32 InitialMagicalArmor =
        Fixture.Monster->CurrentMagicalArmor;
    FGridPlayerAttackRequest Request;
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;

    Fixture.TurnManager->CombatRandomStream.Initialize (
        NaturalOneSeed);
    TestTrue (
        TEXT ("A natural-one attack is still a resolved command"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestEqual (
        TEXT ("The first programmed roll is one"),
        Result.NaturalAttackRoll,
        1);
    TestFalse (TEXT ("Natural one always misses"), Result.bHit);
    TestEqual (
        TEXT ("A miss leaves physical armor unchanged"),
        Fixture.Monster->CurrentPhysicalArmor,
        InitialPhysicalArmor);
    TestEqual (
        TEXT ("A miss leaves magical armor unchanged"),
        Fixture.Monster->CurrentMagicalArmor,
        InitialMagicalArmor);
    TestEqual (
        TEXT ("A miss leaves health unchanged"),
        Fixture.Monster->CurrentHealth,
        InitialHealth);

    Fixture.TurnManager->ResetPlayerAttackPhaseState ();
    Fixture.TurnManager->CombatRandomStream.Initialize (
        NaturalTwentySeed);
    TestTrue (
        TEXT ("A natural-twenty attack resolves"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestEqual (
        TEXT ("The programmed critical roll is twenty"),
        Result.NaturalAttackRoll,
        20);
    TestTrue (TEXT ("Natural twenty hits"), Result.bHit);
    TestTrue (
        TEXT ("Natural twenty is critical"),
        Result.bCriticalHit);
    const int32 StrengthBonus =
        URPGCharacterRulesLibrary::GetAttributeModifier (16);
    TestTrue (
        TEXT ("Critical raw damage is twice a valid base roll"),
        Result.RawDamage >= (1 + StrengthBonus) * 2 &&
            Result.RawDamage <= (3 + StrengthBonus) * 2 &&
            Result.RawDamage % 2 == 0);
    TestEqual (
        TEXT ("Physical armor absorbs damage first"),
        Fixture.Monster->CurrentPhysicalArmor,
        InitialPhysicalArmor - Result.PhysicalArmorDamage);
    TestEqual (
        TEXT ("Magical armor is untouched by physical damage"),
        Fixture.Monster->CurrentMagicalArmor,
        InitialMagicalArmor);
    TestEqual (
        TEXT ("Only the remaining damage reaches health"),
        Fixture.Monster->CurrentHealth,
        InitialHealth - Result.HealthDamage);
    TestEqual (
        TEXT ("Runtime health matches the result after value"),
        Fixture.Monster->CurrentHealth,
        Result.TargetHealthAfter);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON11PlayerResolutionDeathVictoryTest,
    "Grimrock.Monsters.MON11.PlayerResolutionDeathVictory",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11PlayerResolutionDeathVictoryTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON11ResolutionFixture Fixture (1, 0, 0);
    if (!TestTrue (
        TEXT ("The death and victory fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    Fixture.TurnManager->PhaseState.StartCombat ();
    Fixture.TurnManager->PhaseState.BeginRound ();
    Fixture.TurnManager->CurrentPhase =
        EGridCombatPhase::PlayerPhase;
    Fixture.TurnManager->RoundNumber =
        Fixture.TurnManager->PhaseState.GetRoundNumber ();
    Fixture.TurnManager->bCombatActive = true;
    Fixture.TurnManager->BindCombatMonsterDeaths ();
    TestTrue (
        TEXT ("The death component initializes against the runtime"),
        Fixture.Monster->DeathComponent->InitializeDeathComponent (
            Fixture.Runtime));
    Fixture.TurnManager->ClearCombatLog ();

    const int32 NaturalTwentySeed =
        FindSeedForNaturalAttackRoll (20);
    Fixture.TurnManager->CombatRandomStream.Initialize (
        NaturalTwentySeed);
    FGridPlayerAttackRequest Request;
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    TestTrue (
        TEXT ("The lethal player attack resolves"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestTrue (TEXT ("The lethal result is a hit"), Result.bHit);
    TestEqual (
        TEXT ("The lethal attack applies health damage once"),
        Fixture.Monster->CurrentHealth,
        0);
    TestTrue (
        TEXT ("The monster is logically dead"),
        Fixture.Monster->IsDead ());
    TestEqual (
        TEXT ("The logical death event occurs exactly once"),
        Fixture.Monster->DeathComponent->LogicalDeathEventCount,
        1);
    TestEqual (
        TEXT ("MonsterDefeated is logged exactly once"),
        CountCombatLogEntries (
            Fixture.TurnManager,
            EGridCombatLogEntryType::MonsterDefeated),
        1);
    TestEqual (
        TEXT ("The player result is broadcast exactly once"),
        Fixture.TurnManager->PlayerAttackResolvedBroadcastCount,
        1);
    TestEqual (
        TEXT ("The combat reaches Victory"),
        Fixture.TurnManager->CurrentPhase,
        EGridCombatPhase::Victory);
    TestFalse (
        TEXT ("Combat is inactive after Victory"),
        Fixture.TurnManager->bCombatActive);
    TestEqual (
        TEXT ("Victory is logged exactly once"),
        CountCombatLogEntries (
            Fixture.TurnManager,
            EGridCombatLogEntryType::Victory),
        1);

    TArray<EGridCombatLogEntryType> RelevantOrder;
    for (const FGridCombatLogEntry& Entry :
        Fixture.TurnManager->CombatLogEntries)
    {
        if (Entry.Type == EGridCombatLogEntryType::AttackHit ||
            Entry.Type == EGridCombatLogEntryType::MonsterDefeated ||
            Entry.Type == EGridCombatLogEntryType::Victory)
        {
            RelevantOrder.Add (Entry.Type);
        }
    }
    TestEqual (
        TEXT ("The lethal log has three relevant entries"),
        RelevantOrder.Num (),
        3);
    if (RelevantOrder.Num () == 3)
    {
        TestEqual (
            TEXT ("AttackHit is logged first"),
            RelevantOrder[0],
            EGridCombatLogEntryType::AttackHit);
        TestEqual (
            TEXT ("MonsterDefeated is logged second"),
            RelevantOrder[1],
            EGridCombatLogEntryType::MonsterDefeated);
        TestEqual (
            TEXT ("Victory is logged last"),
            RelevantOrder[2],
            EGridCombatLogEntryType::Victory);
    }

    FGridPlayerAttackRequest RejectedRequest;
    FGridAttackResult RejectedResult;
    TestFalse (
        TEXT ("A new attack after Victory is refused"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            RejectedRequest,
            RejectedResult,
            RejectReason));
    TestEqual (
        TEXT ("The post-victory refusal reports CombatInactive"),
        RejectReason,
        EGridPlayerAttackRejectReason::CombatInactive);
    TestEqual (
        TEXT ("The post-victory refusal does not rebroadcast"),
        Fixture.TurnManager->PlayerAttackResolvedBroadcastCount,
        1);
    return true;
}

#endif
