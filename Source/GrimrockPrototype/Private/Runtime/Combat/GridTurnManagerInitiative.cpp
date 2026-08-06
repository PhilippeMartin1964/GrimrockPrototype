#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"

namespace
{
    constexpr int32 InitiativeSeedSalt = 0x4D4F4E12;

    FText ResolveInitiativeCharacterName (
        const FGridCharacterInventoryState& Character,
        int32 CharacterIndex)
    {
        return Character.DisplayName.IsEmpty ()
            ? FText::FromString (FString::Printf (
                TEXT ("Hero_%02d"),
                CharacterIndex + 1))
            : Character.DisplayName;
    }
}

void UGridTurnManagerComponent::BuildGlobalInitiativeOrder ()
{
    InitiativeOrder.Reset ();
    CurrentInitiativeIndex = INDEX_NONE;

    if (IsValid (PartyPawn) &&
        IsValid (PartyPawn->PartyInventoryComponent))
    {
        const TArray<FGridCharacterInventoryState>& Characters =
            PartyPawn->PartyInventoryComponent->PartyInventoryState
                .ActiveCharacters;
        InitiativeOrder.Reserve (
            Characters.Num () + CombatMonsters.Num ());

        for (int32 CharacterIndex = 0;
            CharacterIndex < Characters.Num ();
            ++CharacterIndex)
        {
            const FGridCharacterInventoryState& Character =
                Characters[CharacterIndex];

            FGridInventoryCharacterSummary Summary;
            const bool bHasSummary =
                PartyPawn->PartyInventoryComponent->GetCharacterSummary (
                    CharacterIndex,
                    Summary);

            FGridCombatantInitiativeEntry Entry;
            Entry.CombatantId =
                ResolvePlayerCombatantId (CharacterIndex);
            Entry.Side = EGridCombatantSide::Party;
            Entry.CharacterIndex = CharacterIndex;
            Entry.DisplayName = ResolveInitiativeCharacterName (
                Character,
                CharacterIndex);
            Entry.Portrait = Character.Portrait;
            Entry.InitiativeBase =
                10 + Character.DerivedStats.Initiative;
            Entry.Dexterity = bHasSummary
                ? Summary.Attributes.Dexterity
                : Character.Attributes.Dexterity;
            Entry.CurrentHealth =
                Character.DerivedStats.CurrentHealth;
            Entry.MaximumHealth = FMath::Max (
                1,
                Character.DerivedStats.MaxHealth);
            Entry.State = Entry.CurrentHealth > 0
                ? EGridCombatantTurnState::Waiting
                : EGridCombatantTurnState::Defeated;
            InitiativeOrder.Add (Entry);
        }
    }

    for (AGridMonsterActor* Monster : CombatMonsters)
    {
        FString DefinitionError;
        if (!IsValid (Monster) ||
            !Monster->ValidateMonsterDefinition (DefinitionError) ||
            !Monster->bCombatStatsInitialized)
        {
            if (IsValid (Monster))
            {
                UE_LOG (
                    LogGridTurnManager,
                    Error,
                    TEXT ("[GridInitiative] Monster skipped Monster=%s PersistenceId=%s Definition=%s MonsterId=%s CurrentHealth=%d MaxHealth=%d CombatStatsInitialized=%s RuntimeState=%s Reason=%s ValidationError=\"%s\""),
                    *GetNameSafe (Monster),
                    *Monster->ResolvePersistenceId ().ToString (),
                    *GetPathNameSafe (Monster->MonsterDefinition),
                    Monster->MonsterDefinition
                        ? *Monster->MonsterDefinition->MonsterId.ToString ()
                        : TEXT ("None"),
                    Monster->CurrentHealth,
                    Monster->MonsterDefinition
                        ? Monster->MonsterDefinition->MaxHealth
                        : 0,
                    Monster->bCombatStatsInitialized
                        ? TEXT ("true")
                        : TEXT ("false"),
                    *UEnum::GetValueAsString (Monster->MonsterState),
                    DefinitionError.IsEmpty ()
                        ? TEXT ("UninitializedCombatState")
                        : TEXT ("InvalidDefinition"),
                    *DefinitionError);
            }
            continue;
        }

        const FGuid PersistenceId = Monster->ResolvePersistenceId ();
        if (!PersistenceId.IsValid ())
        {
            continue;
        }

        FGridCombatantInitiativeEntry Entry;
        Entry.CombatantId = PersistenceId;
        Entry.Side = EGridCombatantSide::Monster;
        Entry.DisplayName = ResolveMonsterDisplayName (Monster);
        Entry.Portrait = Monster->MonsterDefinition->Icon;
        Entry.InitiativeBase =
            Monster->MonsterDefinition->Initiative;
        Entry.CurrentHealth = Monster->CurrentHealth;
        Entry.MaximumHealth = FMath::Max (
            1,
            Monster->MonsterDefinition->MaxHealth);
        Entry.State = Monster->IsDead ()
            ? EGridCombatantTurnState::Defeated
            : EGridCombatantTurnState::Waiting;
        InitiativeOrder.Add (Entry);
    }

    InitiativeRandomStream.Initialize (
        ActiveEncounterRandomSeed ^ InitiativeSeedSalt);
    FGridInitiativeOrderBuilder::RollAndSort (
        InitiativeOrder,
        InitiativeRandomStream);

    EnemyTurnOrder.Reset ();
    for (const FGridCombatantInitiativeEntry& Entry : InitiativeOrder)
    {
        if (Entry.Side == EGridCombatantSide::Monster)
        {
            if (AGridMonsterActor* Monster =
                FindCombatMonsterById (Entry.CombatantId))
            {
                EnemyTurnOrder.Add (Monster);
            }
        }

        UE_LOG (
            LogGridTurnManager,
            Log,
            TEXT ("[GridInitiative] Side=%s Id=%s Character=%d Base=%d Roll=%d Total=%d State=%s"),
            *UEnum::GetValueAsString (Entry.Side),
            *Entry.CombatantId.ToString (EGuidFormats::Digits),
            Entry.CharacterIndex,
            Entry.InitiativeBase,
            Entry.InitiativeRoll,
            Entry.InitiativeTotal,
            *UEnum::GetValueAsString (Entry.State));
    }

    BroadcastInitiativeOrderChanged ();
}

