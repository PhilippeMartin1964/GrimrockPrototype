#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellHotbarExecution.h"
#include "Misc/AutomationTest.h"

namespace
{
    FGridCharacterSpellbookState MakeUI0143e2Spellbook (
        const FGuid& CharacterId,
        FName SpellId)
    {
        FGridCharacterSpellbookState Spellbook;
        Spellbook.CharacterId = CharacterId;
        if (!SpellId.IsNone ())
        {
            Spellbook.KnownSpellIds.Add (SpellId);
        }
        return Spellbook;
    }

    FRPGDerivedStats MakeUI0143e2CasterStats (
        int32 CurrentHealth = 20,
        int32 CurrentMana = 10)
    {
        FRPGDerivedStats Stats;
        Stats.MaxHealth = 20;
        Stats.CurrentHealth = CurrentHealth;
        Stats.MaxMana = 10;
        Stats.CurrentMana = CurrentMana;
        return Stats;
    }

    FGridPlayerCharacterTurnState MakeUI0143e2TurnState (
        const FGuid& CharacterId,
        int32 RemainingActionPoints = 4)
    {
        FGridPlayerCharacterTurnState State;
        State.CharacterIndex = 0;
        State.CharacterId = CharacterId;
        State.MaximumActionPoints = 4;
        State.RemainingActionPoints = RemainingActionPoints;
        State.State = EGridCombatantTurnState::Active;
        return State;
    }

