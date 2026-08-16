#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectInitiativeResolver.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"

namespace
{
    UGridStatusEffectDefinitionAsset* MON164MakeDefinition (
        UObject* Outer,
        FName EffectId,
        int32 InitiativeModifier,
        EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds,
        int32 DefaultDuration = 3,
        EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack,
        int32 MaxStacks = 1)
    {
        UGridStatusEffectDefinitionAsset* Definition =
            NewObject<UGridStatusEffectDefinitionAsset> (Outer);
        Definition->EffectId = EffectId;
        Definition->DisplayName = FText::FromName (EffectId);
        Definition->Disposition = InitiativeModifier >= 0
            ? EGridStatusEffectDisposition::Buff
            : EGridStatusEffectDisposition::Debuff;
        Definition->DurationUnit = DurationUnit;
        Definition->DefaultDuration = DefaultDuration;
        Definition->StackPolicy = StackPolicy;
        Definition->MaxStacks = MaxStacks;
        Definition->InitiativeModifier = InitiativeModifier;
        return Definition;
    }

    FGridCharacterInventoryState MON164MakeCharacter (
        const FGuid& CharacterId,
        const TCHAR* Name)
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = CharacterId;
        Character.DisplayName = FText::FromString (Name);
        Character.DerivedStats.CurrentHealth = 20;
        Character.DerivedStats.MaxHealth = 20;
        return Character;
    }

    FGridCombatantInitiativeEntry MON164MakeEntry (
        const FGuid& CombatantId,
        EGridCombatantSide Side,
        int32 CharacterIndex,
        int32 InitiativeTotal,
        EGridCombatantTurnState State)
    {
        FGridCombatantInitiativeEntry Entry;
        Entry.CombatantId = CombatantId;
        Entry.Side = Side;
        Entry.CharacterIndex = CharacterIndex;
        Entry.DisplayName = FText::FromString (TEXT ("MON16.4"));
        Entry.InitiativeRoll = 7;
        Entry.InitiativeBase = InitiativeTotal - Entry.InitiativeRoll;
        Entry.InitiativeTotal = InitiativeTotal;
        Entry.CurrentHealth = 20;
        Entry.MaximumHealth = 20;
        Entry.State = State;
        return Entry;
    }

    const FGridCombatantInitiativeEntry* MON164FindEntry (
        const UGridTurnManagerComponent* TurnManager,
        const FGuid& CombatantId)
    {
        return IsValid (TurnManager)
            ? TurnManager->InitiativeOrder.FindByPredicate (
                [&CombatantId] (const FGridCombatantInitiativeEntry& Entry)
                {
                    return Entry.CombatantId == CombatantId;
                })
            : nullptr;
    }

    struct FGridMON164TestWorld
    {
        UWorld* World = nullptr;

        FGridMON164TestWorld ()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues ()
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
                    TEXT ("MON164TestWorld_%s"),
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

        ~FGridMON164TestWorld ()
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

    struct FGridMON164Fixture
    {
        FGridMON164TestWorld TestWorld;
        AGrimrockPartyPawn* Party = nullptr;
        AActor* Owner = nullptr;
        AGridMonsterActor* Monster = nullptr;
        UGridTurnManagerComponent* TurnManager = nullptr;
        UGridStatusEffectLifecycleSubsystem* Lifecycle = nullptr;
        FGuid CharacterIds[3] = {
            FGuid (16, 4, 1, 1),
            FGuid (16, 4, 1, 2),
            FGuid (16, 4, 1, 3)
        };
        FGuid MonsterId = FGuid (16, 4, 2, 1);

        FGridMON164Fixture ()
        {
            if (!TestWorld.World)
            {
                return;
            }

            Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
            Owner = TestWorld.World->SpawnActor<AActor> ();
            Monster = TestWorld.World->SpawnActor<AGridMonsterActor> ();
            if (!Party || !Party->PartyInventoryComponent || !Owner || !Monster)
            {
                return;
            }

            Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = {
                MON164MakeCharacter (CharacterIds[0], TEXT ("A")),
                MON164MakeCharacter (CharacterIds[1], TEXT ("B")),
                MON164MakeCharacter (CharacterIds[2], TEXT ("C"))
            };
            Monster->SpawnObjectId = MonsterId;
            Monster->PersistentMonsterId = MonsterId;

            TurnManager = NewObject<UGridTurnManagerComponent> (
                Owner,
                TEXT ("MON164TurnManager"));
            Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem> (
                TestWorld.World,
                TEXT ("MON164Lifecycle"));
            if (!TurnManager || !Lifecycle)
            {
                return;
            }

            TurnManager->PartyPawn = Party;
            TurnManager->CombatMonsters.Add (Monster);
            TurnManager->bCombatActive = true;
            TurnManager->RoundNumber = 1;
            ResetInitiative (true);
            Lifecycle->BindToTurnManager (TurnManager);
        }

        void ResetInitiative (bool bActiveFirst)
        {
            if (!TurnManager)
            {
                return;
            }
            TurnManager->InitiativeOrder = {
                MON164MakeEntry (
                    CharacterIds[0],
                    EGridCombatantSide::Party,
                    0,
                    30,
                    bActiveFirst
                        ? EGridCombatantTurnState::Active
                        : EGridCombatantTurnState::Waiting),
                MON164MakeEntry (
                    CharacterIds[1],
                    EGridCombatantSide::Party,
                    1,
                    20,
                    EGridCombatantTurnState::Waiting),
                MON164MakeEntry (
                    MonsterId,
                    EGridCombatantSide::Monster,
                    INDEX_NONE,
                    15,
                    EGridCombatantTurnState::Waiting),
                MON164MakeEntry (
                    CharacterIds[2],
                    EGridCombatantSide::Party,
                    2,
                    10,
                    EGridCombatantTurnState::Waiting)
            };
            TurnManager->CurrentInitiativeIndex = bActiveFirst ? 0 : INDEX_NONE;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164AggregateModifierTest,
    "Grimrock.RPG.MON16.4.AggregateModifier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164AggregateModifierTest::RunTest (const FString& Parameters)
{
    UGridStatusEffectDefinitionAsset* Positive = MON164MakeDefinition (
        GetTransientPackage (), TEXT ("MON164_Positive"), 8);
    UGridStatusEffectDefinitionAsset* Negative = MON164MakeDefinition (
        GetTransientPackage (), TEXT ("MON164_Negative"), -3);
    FGridStatusEffectCollection Collection;
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Positive effect applies"),
        Collection.TryApply (*Positive, FGuid::NewGuid (), Result, Error));
    TestTrue (TEXT ("Negative effect applies"),
        Collection.TryApply (*Negative, FGuid::NewGuid (), Result, Error));
    TestEqual (TEXT ("Different effects add algebraically"),
        FGridStatusEffectInitiativeResolver::ComputeModifier (Collection), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164StackScalingAndSaturationTest,
    "Grimrock.RPG.MON16.4.StackScalingAndSaturation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164StackScalingAndSaturationTest::RunTest (const FString& Parameters)
{
    UGridStatusEffectDefinitionAsset* Stacked = MON164MakeDefinition (
        GetTransientPackage (),
        TEXT ("MON164_Stacked"),
        7,
        EGridStatusEffectDurationUnit::Rounds,
        3,
        EGridStatusEffectStackPolicy::AddStacks,
        3);
    FGridStatusEffectCollection Collection;
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Three stacks apply"),
        Collection.TryApply (
            *Stacked,
            FGuid::NewGuid (),
            3,
            INDEX_NONE,
            INDEX_NONE,
            Result,
            Error));
    TestEqual (TEXT ("Modifier scales per active stack"),
        FGridStatusEffectInitiativeResolver::ComputeModifier (Collection), 21);

    UGridStatusEffectDefinitionAsset* HugePositive = MON164MakeDefinition (
        GetTransientPackage (),
        TEXT ("MON164_HugePositive"),
        MAX_int32,
        EGridStatusEffectDurationUnit::Rounds,
        3,
        EGridStatusEffectStackPolicy::AddStacks,
        2);
    FGridStatusEffectCollection PositiveCollection;
    TestTrue (TEXT ("Huge positive applies"),
        PositiveCollection.TryApply (
            *HugePositive,
            FGuid::NewGuid (),
            2,
            INDEX_NONE,
            INDEX_NONE,
            Result,
            Error));
    TestEqual (TEXT ("Positive overflow saturates"),
        FGridStatusEffectInitiativeResolver::ComputeModifier (PositiveCollection),
        MAX_int32);

    UGridStatusEffectDefinitionAsset* HugeNegative = MON164MakeDefinition (
        GetTransientPackage (),
        TEXT ("MON164_HugeNegative"),
        MIN_int32,
        EGridStatusEffectDurationUnit::Rounds,
        3,
        EGridStatusEffectStackPolicy::AddStacks,
        2);
    FGridStatusEffectCollection NegativeCollection;
    TestTrue (TEXT ("Huge negative applies"),
        NegativeCollection.TryApply (
            *HugeNegative,
            FGuid::NewGuid (),
            2,
            INDEX_NONE,
            INDEX_NONE,
            Result,
            Error));
    TestEqual (TEXT ("Negative overflow saturates"),
        FGridStatusEffectInitiativeResolver::ComputeModifier (NegativeCollection),
        MIN_int32);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164TurnOrderBroadcastProjectionTest,
    "Grimrock.RPG.MON16.4.TurnOrderBroadcastProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164TurnOrderBroadcastProjectionTest::RunTest (const FString& Parameters)
{
    FGridMON164Fixture Fixture;
    if (!Fixture.Lifecycle || !Fixture.TurnManager) return false;
    Fixture.ResetInitiative (false);

    UGridStatusEffectDefinitionAsset* Positive = MON164MakeDefinition (
        GetTransientPackage (), TEXT ("MON164_PreExisting"), 25);
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Pre-existing status can be stored before order publication"),
        Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[2]
            .StatusEffects.TryApply (*Positive, FGuid::NewGuid (), Result, Error));

    Fixture.TurnManager->OnTurnOrderChanged.Broadcast ();
    const FGridCombatantInitiativeEntry* Entry =
        MON164FindEntry (Fixture.TurnManager, Fixture.CharacterIds[2]);
    TestEqual (TEXT ("Published order receives projected modifier"),
        Entry ? Entry->InitiativeModifier : 0, 25);
    TestEqual (TEXT ("Pre-existing status can become the initiative leader"),
        Fixture.TurnManager->InitiativeOrder[0].CombatantId,
        Fixture.CharacterIds[2]);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164FutureHasteReorderTest,
    "Grimrock.RPG.MON16.4.FutureHasteReorder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164FutureHasteReorderTest::RunTest (const FString& Parameters)
{
    FGridMON164Fixture Fixture;
    if (!Fixture.Lifecycle || !Fixture.TurnManager) return false;
    const int32 OriginalRoll = Fixture.TurnManager->InitiativeOrder[3].InitiativeRoll;
    const int32 OriginalTotal = Fixture.TurnManager->InitiativeOrder[3].InitiativeTotal;
    UGridStatusEffectDefinitionAsset* Positive = MON164MakeDefinition (
        GetTransientPackage (), TEXT ("MON164_FuturePositive"), 12);
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Future positive modifier applies"),
        Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter (
            2, Positive, FGuid::NewGuid (), Result, Error));
    TestEqual (TEXT ("Active combatant remains first"),
        Fixture.TurnManager->InitiativeOrder[0].CombatantId,
        Fixture.CharacterIds[0]);
    TestEqual (TEXT ("Hasted future combatant moves ahead"),
        Fixture.TurnManager->InitiativeOrder[1].CombatantId,
        Fixture.CharacterIds[2]);
    const FGridCombatantInitiativeEntry* Entry =
        MON164FindEntry (Fixture.TurnManager, Fixture.CharacterIds[2]);
    TestEqual (TEXT ("Encounter roll is preserved"),
        Entry ? Entry->InitiativeRoll : -1, OriginalRoll);
    TestEqual (TEXT ("Rolled initiative total is preserved"),
        Entry ? Entry->InitiativeTotal : -1, OriginalTotal);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164FutureSlowReorderTest,
    "Grimrock.RPG.MON16.4.FutureSlowReorder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164FutureSlowReorderTest::RunTest (const FString& Parameters)
{
    FGridMON164Fixture Fixture;
    if (!Fixture.Lifecycle || !Fixture.TurnManager) return false;
    UGridStatusEffectDefinitionAsset* Negative = MON164MakeDefinition (
        GetTransientPackage (), TEXT ("MON164_FutureNegative"), -15);
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Future negative modifier applies"),
        Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter (
            1, Negative, FGuid::NewGuid (), Result, Error));
    TestEqual (TEXT ("Active combatant remains first"),
        Fixture.TurnManager->InitiativeOrder[0].CombatantId,
        Fixture.CharacterIds[0]);
    TestEqual (TEXT ("Monster now precedes slowed combatant"),
        Fixture.TurnManager->InitiativeOrder[1].CombatantId,
        Fixture.MonsterId);
    TestEqual (TEXT ("Unmodified party member also precedes slowed combatant"),
        Fixture.TurnManager->InitiativeOrder[2].CombatantId,
        Fixture.CharacterIds[2]);
    TestEqual (TEXT ("Slowed future combatant moves last"),
        Fixture.TurnManager->InitiativeOrder[3].CombatantId,
        Fixture.CharacterIds[1]);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164ActiveCombatantStabilityTest,
    "Grimrock.RPG.MON16.4.ActiveCombatantStability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164ActiveCombatantStabilityTest::RunTest (const FString& Parameters)
{
    FGridMON164Fixture Fixture;
    if (!Fixture.Lifecycle || !Fixture.TurnManager) return false;
    const int32 Roll = Fixture.TurnManager->InitiativeOrder[0].InitiativeRoll;
    const int32 Total = Fixture.TurnManager->InitiativeOrder[0].InitiativeTotal;
    UGridStatusEffectDefinitionAsset* Positive = MON164MakeDefinition (
        GetTransientPackage (), TEXT ("MON164_ActivePositive"), 100);
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Active status applies"),
        Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter (
            0, Positive, FGuid::NewGuid (), Result, Error));
    TestEqual (TEXT ("Active combatant never moves retroactively"),
        Fixture.TurnManager->InitiativeOrder[0].CombatantId,
        Fixture.CharacterIds[0]);
    const FGridCombatantInitiativeEntry* Entry =
        MON164FindEntry (Fixture.TurnManager, Fixture.CharacterIds[0]);
    TestEqual (TEXT ("Active modifier updates"),
        Entry ? Entry->InitiativeModifier : 0, 100);
    TestEqual (TEXT ("Active roll unchanged"), Entry ? Entry->InitiativeRoll : -1, Roll);
    TestEqual (TEXT ("Active total unchanged"), Entry ? Entry->InitiativeTotal : -1, Total);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164TurnExpirationTest,
    "Grimrock.RPG.MON16.4.TurnExpiration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164TurnExpirationTest::RunTest (const FString& Parameters)
{
    FGridMON164Fixture Fixture;
    if (!Fixture.Lifecycle || !Fixture.TurnManager) return false;
    UGridStatusEffectDefinitionAsset* Positive = MON164MakeDefinition (
        GetTransientPackage (),
        TEXT ("MON164_TurnPositive"),
        9,
        EGridStatusEffectDurationUnit::Turns,
        1);
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Turn effect applies"),
        Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter (
            0, Positive, FGuid::NewGuid (), Result, Error));
    FGridCombatantInitiativeEntry Completed = Fixture.TurnManager->InitiativeOrder[0];
    Completed.State = EGridCombatantTurnState::Completed;
    Fixture.TurnManager->OnCombatantStateChanged.Broadcast (Completed);
    const FGridCharacterInventoryState& Character =
        Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
    TestFalse (TEXT ("Turn effect expires"),
        Character.StatusEffects.Contains (Positive->EffectId));
    const FGridCombatantInitiativeEntry* Entry =
        MON164FindEntry (Fixture.TurnManager, Fixture.CharacterIds[0]);
    TestEqual (TEXT ("Expired turn effect restores zero modifier"),
        Entry ? Entry->InitiativeModifier : 999, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164RoundExpirationTest,
    "Grimrock.RPG.MON16.4.RoundExpiration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164RoundExpirationTest::RunTest (const FString& Parameters)
{
    FGridMON164Fixture Fixture;
    if (!Fixture.Lifecycle || !Fixture.TurnManager) return false;
    UGridStatusEffectDefinitionAsset* Positive = MON164MakeDefinition (
        GetTransientPackage (),
        TEXT ("MON164_RoundPositive"),
        12,
        EGridStatusEffectDurationUnit::Rounds,
        1);
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Round effect applies"),
        Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter (
            2, Positive, FGuid::NewGuid (), Result, Error));
    Fixture.TurnManager->OnRoundStarted.Broadcast (1);
    TestTrue (TEXT ("Round one is still baseline"),
        Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[2]
            .StatusEffects.Contains (Positive->EffectId));
    Fixture.TurnManager->OnRoundStarted.Broadcast (2);
    TestFalse (TEXT ("Round effect expires at first boundary"),
        Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[2]
            .StatusEffects.Contains (Positive->EffectId));
    const FGridCombatantInitiativeEntry* Entry =
        MON164FindEntry (Fixture.TurnManager, Fixture.CharacterIds[2]);
    TestEqual (TEXT ("Expired round effect restores zero modifier"),
        Entry ? Entry->InitiativeModifier : 999, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164MonsterParityTest,
    "Grimrock.RPG.MON16.4.MonsterParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164MonsterParityTest::RunTest (const FString& Parameters)
{
    FGridMON164Fixture Fixture;
    if (!Fixture.Lifecycle || !Fixture.TurnManager || !Fixture.Monster) return false;
    UGridStatusEffectDefinitionAsset* Negative = MON164MakeDefinition (
        GetTransientPackage (), TEXT ("MON164_MonsterNegative"), -10);
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("Monster status applies through shared lifecycle"),
        Fixture.Lifecycle->TryApplyStatusEffectToMonster (
            Fixture.Monster,
            Negative,
            FGuid::NewGuid (),
            Result,
            Error));
    const FGridCombatantInitiativeEntry* Entry =
        MON164FindEntry (Fixture.TurnManager, Fixture.MonsterId);
    TestEqual (TEXT ("Monster receives same initiative projection"),
        Entry ? Entry->InitiativeModifier : 0, -10);
    TestEqual (TEXT ("Slowed monster moves behind unmodified future party member"),
        Fixture.TurnManager->InitiativeOrder.Last ().CombatantId,
        Fixture.MonsterId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164ReapplicationUpdatesModifierTest,
    "Grimrock.RPG.MON16.4.ReapplicationUpdatesModifier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164ReapplicationUpdatesModifierTest::RunTest (const FString& Parameters)
{
    FGridMON164Fixture Fixture;
    if (!Fixture.Lifecycle || !Fixture.TurnManager) return false;
    UGridStatusEffectDefinitionAsset* Stacked = MON164MakeDefinition (
        GetTransientPackage (),
        TEXT ("MON164_StackReapply"),
        4,
        EGridStatusEffectDurationUnit::Rounds,
        3,
        EGridStatusEffectStackPolicy::AddStacks,
        3);
    FGridStatusEffectApplyResult Result;
    FString Error;
    TestTrue (TEXT ("First stack applies"),
        Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter (
            2, Stacked, FGuid::NewGuid (), Result, Error));
    const FGridCombatantInitiativeEntry* First =
        MON164FindEntry (Fixture.TurnManager, Fixture.CharacterIds[2]);
    TestEqual (TEXT ("First stack contributes once"),
        First ? First->InitiativeModifier : 0, 4);
    TestTrue (TEXT ("Second stack reapplies"),
        Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter (
            2, Stacked, FGuid::NewGuid (), Result, Error));
    const FGridCombatantInitiativeEntry* Second =
        MON164FindEntry (Fixture.TurnManager, Fixture.CharacterIds[2]);
    TestEqual (TEXT ("Two stacks contribute twice"),
        Second ? Second->InitiativeModifier : 0, 8);
    TestEqual (TEXT ("Second stack changes future order immediately"),
        Fixture.TurnManager->InitiativeOrder[2].CombatantId,
        Fixture.CharacterIds[2]);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON164NoParallelSystemTest,
    "Grimrock.RPG.MON16.4.NoParallelSystem",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON164NoParallelSystemTest::RunTest (const FString& Parameters)
{
    const TArray<FString> Paths = {
        TEXT ("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectInitiativeResolver.h"),
        TEXT ("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectInitiativeResolver.cpp"),
        TEXT ("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"),
        TEXT ("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp")
    };
    for (const FString& RelativePath : Paths)
    {
        FString Source;
        const FString FullPath = FPaths::Combine (FPaths::ProjectDir (), RelativePath);
        TestTrue (*FString::Printf (TEXT ("Load %s"), *RelativePath),
            FFileHelper::LoadFileToString (Source, *FullPath));
        TestFalse (*FString::Printf (TEXT ("No UI include in %s"), *RelativePath),
            Source.Contains (TEXT ("#include \"UI/")));
        TestFalse (*FString::Printf (TEXT ("No UMG in %s"), *RelativePath),
            Source.Contains (TEXT ("UMG")));
        TestFalse (*FString::Printf (TEXT ("No hard-coded Haste id in %s"), *RelativePath),
            Source.Contains (TEXT ("TEXT (\"Haste\")")));
        TestFalse (*FString::Printf (TEXT ("No hard-coded Slow id in %s"), *RelativePath),
            Source.Contains (TEXT ("TEXT (\"Slow\")")));
    }

    FString LifecycleSource;
    const FString LifecyclePath = FPaths::Combine (
        FPaths::ProjectDir (),
        TEXT ("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp"));
    TestTrue (TEXT ("Lifecycle source loads"),
        FFileHelper::LoadFileToString (LifecycleSource, *LifecyclePath));
    TestTrue (TEXT ("Projection reuses TurnManager setter"),
        LifecycleSource.Contains (TEXT ("SetCombatantInitiativeModifier")));
    TestFalse (TEXT ("Lifecycle does not rewrite the encounter roll"),
        LifecycleSource.Contains (TEXT ("InitiativeRoll =")));
    TestFalse (TEXT ("Lifecycle does not rewrite rolled initiative total"),
        LifecycleSource.Contains (TEXT ("InitiativeTotal =")));
    return true;
}

#endif
