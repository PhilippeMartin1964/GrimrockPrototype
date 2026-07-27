#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterFastHarasserPlanner.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace
{
    FString GetCombatPhaseText (EGridCombatPhase Phase)
    {
        if (const UEnum* Enum = StaticEnum<EGridCombatPhase> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Phase));
        }
        return TEXT ("Unknown");
    }

    FString GetActionTypeText (EGridCombatActionType Type)
    {
        if (const UEnum* Enum = StaticEnum<EGridCombatActionType> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Type));
        }
        return TEXT ("Unknown");
    }

    FString GetCombatLogEntryTypeText (EGridCombatLogEntryType Type)
    {
        if (const UEnum* Enum = StaticEnum<EGridCombatLogEntryType> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Type));
        }
        return TEXT ("Unknown");
    }

    bool HasStableMonsterId (const AGridMonsterActor* Monster)
    {
        if (IsValid (Monster) &&
            Monster->ResolvePersistenceId ().IsValid ())
        {
            return true;
        }

        UE_LOG (LogGridMonsterState, Error,
            TEXT ("[GridMonsterState] Combat participation skipped Monster=%s Reason=InvalidPersistenceId"),
            *GetNameSafe (Monster));
        return false;
    }

    FString GetStableMonsterSortKey (const AGridMonsterActor* Monster)
    {
        if (Monster)
        {
            const FGuid PersistenceId =
                Monster->ResolvePersistenceId ();
            if (PersistenceId.IsValid ())
            {
                return PersistenceId.ToString (
                    EGuidFormats::Digits);
            }
        }
        return GetPathNameSafe (Monster);
    }
}

UGridTurnManagerComponent::UGridTurnManagerComponent ()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    CombatRandomStream.Initialize (EncounterRandomSeed);
}

void UGridTurnManagerComponent::BeginPlay ()
{
    Super::BeginPlay ();

    if (bAutoInitialize)
    {
        InitializeTurnManager (nullptr, nullptr);
    }
}

void UGridTurnManagerComponent::EndPlay (const EEndPlayReason::Type EndPlayReason)
{
    if (CurrentMovementComponent && CurrentMovementComponent->IsBusy ())
    {
        CurrentMovementComponent->CancelCurrentAction ();
    }
    if (CurrentCombatComponent)
    {
        CurrentCombatComponent->CancelAttackPresentation ();
    }

    UnbindCurrentMovement ();
    UnbindCurrentCombat ();
    UnbindCombatMonsterDeaths ();
    SetPartyInputLocked (false);
    Super::EndPlay (EndPlayReason);
}

void UGridTurnManagerComponent::TickComponent (
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent (DeltaTime, TickType, ThisTickFunction);

    const float SafeDeltaTime = FMath::Max (0.0f, DeltaTime);

    if (bWaitingForCombatStart)
    {
        CombatStartDelayRemaining -= SafeDeltaTime;
        if (CombatStartDelayRemaining <= 0.0f)
        {
            bWaitingForCombatStart = false;
            CombatStartDelayRemaining = 0.0f;
            BeginRound ();
        }
    }

    if (bHasActiveAction && ActiveAction.Type == EGridCombatActionType::MeleeAttack)
    {
        if (!bActiveAttackImpactCommitted && ActiveAttackImpactTimeRemaining > 0.0f)
        {
            ActiveAttackImpactTimeRemaining -= SafeDeltaTime;
            if (ActiveAttackImpactTimeRemaining <= 0.0f)
            {
                NotifyActiveAttackImpact ();
            }
        }

        if (bHasActiveAction && ActiveAttackCompleteTimeRemaining > 0.0f)
        {
            ActiveAttackCompleteTimeRemaining -= SafeDeltaTime;
            if (ActiveAttackCompleteTimeRemaining <= 0.0f)
            {
                NotifyActiveAttackComplete ();
            }
        }
    }

    if (bHasActiveAction && ActiveActionTimeoutRemaining > 0.0f)
    {
        ActiveActionTimeoutRemaining -= SafeDeltaTime;
        if (ActiveActionTimeoutRemaining <= 0.0f)
        {
            UE_LOG (LogTemp, Error,
                TEXT ("[GridTurnManager] Action timeout. Monster=%s Action=%s Target=(%d,%d)"),
                *GetNameSafe (CurrentMonster),
                *GetActionTypeText (ActiveAction.Type),
                ActiveAction.TargetCell.X,
                ActiveAction.TargetCell.Y);

            if (ActiveAction.Type == EGridCombatActionType::MeleeAttack)
            {
                CommitActiveAttackImpact ();
                if (CurrentCombatComponent)
                {
                    CurrentCombatComponent->CancelAttackPresentation ();
                }
                CompleteActiveAction (ActiveAction.bOutcomeCommitted);
            }
            else
            {
                if (CurrentMovementComponent && CurrentMovementComponent->IsBusy ())
                {
                    CurrentMovementComponent->CancelCurrentAction ();
                }
                CompleteActiveAction (false);
            }
        }
    }

    RefreshTickEnabled ();
}

