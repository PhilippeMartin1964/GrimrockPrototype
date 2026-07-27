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
    if (!bInitialized && !InitializeTurnManager (nullptr, nullpt