    FGridSpellCastRequest MakeUI0143e2Request (
        const FGuid& CharacterId,
        FName SpellId,
        const FGuid& TargetId,
        FIntPoint TargetCell)
    {
        FGridSpellCastRequest Request;
        Request.CasterCharacterId = CharacterId;
        Request.SpellId = SpellId;
        Request.Target.TargetId = TargetId;
        Request.Target.GridCell = TargetCell;
        Request.Target.bHasGridCell = true;
        return Request;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridUI0143e2ArcaneBoltExecutionTest,
    "Grimrock.UI.UI01.4.3e.2.ArcaneBoltExecution",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridUI0143e2ArcaneBoltExecutionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGuid CharacterId = FGuid::NewGuid ();
    const FGuid TargetId = FGuid::NewGuid ();
    const FGridSpellDefinition Definition =
        FGridProductionSpellLibrary::MakeArcaneBolt ();
    const FGridCharacterSpellbookState Spellbook =
        MakeUI0143e2Spellbook (CharacterId, Definition.SpellId);
    const FRPGDerivedStats CasterStats = MakeUI0143e2CasterStats ();
    const FGridPlayerCharacterTurnState TurnState =
        MakeUI0143e2TurnState (CharacterId);
    const FGridSpellCastRequest Request = MakeUI0143e2Request (
        CharacterId,
        Definition.SpellId,
        TargetId,
        FIntPoint (1, 3));

    FGridSpellTargetingContext Context;
    Context.CasterCell = FIntPoint (1, 1);
    Context.ResolvedTargetId = TargetId;
    Context.ResolvedTargetCell = FIntPoint (1, 3);
    Context.bHasResolvedTargetCell = true;
    Context.bResolvedTargetIsHostile = true;
    Context.bLineOfSightClear = true;

    FGridSpellHotbarExecutionResult Result;
    TestTrue (
        TEXT ("Arcane Bolt executes through the MON18 pipeline"),
        FGridSpellHotbarExecutionService::TryExecute (
            Definition,
            Request,
            Context,
            Spellbook,
            CasterStats,
            TurnState,
            12,
            12,
            FGridStatusEffectCollection (),
            [] (FName) -> const UGridStatusEffectDefinitionAsset*
            {
                return nullptr;
            },
            Result));
    TestEqual (TEXT ("Arcane Bolt spends two AP"),
        Result.CasterTurnState.RemainingActionPoints, 2);
    TestEqual (TEXT ("Arcane Bolt spends three mana"),
        Result.CasterStats.CurrentMana, 7);
    TestEqual (TEXT ("Arcane Bolt deals four damage"),
        Result.TargetCurrentHealth, 8);
    TestEqual (TEXT ("Arcane Bolt reports four total damage"),
        Result.EffectResult.TotalDamage, 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridUI0143e2LesserHealExecutionTest,
    "Grimrock.UI.UI01.4.3e.2.LesserHealExecution",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridUI0143e2LesserHealExecutionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGuid CharacterId = FGuid::NewGuid ();
    const FGridSpellDefinition Definition =
        FGridProductionSpellLibrary::MakeLesserHeal ();
    const FGridCharacterSpellbookState Spellbook =
        MakeUI0143e2Spellbook (CharacterId, Definition.SpellId);
    const FRPGDerivedStats CasterStats =
        MakeUI0143e2CasterStats (10, 10);
    const FGridPlayerCharacterTurnState TurnState =
        MakeUI0143e2TurnState (CharacterId);
    const FGridSpellCastRequest Request = MakeUI0143e2Request (
        CharacterId,
        Definition.SpellId,
        CharacterId,
        FIntPoint (2, 2));

    FGridSpellTargetingContext Context;
    Context.CasterCell = FIntPoint (2, 2);
    Context.ResolvedTargetId = CharacterId;
    Context.ResolvedTargetCell = FIntPoint (2, 2);
    Context.bHasResolvedTargetCell = true;
    Context.bResolvedTargetIsAlly = true;
    Context.bLineOfSightClear = true;

    FGridSpellHotbarExecutionResult Result;
    TestTrue (
        TEXT ("Lesser Heal executes on an allied target"),
        FGridSpellHotbarExecutionService::TryExecute (
            Definition,
            Request,
            Context,
            Spellbook,
            CasterStats,
            TurnState,
            20,
            10,
            FGridStatusEffectCollection (),
            [] (FName) -> const UGridStatusEffectDefinitionAsset*
            {
                return nullptr;
            },
            Result));
    TestEqual (TEXT ("Lesser Heal spends two AP"),
        Result.CasterTurnState.RemainingActionPoints, 2);
    TestEqual (TEXT ("Lesser Heal spends four mana"),
        Result.CasterStats.CurrentMana, 6);
    TestEqual (TEXT ("Lesser Heal restores five health"),
        Result.TargetCurrentHealth, 15);
    TestEqual (TEXT ("Lesser Heal reports five healing"),
        Result.EffectResult.TotalHealing, 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridUI0143e2MissingStatusNoCostCommitTest,
    "Grimrock.UI.UI01.4.3e.2.MissingStatusNoCostCommit",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridUI0143e2MissingStatusNoCostCommitTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGuid CharacterId = FGuid::NewGuid ();
    const FGridSpellDefinition Definition =
        FGridProductionSpellLibrary::MakeHaste ();
    const FGridCharacterSpellbookState Spellbook =
        MakeUI0143e2Spellbook (CharacterId, Definition.SpellId);
    const FRPGDerivedStats CasterStats = MakeUI0143e2CasterStats ();
    const FGridPlayerCharacterTurnState TurnState =
        MakeUI0143e2TurnState (CharacterId);
    const FGridSpellCastRequest Request = MakeUI0143e2Request (
        CharacterId,
        Definition.SpellId,
        CharacterId,
        FIntPoint (0, 0));

    FGridSpellTargetingContext Context;
    Context.CasterCell = FIntPoint (0, 0);
    Context.ResolvedTargetId = CharacterId;
    Context.ResolvedTargetCell = FIntPoint (0, 0);
    Context.bHasResolvedTargetCell = true;
    Context.bResolvedTargetIsAlly = true;
    Context.bLineOfSightClear = true;

    FGridSpellHotbarExecutionResult Result;
    TestFalse (
        TEXT ("Haste rejects a missing MON16 status definition"),
        FGridSpellHotbarExecutionService::TryExecute (
            Definition,
            Request,
            Context,
            Spellbook,
            CasterStats,
            TurnState,
            20,
            20,
            FGridStatusEffectCollection (),
            [] (FName) -> const UGridStatusEffectDefinitionAsset*
            {
                return nullptr;
            },
            Result));
    TestTrue (TEXT ("The rejection is an effect-definition rejection"),
        Result.EffectRejectReason ==
            EGridSpellEffectResolutionRejectReason::
                MissingStatusEffectDefinition);
    TestEqual (TEXT ("Input mana remains unchanged"),
        CasterStats.CurrentMana, 10);
    TestEqual (TEXT ("Input AP remains unchanged"),
        TurnState.RemainingActionPoints, 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridUI0143e2UnknownSpellNoCostCommitTest,
    "Grimrock.UI.UI01.4.3e.2.UnknownSpellNoCostCommit",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridUI0143e2UnknownSpellNoCostCommitTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGuid CharacterId = FGuid::NewGuid ();
    const FGridSpellDefinition Definition =
        FGridProductionSpellLibrary::MakeLesserHeal ();
    const FGridCharacterSpellbookState Spellbook =
        MakeUI0143e2Spellbook (CharacterId, NAME_None);
    const FRPGDerivedStats CasterStats = MakeUI0143e2CasterStats (10, 10);
    const FGridPlayerCharacterTurnState TurnState =
        MakeUI0143e2TurnState (CharacterId);
    const FGridSpellCastRequest Request = MakeUI0143e2Request (
        CharacterId,
        Definition.SpellId,
        CharacterId,
        FIntPoint (0, 0));

    FGridSpellTargetingContext Context;
    Context.CasterCell = FIntPoint (0, 0);
    Context.ResolvedTargetId = CharacterId;
    Context.ResolvedTargetCell = FIntPoint (0, 0);
    Context.bHasResolvedTargetCell = true;
    Context.bResolvedTargetIsAlly = true;
    Context.bLineOfSightClear = true;

    FGridSpellHotbarExecutionResult Result;
    TestFalse (
        TEXT ("An unknown spell cannot execute from the hotbar"),
        FGridSpellHotbarExecutionService::TryExecute (
            Definition,
            Request,
            Context,
            Spellbook,
            CasterStats,
            TurnState,
            20,
            10,
            FGridStatusEffectCollection (),
            [] (FName) -> const UGridStatusEffectDefinitionAsset*
            {
                return nullptr;
            },
            Result));
    TestTrue (TEXT ("Unknown spell rejects in the transaction stage"),
        Result.PipelineRejectStage ==
            EGridSpellCastPipelineRejectStage::Transaction);
    TestTrue (TEXT ("Unknown spell reports SpellNotKnown"),
        Result.TransactionRejectReason ==
            EGridSpellCastTransactionRejectReason::SpellNotKnown);
    TestEqual (TEXT ("Input mana remains unchanged"),
        CasterStats.CurrentMana, 10);
    TestEqual (TEXT ("Input AP remains unchanged"),
        TurnState.RemainingActionPoints, 4);
    return true;
}

#endif