void UGridTurnManagerComponent::ResetInitiativeRound ()
{
    CurrentInitiativeIndex = INDEX_NONE;
    ResetPartyMobilityForRound ();
    BeginPlayerCharacterPhase ();

    for (FGridCombatantInitiativeEntry& Entry : InitiativeOrder)
    {
        RefreshInitiativeEntryVitals (Entry);
        const AGridMonsterActor* Monster =
            Entry.Side == EGridCombatantSide::Monster
                ? FindCombatMonsterById (Entry.CombatantId)
                : nullptr;
        const bool bDefeated = Entry.Side == EGridCombatantSide::Party
            ? Entry.CurrentHealth <= 0
            : IsValid (Monster) && Monster->IsDead ();
        if (bDefeated)
        {
            Entry.State = EGridCombatantTurnState::Defeated;
        }
        else if (Entry.State != EGridCombatantTurnState::Incapacitated)
        {
            Entry.State = EGridCombatantTurnState::Waiting;
        }
    }

    BroadcastInitiativeOrderChanged ();
}

void UGridTurnManagerComponent::BeginNextCombatantTurn ()
{
    if (!bCombatActive)
    {
        return;
    }

    if (!HasLivingPartyCharacter ())
    {
        FinishCombat (EGridCombatPhase::Defeat);
        return;
    }
    if (!HasLivingCombatMonster ())
    {
        FinishCombat (EGridCombatPhase::Victory);
        return;
    }

    while (++CurrentInitiativeIndex < InitiativeOrder.Num ())
    {
        FGridCombatantInitiativeEntry& Entry =
            InitiativeOrder[CurrentInitiativeIndex];
        RefreshInitiativeEntryVitals (Entry);
        if (Entry.State == EGridCombatantTurnState::Defeated ||
            Entry.State == EGridCombatantTurnState::Incapacitated)
        {
            OnCombatantStateChanged.Broadcast (Entry);
            continue;
        }

        const bool bStarted = Entry.Side == EGridCombatantSide::Party
            ? BeginPlayerCombatantTurn (Entry)
            : BeginMonsterCombatantTurn (Entry);
        if (bStarted)
        {
            return;
        }

        SetInitiativeEntryState (
            Entry,
            EGridCombatantTurnState::Incapacitated);
    }

    FinishInitiativeRound ();
}