bool UGridTurnManagerComponent::InitializeTurnManager (
    AGridLevelRuntimeActor* InRuntimeActor,
    AGrimrockPartyPawn* InPartyPawn)
{
    AGridLevelRuntimeActor* CandidateRuntime = IsValid (InRuntimeActor)
        ? InRuntimeActor
        : Cast<AGridLevelRuntimeActor> (GetOwner ());
    if (!IsValid (CandidateRuntime))
    {
        CandidateRuntime = FindRuntimeActor ();
    }

    AGrimrockPartyPawn* CandidateParty = IsValid (InPartyPawn)
        ? InPartyPawn
        : FindPartyPawn ();

    if (!IsValid (CandidateRuntime) || !IsValid (CandidateParty))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Initialization failed. Owner=%s Runtime=%s Party=%s"),
            *GetNameSafe (GetOwner ()),
            *GetNameSafe (CandidateRuntime),
            *GetNameSafe (CandidateParty));
        return false;
    }

    RuntimeActor = CandidateRuntime;
    PartyPawn = CandidateParty;
    bInitialized = true;
    CurrentPhase = PhaseState.GetPhase ();
    RoundNumber = PhaseState.GetRoundNumber ();
    return true;
}

bool UGridTurnManagerComponent::StartCombatFromPerception ()
{
    TArray<AGridMonsterActor*> Monsters;
    CollectPerceivingMonsters (Monsters);
    return StartCombatInternal (Monsters);
}

bool UGridTurnManagerComponent::StartCombatWithAllMonsters ()
{
    TArray<AGridMonsterActor*> Monsters;
    CollectAllLivingMonsters (Monsters);
    return StartCombatInternal (Monsters);
}

bool UGridTurnManagerComponent::EndPlayerPhase ()
{
    if (!bCombatActive || CurrentPhase != EGridCombatPhase::PlayerPhase || !IsPartyAtRest ())
    {
        return false;
    }

    if (!PhaseState.EndPlayerPhase ())
    {
        return false;
    }

    SetPartyInputLocked (true);
    SetPhase (PhaseState.GetPhase ());
    BeginEnemyPhase ();
    return true;
}

void UGridTurnManagerComponent::AbortCombat ()
{
    if (CurrentMovementComponent && CurrentMovementComponent->IsBusy ())
    {
        CurrentMovementComponent->CancelCurrentAction ();
    }
    if (CurrentCombatComponent)
    {
        CurrentCombatComponent->CancelAttackPresentation ();
    }

    UnbindCurrentMovement ();
    UnbindCurrentCombat ();
    UnbindCombatMonsterDeaths ();
    bWaitingForCombatStart = false;
    CombatStartDelayRemaining = 0.0f;
    ActiveActionTimeoutRemaining = 0.0f;
    bHasActiveAction = false;
    ActiveAction = FGridCombatAction ();
    PendingActions.Reset ();
    EnemyTurnOrder.Reset ();
    CombatMonsters.Reset ();
    CurrentMonster = nullptr;
    CurrentEnemyIndex = INDEX_NONE;
    CurrentMonsterMaximumActionPoints = 0;
    CurrentMonsterRemainingActionPoints = 0;
    ActionPointBudget.Reset (0);
    ResetActiveAttackState ();
    bCombatActive = false;

    PhaseState.AbortCombat ();
    RoundNumber = PhaseState.GetRoundNumber ();
    SetPhase (PhaseState.GetPhase ());
    SetPartyInputLocked (false);
    RefreshTickEnabled ();
}

