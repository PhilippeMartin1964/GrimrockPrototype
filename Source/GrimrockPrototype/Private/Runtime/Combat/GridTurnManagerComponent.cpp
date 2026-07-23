#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"

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
}

UGridTurnManagerComponent::UGridTurnManagerComponent ()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
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

    UnbindCurrentMovement ();
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

            if (CurrentMovementComponent && CurrentMovementComponent->IsBusy ())
            {
                CurrentMovementComponent->CancelCurrentAction ();
            }
            CompleteActiveAction (false);
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

    UnbindCurrentMovement ();
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
        TEXT ("[GridTurnManager] Initialized=%s Active=%s Phase=%s Round=%d Monsters=%d EnemyIndex=%d Current=%s AP=%d/%d Pending=%d ActiveAction=%s Timeout=%.2f InputLocked=%s"),
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
        bPartyInputLocked ? TEXT ("true") : TEXT ("false"));
}

bool UGridTurnManagerComponent::StartCombatInternal (const TArray<AGridMonsterActor*>& Monsters)
{
    if (!bInitialized && !InitializeTurnManager (nullptr, nullptr))
    {
        return false;
    }

    if (bCombatActive)
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
        if (IsValid (Monster) && !Monster->IsDead ())
        {
            OutMonsters.Add (Monster);
        }
    }
}

void UGridTurnManagerComponent::CollectPerceivingMonsters (TArray<AGridMonsterActor*>& OutMonsters)
{
    TArray<AGridMonsterActor*> AllMonsters;
    CollectAllLivingMonsters (AllMonsters);
    OutMonsters.Reset ();

    for (AGridMonsterActor* Monster : AllMonsters)
    {
        if (!PrepareMonsterForCombat (Monster))
        {
            continue;
        }

        UGridMonsterBehaviorComponent* Behavior =
            Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ();
        if (Behavior && Behavior->RefreshPerception ())
        {
            OutMonsters.Add (Monster);
        }
    }
}

bool UGridTurnManagerComponent::PrepareMonsterForCombat (AGridMonsterActor* Monster)
{
    if (!IsValid (Monster) || Monster->IsDead () || !IsValid (Monster->MonsterDefinition))
    {
        return false;
    }

    if (!Monster->SpawnObjectId.IsValid ())
    {
        Monster->SpawnObjectId = FGuid::NewGuid ();
    }

    UGridMonsterMovementComponent* Movement =
        Monster->FindComponentByClass<UGridMonsterMovementComponent> ();
    UGridMonsterBehaviorComponent* Behavior =
        Monster->FindComponentByClass<UGridMonsterBehaviorComponent> ();

    if (!Movement || !Behavior)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Monster %s requires MonsterMovement and MonsterBehavior components."),
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