bool UGridTurnManagerComponent::BeginPlayerCombatantTurn (
    FGridCombatantInitiativeEntry& Entry)
{
    if (Entry.Side != EGridCombatantSide::Party ||
        !IsValid (PartyPawn) ||
        !IsValid (PartyPawn->PartyInventoryComponent))
    {
        return false;
    }

    FGridPlayerCharacterTurnState* TurnState =
        EnsurePlayerCharacterTurnState (Entry.CharacterIndex);
    if (!TurnState ||
        TurnState->State == EGridCombatantTurnState::Defeated)
    {
        return false;
    }

    if (!PhaseState.BeginCombatantTurn (EGridCombatantSide::Party))
    {
        return false;
    }

    SetPartyInputLocked (false);
    SetInitiativeEntryState (Entry, EGridCombatantTurnState::Active);
    TurnState->MaximumActionPoints = FMath::Clamp (
        BasePlayerActionPointsPerTurn,
        2,
        6);
    TurnState->RemainingActionPoints =
        TurnState->MaximumActionPoints;
    TurnState->State = EGridCombatantTurnState::Active;
    BroadcastPlayerCharacterTurnState (*TurnState);
    SetPhase (PhaseState.GetPhase ());
    PartyPawn->PartyInventoryComponent->SetSelectedCharacterIndex (
        Entry.CharacterIndex);
    OnActiveCombatantChanged.Broadcast (Entry);

    UE_LOG (
        LogGridTurnManager,
        Log,
        TEXT ("[GridInitiative] Active Side=Party Character=%d Id=%s AP=%d/%d"),
        Entry.CharacterIndex,
        *Entry.CombatantId.ToString (EGuidFormats::Digits),
        TurnState->RemainingActionPoints,
        TurnState->MaximumActionPoints);
    return true;
}

bool UGridTurnManagerComponent::BeginMonsterCombatantTurn (
    FGridCombatantInitiativeEntry& Entry)
{
    if (Entry.Side != EGridCombatantSide::Monster)
    {
        return false;
    }

    AGridMonsterActor* Candidate =
        FindCombatMonsterById (Entry.CombatantId);
    if (!IsValid (Candidate) ||
        Candidate->IsDead () ||
        !PrepareMonsterForCombat (Candidate))
    {
        return false;
    }

    UnbindCurrentMovement ();
    UnbindCurrentCombat ();
    PendingActions.Reset ();
    bHasActiveAction = false;
    ActiveAction = FGridCombatAction ();
    ActiveActionTimeoutRemaining = 0.0f;
    ResetActiveAttackState ();

    CurrentMonster = Candidate;
    CurrentEnemyIndex = EnemyTurnOrder.IndexOfByPredicate (
        [Candidate] (const TObjectPtr<AGridMonsterActor>& Monster)
        {
            return Monster.Get () == Candidate;
        });
    CurrentMonsterMaximumActionPoints = FMath::Max (
        0,
        Candidate->MonsterDefinition->ActionPointsPerTurn);
    ActionPointBudget.Reset (CurrentMonsterMaximumActionPoints);
    CurrentMonsterRemainingActionPoints =
        ActionPointBudget.GetRemainingPoints ();
    if (bCollectRuntimeMetrics)
    {
        ++RuntimeMetrics.MonsterTurnsStarted;
    }

    BindCurrentMovement (
        Candidate->FindComponentByClass<UGridMonsterMovementComponent> ());
    BindCurrentCombat (
        Candidate->FindComponentByClass<UGridMonsterCombatComponent> ());

    if (!PhaseState.BeginCombatantTurn (EGridCombatantSide::Monster))
    {
        return false;
    }

    SetPartyInputLocked (true);
    SetInitiativeEntryState (Entry, EGridCombatantTurnState::Active);
    SetPhase (PhaseState.GetPhase ());
    OnActiveCombatantChanged.Broadcast (Entry);

    FGridCombatLogEntry TurnEntry;
    TurnEntry.RoundNumber = RoundNumber;
    TurnEntry.Phase = CurrentPhase;
    TurnEntry.Type = EGridCombatLogEntryType::MonsterTurnStarted;
    TurnEntry.SourceId = ResolveMonsterLogId (Candidate);
    TurnEntry.SourceDisplayName = ResolveMonsterDisplayName (Candidate);
    TurnEntry.Message =
        FGridCombatLogFormatter::FormatMonsterTurnStarted (
            TurnEntry.SourceDisplayName);
    AppendCombatLogEntry (TurnEntry);
    OnMonsterTurnStarted.Broadcast (Candidate);
    PrepareCurrentMonsterActions ();
    ExecuteNextAction ();
    return true;
}

bool UGridTurnManagerComponent::EndActivePlayerTurn ()
{
    if (!CanEndActivePlayerTurn ())
    {
        return false;
    }

    FinishActivePlayerTurn ();
    return true;
}