void UGridTurnManagerComponent::ForceVictory ()
{
    FinishCombat (EGridCombatPhase::Victory);
}

void UGridTurnManagerComponent::ForceDefeat ()
{
    FinishCombat (EGridCombatPhase::Defeat);
}

void UGridTurnManagerComponent::LogCurrentTurnState () const
{
    UE_LOG (LogTemp, Log,
        TEXT ("[GridTurnManager] Initialized=%s Active=%s Phase=%s Round=%d Monsters=%d EnemyIndex=%d Current=%s AP=%d/%d Pending=%d ActiveAction=%s Timeout=%.2f InputLocked=%s Attack=%s Target=%d Impact=%s"),
        bInitialized ? TEXT ("true") : TEXT ("false"),
        bCombatActive ? TEXT ("true") : TEXT ("false"),
        *GetCombatPhaseText (CurrentPhase),
        RoundNumber,
        CombatMonsters.Num (),
        CurrentEnemyIndex,
        *GetNameSafe (CurrentMonster),
        CurrentMonsterRemainingActionPoints,
        CurrentMonsterMaximumActionPoints,
        PendingActions.Num (),
        bHasActiveAction ? *GetActionTypeText (ActiveAction.Type) : TEXT ("None"),
        ActiveActionTimeoutRemaining,
        bPartyInputLocked ? TEXT ("true") : TEXT ("false"),
        *ActiveAttackDefinition.AttackId.ToString (),
        LastTargetCharacterIndex,
        bActiveAttackImpactCommitted ? TEXT ("true") : TEXT ("false"));
}

void UGridTurnManagerComponent::LogPartyCombatState () const
{
    if (!IsValid (PartyPawn) || !IsValid (PartyPawn->PartyInventoryComponent))
    {
        UE_LOG (LogTemp, Warning, TEXT ("[GridTurnManager] Party combat state unavailable."));
        return;
    }

    const TArray<FGridCharacterInventoryState>& Characters =
        PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
    UE_LOG (LogTemp, Log,
        TEXT ("[GridTurnManager] PartyCombat Characters=%d Living=%s"),
        Characters.Num (),
        HasLivingPartyCharacter () ? TEXT ("true") : TEXT ("false"));

    for (int32 Index = 0; Index < Characters.Num (); ++Index)
    {
        const FGridCharacterInventoryState& Character = Characters[Index];
        UE_LOG (LogTemp, Log,
            TEXT ("[GridTurnManager] PartyCharacter Index=%d Name=%s HP=%d/%d PhysicalArmor=%d MagicalArmor=%d Evasion=%d"),
            Index,
            *Character.DisplayName.ToString (),
            Character.DerivedStats.CurrentHealth,
            Character.DerivedStats.MaxHealth,
            Character.DerivedStats.PhysicalArmor,
            Character.DerivedStats.MagicalArmor,
            Character.DerivedStats.Evasion);
    }
}

bool UGridTurnManagerComponent::GetLatestCombatLogEntry (
    FGridCombatLogEntry& OutEntry) const
{
    if (CombatLogEntries.IsEmpty ())
    {
        OutEntry = FGridCombatLogEntry ();
        return false;
    }

    OutEntry = CombatLogEntries.Last ();
    return true;
}

void UGridTurnManagerComponent::ClearCombatLog ()
{
    CombatLogEntries.Reset ();
    NextCombatLogSequenceNumber = 1;
    CombatLogBroadcastCount = 0;
}

