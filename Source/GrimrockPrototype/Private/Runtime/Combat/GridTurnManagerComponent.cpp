#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Crc.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
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

    void EnsureStableMonsterId (AGridMonsterActor* Monster)
    {
        if (!IsValid (Monster) || Monster->SpawnObjectId.IsValid ())
        {
            return;
        }

        const FString StablePath = Monster->GetPathName ();
        Monster->SpawnObjectId = FGuid (
            FCrc::StrCrc32 (*StablePath),
            FCrc::StrCrc32 (*(StablePath + TEXT ("|MON7-B"))),
            FCrc::StrCrc32 (*(StablePath + TEXT ("|MON7-C"))),
            FCrc::StrCrc32 (*(StablePath + TEXT ("|MON7-D"))));
        if (!Monster->SpawnObjectId.IsValid ())
        {
            Monster->SpawnObjectId = FGuid::NewGuid ();
        }
    }

    FString GetStableMonsterSortKey (const AGridMonsterActor* Monster)
    {
        if (Monster && Monster->SpawnObjectId.IsValid ())
        {
            return Monster->SpawnObjectId.ToString (EGuidFormats::Digits);
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
        if (IsValid (Monster) && !Monster->IsDead () && Monster->bMonsterEnabled)
        {
            EnsureStableMonsterId (Monster);
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
        Candidate.SpawnObjectId = Monster->SpawnObjectId;
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
            Source->SpawnObjectId,
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
                        Candidate->SpawnObjectId == TargetId;
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
        !IsValid (Monster->MonsterDefinition))
    {
        return false;
    }

    EnsureStableMonsterId (Monster);
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