bool UGridTurnManagerComponent::CanEndActivePlayerTurn () const
{
    FGridCombatantInitiativeEntry ActiveCombatant;
    return bInitialized &&
        bCombatActive &&
        !bPartyInputLocked &&
        !bPlayerAttackResolutionInProgress &&
        !bHasActiveAction &&
        GetActiveCombatant (ActiveCombatant) &&
        ActiveCombatant.Side == EGridCombatantSide::Party &&
        IsPartyAtRest ();
}

void UGridTurnManagerComponent::FinishActivePlayerTurn ()
{
    if (!InitiativeOrder.IsValidIndex (CurrentInitiativeIndex))
    {
        return;
    }

    FGridCombatantInitiativeEntry& Entry =
        InitiativeOrder[CurrentInitiativeIndex];
    if (Entry.Side != EGridCombatantSide::Party ||
        Entry.State != EGridCombatantTurnState::Active)
    {
        return;
    }

    if (FGridPlayerCharacterTurnState* TurnState =
        EnsurePlayerCharacterTurnState (Entry.CharacterIndex))
    {
        if (TurnState->State != EGridCombatantTurnState::Defeated &&
            TurnState->State != EGridCombatantTurnState::Incapacitated)
        {
            TurnState->State = EGridCombatantTurnState::Completed;
            BroadcastPlayerCharacterTurnState (*TurnState);
        }
    }

    SetInitiativeEntryState (Entry, EGridCombatantTurnState::Completed);
    SetPartyInputLocked (true);
    BeginNextCombatantTurn ();
}

void UGridTurnManagerComponent::FinishInitiativeRound ()
{
    if (!bCombatActive || !PhaseState.CompleteInitiativeRound ())
    {
        return;
    }

    SetPartyInputLocked (true);
    SetPhase (PhaseState.GetPhase ());

    if (!HasLivingPartyCharacter ())
    {
        FinishCombat (EGridCombatPhase::Defeat);
        return;
    }
    if (!HasLivingCombatMonster ())
    {
        FinishCombat (EGridCombatPhase::Victory);
        return;
    }
    if (!PhaseState.BeginNextRound ())
    {
        AbortCombat ();
        return;
    }

    RoundNumber = PhaseState.GetRoundNumber ();
    if (bCollectRuntimeMetrics)
    {
        ++RuntimeMetrics.RoundsStarted;
    }

    FGridCombatLogEntry RoundEntry;
    RoundEntry.RoundNumber = RoundNumber;
    RoundEntry.Phase = PhaseState.GetPhase ();
    RoundEntry.Type = EGridCombatLogEntryType::RoundStarted;
    RoundEntry.Message =
        FGridCombatLogFormatter::FormatRoundStarted (RoundNumber);
    AppendCombatLogEntry (RoundEntry);

    ResetInitiativeRound ();
    OnRoundStarted.Broadcast (RoundNumber);
    BeginNextCombatantTurn ();
}

void UGridTurnManagerComponent::ClearInitiativeState (bool bBroadcast)
{
    InitiativeOrder.Reset ();
    CurrentInitiativeIndex = INDEX_NONE;
    if (bBroadcast)
    {
        OnActiveCombatantChanged.Broadcast (
            FGridCombatantInitiativeEntry ());
        BroadcastInitiativeOrderChanged ();
    }
}

void UGridTurnManagerComponent::SetInitiativeEntryState (
    FGridCombatantInitiativeEntry& Entry,
    EGridCombatantTurnState NewState)
{
    if (Entry.State == NewState)
    {
        return;
    }

    Entry.State = NewState;
    OnCombatantStateChanged.Broadcast (Entry);
}

void UGridTurnManagerComponent::RefreshInitiativeEntryVitals (
    FGridCombatantInitiativeEntry& Entry)
{
    if (Entry.Side == EGridCombatantSide::Party)
    {
        if (!IsValid (PartyPawn) ||
            !IsValid (PartyPawn->PartyInventoryComponent))
        {
            Entry.CurrentHealth = 0;
            return;
        }

        const TArray<FGridCharacterInventoryState>& Characters =
            PartyPawn->PartyInventoryComponent->PartyInventoryState
                .ActiveCharacters;
        if (!Characters.IsValidIndex (Entry.CharacterIndex))
        {
            Entry.CurrentHealth = 0;
            return;
        }

        const FGridCharacterInventoryState& Character =
            Characters[Entry.CharacterIndex];
        Entry.CurrentHealth = Character.DerivedStats.CurrentHealth;
        Entry.MaximumHealth = FMath::Max (
            1,
            Character.DerivedStats.MaxHealth);
        return;
    }

    const AGridMonsterActor* Monster =
        FindCombatMonsterById (Entry.CombatantId);
    if (!IsValid (Monster) ||
        !IsValid (Monster->MonsterDefinition))
    {
        Entry.CurrentHealth = 0;
        return;
    }

    Entry.CurrentHealth = Monster->CurrentHealth;
    Entry.MaximumHealth = FMath::Max (
        1,
        Monster->MonsterDefinition->MaxHealth);
}