void UGridTurnManagerComponent::LogCombatHistory () const
{
    for (const FGridCombatLogEntry& Entry : CombatLogEntries)
    {
        UE_LOG (
            LogGridCombat,
            Log,
            TEXT ("[GridCombat] History Seq=%d Round=%d Phase=%s Type=%s Message=\"%s\""),
            Entry.SequenceNumber,
            Entry.RoundNumber,
            *GetCombatPhaseText (Entry.Phase),
            *GetCombatLogEntryTypeText (Entry.Type),
            *Entry.Message.ToString ());
    }
}

void UGridTurnManagerComponent::AppendCombatLogEntry (
    FGridCombatLogEntry Entry)
{
    Entry.SequenceNumber = NextCombatLogSequenceNumber++;
    CombatLogEntries.Add (Entry);

    const int32 SafeCapacity = FMath::Clamp (
        MaxCombatLogEntries,
        1,
        512);
    const int32 ExcessEntryCount =
        CombatLogEntries.Num () - SafeCapacity;
    if (ExcessEntryCount > 0)
    {
        CombatLogEntries.RemoveAt (0, ExcessEntryCount);
    }

    UE_LOG (
        LogGridCombat,
        Log,
        TEXT ("[GridCombat] Seq=%d Round=%d Phase=%s Type=%s Message=\"%s\""),
        Entry.SequenceNumber,
        Entry.RoundNumber,
        *GetCombatPhaseText (Entry.Phase),
        *GetCombatLogEntryTypeText (Entry.Type),
        *Entry.Message.ToString ());

    ++CombatLogBroadcastCount;
    OnCombatLogEntryAdded.Broadcast (Entry);
}

FName UGridTurnManagerComponent::ResolveMonsterLogId (
    const AGridMonsterActor* Monster) const
{
    if (IsValid (Monster) &&
        IsValid (Monster->MonsterDefinition) &&
        !Monster->MonsterDefinition->MonsterId.IsNone ())
    {
        return Monster->MonsterDefinition->MonsterId;
    }

    return IsValid (Monster)
        ? FName (*Monster->GetName ())
        : NAME_None;
}

FText UGridTurnManagerComponent::ResolveMonsterDisplayName (
    const AGridMonsterActor* Monster) const
{
    if (IsValid (Monster) && IsValid (Monster->MonsterDefinition))
    {
        if (!Monster->MonsterDefinition->DisplayName.IsEmpty ())
        {
            return Monster->MonsterDefinition->DisplayName;
        }
        if (!Monster->MonsterDefinition->MonsterId.IsNone ())
        {
            return FText::FromName (
                Monster->MonsterDefinition->MonsterId);
        }
    }

    return FText::FromString (GetNameSafe (Monster));
}

FText UGridTurnManagerComponent::ResolveCharacterDisplayName (
    int32 CharacterIndex) const
{
    if (IsValid (PartyPawn) &&
        IsValid (PartyPawn->PartyInventoryComponent) &&
        PartyPawn->PartyInventoryComponent->PartyInventoryState
            .ActiveCharacters.IsValidIndex (CharacterIndex))
    {
        const FText& DisplayName =
            PartyPawn->PartyInventoryComponent->PartyInventoryState
                .ActiveCharacters[CharacterIndex].DisplayName;
        if (!DisplayName.IsEmpty ())
        {
            return DisplayName;
        }
    }

    return FText::Format (
        NSLOCTEXT (
            "GridCombatLog",
            "CharacterFallback",
            "Personnage {0}"),
        FText::AsNumber (CharacterIndex));
}

