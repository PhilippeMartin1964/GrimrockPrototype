#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGExperienceRewardService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
    struct FRPGMON152TestWorld
    {
        UWorld* World = nullptr;

        FRPGMON152TestWorld ()
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
                    TEXT ("MON152TestWorld_%s"),
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

        ~FRPGMON152TestWorld ()
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

    FGridCharacterInventoryState MakeMON152Character (
        int32 Seed,
        int32 Level = 1,
        int32 Experience = 0)
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid (152, Seed, 0, 1);
        Character.DisplayName = FText::FromString (
            FString::Printf (TEXT ("MON15.2 Hero %d"), Seed));
        Character.Level = Level;
        Character.Experience = Experience;
        Character.Attributes = FRPGAttributes { 12, 11, 10, 9, 8, 7 };
        Character.DerivedStats.MaxHealth = 20 + Seed;
        Character.DerivedStats.CurrentHealth = 10 + Seed;
        Character.DerivedStats.MaxMana = 5;
        Character.DerivedStats.CurrentMana = 3;
        return Character;
    }

    AGrimrockPartyPawn* SpawnMON152Party (
        UWorld* World,
        int32 ActiveCharacterCount)
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

        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Reset ();
        for (int32 Index = 0; Index < ActiveCharacterCount; ++Index)
        {
            Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Add (
                MakeMON152Character (Index + 1));
        }
        return Party;
    }

    UGridMonsterDefinitionAsset* MakeMON152MonsterDefinition (
        UObject* Outer,
        FName MonsterId,
        int32 ExperienceReward)
    {
        UGridMonsterDefinitionAsset* Definition =
            NewObject<UGridMonsterDefinitionAsset> (Outer);
        Definition->MonsterId = MonsterId;
        Definition->DisplayName = FText::FromString (TEXT ("MON15.2 Test Monster"));
        Definition->CategoryId = TEXT ("MON152");
        Definition->MaxHealth = 8;
        Definition->ActionPointsPerTurn = 2;
        Definition->DeathExpectedDuration = 1.0f;
        Definition->ExperienceReward = ExperienceReward;
        return Definition;
    }

    AGridMonsterActor* SpawnMON152Monster (
        UWorld* World,
        UGridMonsterDefinitionAsset* Definition,
        FGuid SpawnId,
        FIntPoint Cell)
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
        return Monster;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON152ActivePartyDistributionTest,
    "Grimrock.RPG.MON15.2.ActivePartyDistribution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON152ActivePartyDistributionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component =
        NewObject<UGridPartyInventoryComponent> ();
    Component->PartyInventoryState.ActiveCharacters = {
        MakeMON152Character (1),
        MakeMON152Character (2),
        MakeMON152Character (3),
        MakeMON152Character (4)
    };
    Component->PartyInventoryState.CharacterPool = {
        MakeMON152Character (99, 1, 100)
    };

    int32 EventCount = 0;
    int32 EventAppliedTotal = 0;
    const FDelegateHandle EventHandle =
        FRPGExperienceRewardService::OnCharacterExperienceAwarded ().AddLambda (
            [&EventCount, &EventAppliedTotal] (
                int32 CharacterIndex,
                int32 AwardedExperience,
                int32 PreviousExperience,
                int32 NewExperience)
            {
                ++EventCount;
                EventAppliedTotal += AwardedExperience;
                check (CharacterIndex >= 0 && CharacterIndex < 4);
                check (NewExperience - PreviousExperience == AwardedExperience);
            });

    const int32 Applied =
        FRPGExperienceRewardService::AwardToActiveParty (Component, 10);

    FRPGExperienceRewardService::OnCharacterExperienceAwarded ().Remove (
        EventHandle);

    TestEqual (TEXT ("The complete reward is applied"), Applied, 10);
    TestEqual (TEXT ("Character zero receives the first remainder point"),
        Component->PartyInventoryState.ActiveCharacters[0].Experience,
        3);
    TestEqual (TEXT ("Character one receives the second remainder point"),
        Component->PartyInventoryState.ActiveCharacters[1].Experience,
        3);
    TestEqual (TEXT ("Character two receives the base share"),
        Component->PartyInventoryState.ActiveCharacters[2].Experience,
        2);
    TestEqual (TEXT ("Character three receives the base share"),
        Component->PartyInventoryState.ActiveCharacters[3].Experience,
        2);
    TestEqual (TEXT ("A reserve character receives no XP"),
        Component->PartyInventoryState.CharacterPool[0].Experience,
        100);
    TestEqual (TEXT ("One event is emitted per rewarded active character"),
        EventCount,
        4);
    TestEqual (TEXT ("Events account for the full applied reward"),
        EventAppliedTotal,
        10);

    for (const FGridCharacterInventoryState& Character :
        Component->PartyInventoryState.ActiveCharacters)
    {
        TestEqual (TEXT ("MON15.2 never changes stored Level"),
            Character.Level,
            1);
    }

    const int32 XPBeforeNoOp =
        Component->PartyInventoryState.ActiveCharacters[0].Experience;
    TestEqual (TEXT ("Zero reward is a no-op"),
        FRPGExperienceRewardService::AwardToActiveParty (Component, 0),
        0);
    TestEqual (TEXT ("Negative reward is a no-op"),
        FRPGExperienceRewardService::AwardToActiveParty (Component, -10),
        0);
    TestEqual (TEXT ("No-op rewards do not mutate XP"),
        Component->PartyInventoryState.ActiveCharacters[0].Experience,
        XPBeforeNoOp);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON152ProgressionBoundariesTest,
    "Grimrock.RPG.MON15.2.ProgressionBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON152ProgressionBoundariesTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component =
        NewObject<UGridPartyInventoryComponent> ();
    Component->PartyInventoryState.ActiveCharacters = {
        MakeMON152Character (1, 1, 999),
        MakeMON152Character (2, 19, 189999)
    };

    const FRPGDerivedStats FirstStatsBefore =
        Component->PartyInventoryState.ActiveCharacters[0].DerivedStats;
    const FRPGDerivedStats SecondStatsBefore =
        Component->PartyInventoryState.ActiveCharacters[1].DerivedStats;

    TestEqual (TEXT ("Three XP are fully shared at the boundary"),
        FRPGExperienceRewardService::AwardToActiveParty (Component, 3),
        3);
    TestEqual (TEXT ("The first character crosses the level-two XP threshold"),
        Component->PartyInventoryState.ActiveCharacters[0].Experience,
        1001);
    TestEqual (TEXT ("The second character reaches the maximum XP threshold"),
        Component->PartyInventoryState.ActiveCharacters[1].Experience,
        190000);
    TestEqual (TEXT ("The first stored Level remains pending for MON15.3"),
        Component->PartyInventoryState.ActiveCharacters[0].Level,
        1);
    TestEqual (TEXT ("The second stored Level remains unchanged"),
        Component->PartyInventoryState.ActiveCharacters[1].Level,
        19);
    TestEqual (TEXT ("The first expected level can still be reconstructed"),
        URPGCharacterRulesLibrary::GetLevelForExperience (
            Component->PartyInventoryState.ActiveCharacters[0].Experience),
        2);
    TestEqual (TEXT ("The second expected level can still be reconstructed"),
        URPGCharacterRulesLibrary::GetLevelForExperience (
            Component->PartyInventoryState.ActiveCharacters[1].Experience),
        20);

    TestEqual (TEXT ("A capped character is excluded from later sharing"),
        FRPGExperienceRewardService::AwardToActiveParty (Component, 5),
        5);
    TestEqual (TEXT ("All later XP goes to the remaining eligible active character"),
        Component->PartyInventoryState.ActiveCharacters[0].Experience,
        1006);
    TestEqual (TEXT ("The capped character remains capped"),
        Component->PartyInventoryState.ActiveCharacters[1].Experience,
        190000);

    TestEqual (TEXT ("First MaxHealth is not recalculated"),
        Component->PartyInventoryState.ActiveCharacters[0].DerivedStats.MaxHealth,
        FirstStatsBefore.MaxHealth);
    TestEqual (TEXT ("First CurrentHealth is not recalculated"),
        Component->PartyInventoryState.ActiveCharacters[0].DerivedStats.CurrentHealth,
        FirstStatsBefore.CurrentHealth);
    TestEqual (TEXT ("Second MaxHealth is not recalculated"),
        Component->PartyInventoryState.ActiveCharacters[1].DerivedStats.MaxHealth,
        SecondStatsBefore.MaxHealth);
    TestEqual (TEXT ("Second CurrentHealth is not recalculated"),
        Component->PartyInventoryState.ActiveCharacters[1].DerivedStats.CurrentHealth,
        SecondStatsBefore.CurrentHealth);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON152MonsterDeathExactlyOnceTest,
    "Grimrock.RPG.MON15.2.MonsterDeathExactlyOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON152MonsterDeathExactlyOnceTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FRPGMON152TestWorld TestWorld;
    if (!TestNotNull (TEXT ("The transient MON15.2 world exists"), TestWorld.World))
    {
        return false;
    }

    AGrimrockPartyPawn* Party =
        SpawnMON152Party (TestWorld.World, 2);
    if (!TestNotNull (TEXT ("The MON15.2 party exists"), Party))
    {
        return false;
    }

    UGridMonsterDefinitionAsset* Definition =
        MakeMON152MonsterDefinition (
            TestWorld.World,
            TEXT ("MON152_RewardMonster"),
            10);
    AGridMonsterActor* Monster =
        SpawnMON152Monster (
            TestWorld.World,
            Definition,
            FGuid (152, 10, 0, 1),
            FIntPoint (1, 1));
    if (!TestNotNull (TEXT ("The reward monster exists"), Monster) ||
        !Monster->DeathComponent)
    {
        return false;
    }

    Monster->MarkDead ();
    TestEqual (TEXT ("First active character receives five XP"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        5);
    TestEqual (TEXT ("Second active character receives five XP"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[1].Experience,
        5);

    Monster->MarkDead ();
    TestEqual (TEXT ("Repeated MarkDead does not award XP again to character zero"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        5);
    TestEqual (TEXT ("Repeated MarkDead does not award XP again to character one"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[1].Experience,
        5);

    AGridMonsterActor* RestoredDeadMonster =
        SpawnMON152Monster (
            TestWorld.World,
            Definition,
            FGuid (152, 10, 0, 2),
            FIntPoint (2, 1));
    if (!TestNotNull (TEXT ("The restored-dead monster exists"), RestoredDeadMonster) ||
        !RestoredDeadMonster->DeathComponent)
    {
        return false;
    }

    RestoredDeadMonster->DeathComponent->RestoreCommittedDeathState (
        FIntPoint (2, 1),
        false);
    RestoredDeadMonster->MarkDead ();
    TestEqual (TEXT ("A restored committed death does not replay XP for character zero"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        5);
    TestEqual (TEXT ("A restored committed death does not replay XP for character one"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[1].Experience,
        5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON152LootIndependenceTest,
    "Grimrock.RPG.MON15.2.LootIndependence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON152LootIndependenceTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FRPGMON152TestWorld TestWorld;
    if (!TestNotNull (TEXT ("The loot-independence world exists"), TestWorld.World))
    {
        return false;
    }

    AGrimrockPartyPawn* Party =
        SpawnMON152Party (TestWorld.World, 1);
    UGridMonsterDefinitionAsset* Definition =
        MakeMON152MonsterDefinition (
            TestWorld.World,
            TEXT ("MON152_LootFailureMonster"),
            7);
    FGridMonsterLootEntry GuaranteedLoot;
    GuaranteedLoot.ItemDefinitionId = TEXT ("MON152_MissingRuntimeLoot" );
    GuaranteedLoot.DropChance = 1.0f;
    GuaranteedLoot.MinQuantity = 1;
    GuaranteedLoot.MaxQuantity = 1;
    Definition->LootTable.Add (GuaranteedLoot);

    AGridMonsterActor* Monster =
        SpawnMON152Monster (
            TestWorld.World,
            Definition,
            FGuid (152, 20, 0, 1),
            FIntPoint (1, 1));
    if (!TestNotNull (TEXT ("The loot-independence party exists"), Party) ||
        !TestNotNull (TEXT ("The loot-independence monster exists"), Monster) ||
        !Monster->DeathComponent)
    {
        return false;
    }

    Monster->MarkDead ();
    TestEqual (TEXT ("The guaranteed loot placement fails without a runtime actor"),
        Monster->DeathComponent->FailedLootCount,
        1);
    TestEqual (TEXT ("XP is still awarded when loot placement fails"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON152PersistenceStateTest,
    "Grimrock.RPG.MON15.2.PersistenceState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON152PersistenceStateTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* SourceComponent =
        NewObject<UGridPartyInventoryComponent> ();
    SourceComponent->PartyInventoryState.ActiveCharacters = {
        MakeMON152Character (1, 1, 0)
    };
    SourceComponent->PartyInventoryState.ActiveEquipment.SetNum (1);
    SourceComponent->PartyInventoryState.SelectedCharacterIndex = 0;
    SourceComponent->PartyInventoryState.MaxActiveCharacters = 6;
    SourceComponent->PartyInventoryState.bInitialCharacterCreationCompleted = true;

    TestEqual (TEXT ("The source party receives exactly 1000 cumulative XP"),
        FRPGExperienceRewardService::AwardToActiveParty (SourceComponent, 1000),
        1000);
    TestEqual (TEXT ("The source character XP is persisted in the existing field"),
        SourceComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        1000);
    TestEqual (TEXT ("MON15.2 leaves the source Level unchanged"),
        SourceComponent->PartyInventoryState.ActiveCharacters[0].Level,
        1);

    UGrimrockPartySaveGame* SaveGame =
        NewObject<UGrimrockPartySaveGame> ();
    SaveGame->PartyInventoryState = SourceComponent->PartyInventoryState;
    TestEqual (TEXT ("New save containers use the current save version"),
        SaveGame->SaveVersion,
        UGrimrockPartySaveGame::CurrentSaveVersion);

    UGridPartyInventoryComponent* RestoredComponent =
        NewObject<UGridPartyInventoryComponent> ();
    FText RestoreError;
    TestTrue (TEXT ("The existing party save state restores"),
        RestoredComponent->RestorePartyInventoryState (
            SaveGame->PartyInventoryState,
            RestoreError));
    TestTrue (TEXT ("No restore error is produced"), RestoreError.IsEmpty ());
    TestEqual (TEXT ("Restored cumulative XP is exact"),
        RestoredComponent->PartyInventoryState.ActiveCharacters[0].Experience,
        1000);
    TestEqual (TEXT ("Restored Level remains the stored pre-MON15.3 value"),
        RestoredComponent->PartyInventoryState.ActiveCharacters[0].Level,
        1);
    TestEqual (TEXT ("The expected pending level remains reconstructible"),
        URPGCharacterRulesLibrary::GetLevelForExperience (
            RestoredComponent->PartyInventoryState.ActiveCharacters[0].Experience),
        2);
    return true;
}

#endif