void UGridTurnManagerComponent::BroadcastInitiativeOrderChanged ()
{
    OnTurnOrderChanged.Broadcast ();
}

void UGridTurnManagerComponent::GetUpcomingInitiativeOrder (
    TArray<FGridCombatantInitiativeEntry>& OutEntries) const
{
    OutEntries.Reset ();
    const int32 StartIndex = InitiativeOrder.IsValidIndex (
        CurrentInitiativeIndex)
        ? CurrentInitiativeIndex
        : 0;
    for (int32 Index = StartIndex;
        Index < InitiativeOrder.Num ();
        ++Index)
    {
        const FGridCombatantInitiativeEntry& Entry =
            InitiativeOrder[Index];
        if (Entry.State == EGridCombatantTurnState::Active ||
            Entry.State == EGridCombatantTurnState::Waiting)
        {
            OutEntries.Add (Entry);
        }
    }
}

bool UGridTurnManagerComponent::GetActiveCombatant (
    FGridCombatantInitiativeEntry& OutCombatant) const
{
    OutCombatant = FGridCombatantInitiativeEntry ();
    if (!InitiativeOrder.IsValidIndex (CurrentInitiativeIndex) ||
        InitiativeOrder[CurrentInitiativeIndex].State !=
            EGridCombatantTurnState::Active)
    {
        return false;
    }

    OutCombatant = InitiativeOrder[CurrentInitiativeIndex];
    return true;
}

bool UGridTurnManagerComponent::IsActivePlayerCharacter (
    int32 CharacterIndex) const
{
    FGridCombatantInitiativeEntry ActiveCombatant;
    return GetActiveCombatant (ActiveCombatant) &&
        ActiveCombatant.Side == EGridCombatantSide::Party &&
        ActiveCombatant.CharacterIndex == CharacterIndex;
}

FGuid UGridTurnManagerComponent::ResolvePlayerCombatantId (
    int32 CharacterIndex) const
{
    if (IsValid (PartyPawn) &&
        IsValid (PartyPawn->PartyInventoryComponent))
    {
        const TArray<FGridCharacterInventoryState>& Characters =
            PartyPawn->PartyInventoryComponent->PartyInventoryState
                .ActiveCharacters;
        if (Characters.IsValidIndex (CharacterIndex) &&
            Characters[CharacterIndex].CharacterId.IsValid ())
        {
            return Characters[CharacterIndex].CharacterId;
        }
    }

    return FGuid (
        0x50415254,
        0,
        0,
        static_cast<uint32> (FMath::Max (0, CharacterIndex) + 1));
}

AGridMonsterActor* UGridTurnManagerComponent::FindCombatMonsterById (
    const FGuid& CombatantId) const
{
    const TObjectPtr<AGridMonsterActor>* Found =
        CombatMonsters.FindByPredicate (
            [&CombatantId] (
                const TObjectPtr<AGridMonsterActor>& Monster)
            {
                return IsValid (Monster) &&
                    Monster->ResolvePersistenceId () == CombatantId;
            });
    return Found ? Found->Get () : nullptr;
}

FGridCombatantInitiativeEntry*
UGridTurnManagerComponent::FindInitiativeEntry (
    EGridCombatantSide Side,
    const FGuid& CombatantId)
{
    return InitiativeOrder.FindByPredicate (
        [Side, &CombatantId] (
            const FGridCombatantInitiativeEntry& Entry)
        {
            return Entry.Side == Side &&
                Entry.CombatantId == CombatantId;
        });
}

const FGridCombatantInitiativeEntry*
UGridTurnManagerComponent::FindInitiativeEntry (
    EGridCombatantSide Side,
    const FGuid& CombatantId) const
{
    return InitiativeOrder.FindByPredicate (
        [Side, &CombatantId] (
            const FGridCombatantInitiativeEntry& Entry)
        {
            return Entry.Side == Side &&
                Entry.CombatantId == CombatantId;
        });
}