bool UGridTurnManagerComponent::StartCombatInternal (const TArray<AGridMonsterActor*>& Monsters)
{
    if (!bInitialized && !InitializeTurnManager (nullptr, nullptr))
    {
        return false;
    }

    if (bCombatActive || !HasLivingPartyCharacter ())
    {
        return false;
    }

    if (CurrentPhase == EGridCombatPhase::Victory || CurrentPhase == EGridCombatPhase::Defeat)
    {
        PhaseState.AbortCombat ();
        SetPhase (PhaseState.GetPhase ());
    }

    CombatMonsters.Reset ();
    for (AGridMonsterActor* Monster : Monsters)
    {
        const bool bAlreadyAdded = CombatMonsters.ContainsByPredicate (
            [Monster] (const TObjectPtr<AGridMonsterActor>& Existing)
            {
                return Existing.Get () == Monster;
            });
        if (!PrepareMonsterForCombat (Monster) || bAlreadyAdded)
        {
            continue;
        }
        CombatMonsters.Add (Monster);
    }

    if (CombatMonsters.IsEmpty () || !PhaseState.StartCombat ())
    {
        CombatMonsters.Reset ();
        return false;
    }

    ClearCombatLog ();
    AttackResolvedBroadcastCount = 0;
    LoggedDefeatedMonsterIds.Reset ();
    FGridCombatLogEntry CombatStartedEntry;
    CombatStartedEntry.RoundNumber = PhaseState.GetRoundNumber ();
    CombatStartedEntry.Phase = PhaseState.GetPhase ();
    CombatStartedEntry.Type = EGridCombatLogEntryType::CombatStarted;
    CombatStartedEntry.Message =
        FGridCombatLogFormatter::FormatCombatStarted ();
    AppendCombatLogEntry (CombatStartedEntry);

    PendingActions.Reset ();
    EnemyTurnOrder.Reset ();
    CurrentMonster = nullptr;
    CurrentEnemyIndex = INDEX_NONE;
    CurrentMonsterMaximumActionPoints = 0;
    CurrentMonsterRemainingActionPoints = 0;
    bHasActiveAction = false;
    ActiveAction = FGridCombatAction ();
    ActiveActionTimeoutRemaining = 0.0f;
    ActionPointBudget.Reset (0);
    ResetActiveAttackState ();
    CombatRandomStream.Initialize (EncounterRandomSeed);

    bCombatActive = true;
    BindCombatMonsterDeaths ();
    for (AGridMonsterActor* Monster : CombatMonsters)
    {
        if (IsValid (Monster) && Monster->AudioComponent)
        {
            Monster->AudioComponent->PlayAlert ();
        }
    }
    RoundNumber = PhaseState.GetRoundNumber ();
    SetPhase (PhaseState.GetPhase ());
    SetPartyInputLocked (true);

    CombatStartDelayRemaining = CalculateCombatStartDelay ();
    bWaitingForCombatStart = CombatStartDelayRemaining > KINDA_SMALL_NUMBER;
    if (!bWaitingForCombatStart)
    {
        BeginRound ();
    }

    RefreshTickEnabled ();
    return true;
}

void UGridTurnManagerComponent::BindCombatMonsterDeaths ()
{
    for (AGridMonsterActor* Monster : CombatMonsters)
    {
        if (IsValid (Monster))
        {
            Monster->OnMonsterDied.AddUniqueDynamic (
                this,
                &UGridTurnManagerComponent::HandleCombatMonsterDied);
        }
    }
}

void UGridTurnManagerComponent::UnbindCombatMonsterDeaths ()
{
    for (AGridMonsterActor* Monster : CombatMonsters)
    {
        if (IsValid (Monster))
        {
            Monster->OnMonsterDied.RemoveDynamic (
                this,
                &UGridTurnManagerComponent::HandleCombatMonsterDied);
        }
    }
}

void UGridTurnManagerComponent::HandleCombatMonsterDied (
    AGridMonsterActor* Monster,
    FIntPoint DeathCell)
{
    (void)DeathCell;
    if (!bCombatActive || !IsValid (Monster) ||
        !CombatMonsters.ContainsByPredicate (
            [Monster] (const TObjectPtr<AGridMonsterActor>& Candidate)
            {
                return Candidate.Get () == Monster;
            }))
    {
        return;
    }

    const FGuid MonsterPersistenceId = Monster->ResolvePersistenceId ();
    const bool bAlreadyLogged =
        MonsterPersistenceId.IsValid () &&
        LoggedDefeatedMonsterIds.Contains (MonsterPersistenceId);
    if (!bAlreadyLogged)
    {
        if (MonsterPersistenceId.IsValid ())
        {
            LoggedDefeatedMonsterIds.Add (MonsterPersistenceId);
        }

        FGridCombatLogEntry DefeatedEntry;
        DefeatedEntry.RoundNumber = RoundNumber;
        DefeatedEntry.Phase = CurrentPhase;
        DefeatedEntry.Type =
            EGridCombatLogEntryType::MonsterDefeated;
        DefeatedEntry.SourceId = ResolveMonsterLogId (Monster);
        DefeatedEntry.SourceDisplayName =
            ResolveMonsterDisplayName (Monster);
        DefeatedEntry.Message =
            FGridCombatLogFormatter::FormatMonsterDefeated (
                DefeatedEntry.SourceDisplayName);
        AppendCombatLogEntry (DefeatedEntry);
    }

    const bool bWasCurrentMonster = CurrentMonster == Monster;
    const bool bWasEnemyPhase = CurrentPhase == EGridCombatPhase::EnemyPhase;
    PendingActions.RemoveAll (
        [Monster] (const FGridCombatAction& Action)
        {
            return Action.SourceActorId ==
                Monster->ResolvePersistenceId ();
        });

    const int32 DeadTurnIndex = EnemyTurnOrder.IndexOfByPredicate (
        [Monster] (const TObjectPtr<AGridMonsterActor>& Candidate)
        {
            return Candidate.Get () == Monster;
        });
    if (DeadTurnIndex != INDEX_NONE)
    {
        EnemyTurnOrder.RemoveAt (DeadTurnIndex);
        if (DeadTurnIndex <= CurrentEnemyIndex)
        {
            --CurrentEnemyIndex;
        }
    }

    if (bWasCurrentMonster)
    {
        if (CurrentMovementComponent)
        {
            CurrentMovementComponent->CancelCurrentAction ();
        }
        if (CurrentCombatComponent)
        {
            CurrentCombatComponent->CancelAttackPresentation ();
        }
        UnbindCurrentMovement ();
        UnbindCurrentCombat ();
        PendingActions.Reset ();
        bHasActiveAction = false;
        ActiveAction = FGridCombatAction ();
        ActiveActionTimeoutRemaining = 0.0f;
        ResetActiveAttackState ();
        CurrentMonster = nullptr;
    }

    int32 RemainingLiving = 0;
    for (const AGridMonsterActor* Candidate : CombatMonsters)
    {
        RemainingLiving += IsValid (Candidate) && !Candidate->IsDead () ? 1 : 0;
    }
    const bool bVictory = RemainingLiving == 0;
    UE_LOG (LogTemp, Log,
        TEXT ("[GridTurnManager] MonsterDied Monster=%s RemainingLiving=%d Victory=%s"),
        *GetNameSafe (Monster),
        RemainingLiving,
        bVictory ? TEXT ("true") : TEXT ("false"));

    if (bVictory)
    {
        FinishCombat (EGridCombatPhase::Victory);
    }
    else if (bWasCurrentMonster && bWasEnemyPhase)
    {
        BeginNextMonsterTurn ();
    }
}

void UGridTurnManagerComponent::CollectAllLivingMonsters (TArray<AGridMonsterActor*>& OutMonsters) const
{
    OutMonsters.Reset ();
    UWorld* World = GetWorld ();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AGridMonsterActor> It (World); It; ++It)
    {
        AGridMonsterActor* Monster = *It;
        if (IsValid (Monster) &&
            !Monster->IsDead () &&
            Monster->bMonsterEnabled &&
            Monster->IsRuntimeLevelActive () &&
            HasStableMonsterId (Monster))
        {
            OutMonsters.Add (Monster);
        }
    }

    OutMonsters.Sort ([] (
        const AGridMonsterActor& Left,
        const AGridMonsterActor& Right)
    {
        return GetStableMonsterSortKey (&Left) < GetStableMonsterSortKey (&Right);
    });
}

void UGridTurnManagerComponent::CollectPerceivingMonsters (TArray<AGridMonsterActor*>& OutMonsters)
{
    TArray<AGridMonsterActor*> AllMonsters;
    CollectAllLivingMonsters (AllMonsters);
    OutMonsters.Reset ();

    TArray<AGridMonsterActor*> PreparedMonsters;
    TArray<AGridMonsterActor*> DirectSources;
    for (AGridMonsterActor* Monster : AllMonsters)
    {
        if (!PrepareMonsterForCombat (Monster))
        {
            continue;
        }
        PreparedMonsters.Add (Monster);

        UGridMonsterBehaviorComponent* Behavior =
            Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ();
        if (Behavior && Behavior->RefreshPerception ())
        {
            DirectSources.Add (Monster);
            OutMonsters.AddUnique (Monster);
        }
    }

    TArray<FGridMonsterAggroCandidate> AggroCandidates;
    AggroCandidates.Reserve (PreparedMonsters.Num ());
    for (const AGridMonsterActor* Monster : PreparedMonsters)
    {
        FGridMonsterAggroCandidate Candidate;
        Candidate.SpawnObjectId =
            Monster->ResolvePersistenceId ();
        Candidate.MonsterId = Monster->MonsterDefinition
            ? Monster->MonsterDefinition->MonsterId
            : NAME_None;
        Candidate.EncounterGroupId = Monster->EncounterGroupId;
        Candidate.Cell = Monster->CurrentCell;
        Candidate.bIsAlive = !Monster->IsDead ();
        Candidate.bIsEnabled = Monster->bMonsterEnabled;
        AggroCandidates.Add (Candidate);
    }

    // MON7 deliberately performs one deterministic propagation wave.
    for (AGridMonsterActor* Source : DirectSources)
    {
        if (!IsValid (Source) ||
            !IsValid (Source->MonsterDefinition) ||
            !Source->MonsterDefinition->bSharesAggroWithGroup ||
            Source->EncounterGroupId.IsNone ())
        {
            continue;
        }

        TArray<FGuid> TargetIds;
        FGridFastHarasserPlanner::SelectAggroTargets (
            Source->ResolvePersistenceId (),
            Source->MonsterDefinition->MonsterId,
            Source->EncounterGroupId,
            Source->CurrentCell,
            Source->MonsterDefinition->AggroPropagationRange,
            AggroCandidates,
            TargetIds);

        for (const FGuid& TargetId : TargetIds)
        {
            AGridMonsterActor** TargetPtr = PreparedMonsters.FindByPredicate (
                [&TargetId] (const AGridMonsterActor* Candidate)
                {
                    return IsValid (Candidate) &&
                        Candidate->ResolvePersistenceId () == TargetId;
                });
            AGridMonsterActor* Target = TargetPtr ? *TargetPtr : nullptr;
            if (!IsValid (Target) || Target->IsDead () || !Target->bMonsterEnabled)
            {
                continue;
            }

            const bool bAlreadyParticipating = OutMonsters.Contains (Target);
            if (!bAlreadyParticipating)
            {
                Target->SetMonsterState (EGridMonsterState::Alert);
                OutMonsters.Add (Target);
            }

            UE_LOG (LogTemp, Log,
                TEXT ("[GridMonsterAggro] Source=%s Group=%s Propagated=%s Distance=%d"),
                *GetNameSafe (Source),
                *Source->EncounterGroupId.ToString (),
                *GetNameSafe (Target),
                FGridMonsterPathfinder::ManhattanDistance (
                    Source->CurrentCell,
                    Target->CurrentCell));
        }
    }
}

bool UGridTurnManagerComponent::PrepareMonsterForCombat (AGridMonsterActor* Monster)
{
    if (!IsValid (Monster) ||
        Monster->IsDead () ||
        !Monster->bMonsterEnabled ||
        !Monster->IsRuntimeLevelActive () ||
        !IsValid (Monster->MonsterDefinition))
    {
        return false;
    }

    if (!HasStableMonsterId (Monster))
    {
        return false;
    }
    if (IsValid (RuntimeActor))
    {
        RuntimeActor->ApplyMonsterPlacementMetadata (Monster);
    }

    UGridMonsterMovementComponent* Movement =
        Monster->FindComponentByClass<UGridMonsterMovementComponent> ();
    UGridMonsterBehaviorComponent* Behavior =
        Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ();
    UGridMonsterCombatComponent* Combat =
        Monster->FindComponentByClass<UGridMonsterCombatComponent> ();

    if (!Movement || !Behavior || !Combat)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Monster %s requires MonsterMovement, MonsterBehavior and MonsterCombat components."),
            *GetNameSafe (Monster));
        return false;
    }

    if (!Movement->IsInitialized () && !Movement->InitializeMovement (RuntimeActor))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Movement initialization failed for %s."),
            *GetNameSafe (Monster));
        return false;
    }

    if (!Behavior->IsInitialized () && !Behavior->InitializeBehavior (RuntimeActor, PartyPawn))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Behavior initialization failed for %s."),
            *GetNameSafe (Monster));
        return false;
    }

    if (!Combat->IsInitialized () && !Combat->InitializeCombat (PartyPawn))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Combat initialization failed for %s."),
            *GetNameSafe (Monster));
        return false;
    }

    return true;
}

void UGridTurnManagerComponent::SetPhase (EGridCombatPhase NewPhase)
{
    if (CurrentPhase == NewPhase)
    {
        return;
    }

    CurrentPhase = NewPhase;
    if (CurrentPhase != EGridCombatPhase::Victory &&
        CurrentPhase != EGridCombatPhase::Defeat)
    {
        FGridCombatLogEntry PhaseEntry;
        PhaseEntry.RoundNumber = RoundNumber;
        PhaseEntry.Phase = CurrentPhase;
        PhaseEntry.Type = EGridCombatLogEntryType::PhaseChanged;
        PhaseEntry.Message =
            FGridCombatLogFormatter::FormatPhaseChanged (CurrentPhase);
        AppendCombatLogEntry (PhaseEntry);
    }

    if (bLogPhaseChanges)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("[GridTurnManager] Phase=%s Round=%d"),
            *GetCombatPhaseText (CurrentPhase),
            RoundNumber);
    }
    OnPhaseChanged.Broadcast (CurrentPhase);
}

void UGridTurnManagerComponent::SetPartyInputLocked (bool bLocked)
{
    if (bPartyInputLocked == bLocked)
    {
        return;
    }

    if (!IsValid (PartyPawn))
    {
        bPartyInputLocked = bLocked;
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController> (PartyPawn->GetController ());
    if (!PlayerController && GetWorld ())
    {
        PlayerController = GetWorld ()->GetFirstPlayerController ();
    }

    if (PlayerController)
    {
        if (bLocked)
        {
            PartyPawn->DisableInput (PlayerController);
        }
        else
        {
            PartyPawn->EnableInput (PlayerController);
        }
    }

    bPartyInputLocked = bLocked;
}

float UGridTurnManagerComponent::CalculateCombatStartDelay () const
{
    if (!IsValid (PartyPawn))
    {
        return FMath::Max (0.0f, CombatStartSafetyPadding);
    }

    return FMath::Max3 (
        FMath::Max (0.0f, PartyPawn->MoveDuration),
        FMath::Max (0.0f, PartyPawn->TurnDuration),
        FMath::Max (0.0f, PartyPawn->InputBufferMaxAge)) +
        FMath::Max (0.0f, CombatStartSafetyPadding);
}

AGridLevelRuntimeActor* UGridTurnManagerComponent::FindRuntimeActor () const
{
    if (UWorld* World = GetWorld ())
    {
        for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
        {
            return *It;
        }
    }
    return nullptr;
}

AGrimrockPartyPawn* UGridTurnManagerComponent::FindPartyPawn () const
{
    if (UWorld* World = GetWorld ())
    {
        for (TActorIterator<AGrimrockPartyPawn> It (World); It; ++It)
        {
            return *It;
        }
    }
    return nullptr;
}
