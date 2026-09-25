#include "Runtime/Monsters/GridMonsterPatrolSubsystem.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridAutomaticPerceptionEngagementSubsystem.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

DEFINE_LOG_CATEGORY(LogGridMonsterPatrol);

bool FGridMonsterPatrolCursorRules::Initialize(const FIntPoint& CurrentCell, EGridMonsterPatrolMode PatrolMode,
	const TArray<FGridMonsterPatrolWaypoint>& Waypoints, int32& OutTargetWaypointIndex, int32& OutPingPongDirection)
{
	OutTargetWaypointIndex = INDEX_NONE;
	OutPingPongDirection = 1;
	if (PatrolMode == EGridMonsterPatrolMode::None || Waypoints.Num() < 2)
	{
		return false;
	}

	int32 BestDistance = MAX_int32;
	for (int32 Index = 0; Index < Waypoints.Num(); ++Index)
	{
		if (Waypoints[Index].Cell == CurrentCell)
		{
			OutTargetWaypointIndex = Index;
			break;
		}
		const int32 Distance = FGridMonsterPathfinder::ManhattanDistance(CurrentCell, Waypoints[Index].Cell);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			OutTargetWaypointIndex = Index;
		}
	}

	if (PatrolMode == EGridMonsterPatrolMode::PingPong && OutTargetWaypointIndex == Waypoints.Num() - 1)
	{
		OutPingPongDirection = -1;
	}
	return OutTargetWaypointIndex != INDEX_NONE;
}

bool FGridMonsterPatrolCursorRules::Advance(
	EGridMonsterPatrolMode PatrolMode, int32 WaypointCount, int32 ArrivedWaypointIndex, int32& InOutPingPongDirection, int32& OutNextWaypointIndex)
{
	OutNextWaypointIndex = INDEX_NONE;
	if (WaypointCount < 2 || ArrivedWaypointIndex < 0 || ArrivedWaypointIndex >= WaypointCount || PatrolMode == EGridMonsterPatrolMode::None)
	{
		return false;
	}

	if (PatrolMode == EGridMonsterPatrolMode::Loop)
	{
		OutNextWaypointIndex = (ArrivedWaypointIndex + 1) % WaypointCount;
		return true;
	}
	if (PatrolMode != EGridMonsterPatrolMode::PingPong)
	{
		return false;
	}

	InOutPingPongDirection = InOutPingPongDirection < 0 ? -1 : 1;
	if (ArrivedWaypointIndex <= 0)
	{
		InOutPingPongDirection = 1;
	}
	else if (ArrivedWaypointIndex >= WaypointCount - 1)
	{
		InOutPingPongDirection = -1;
	}
	OutNextWaypointIndex = ArrivedWaypointIndex + InOutPingPongDirection;
	return OutNextWaypointIndex >= 0 && OutNextWaypointIndex < WaypointCount;
}

void UGridMonsterPatrolSubsystem::Deinitialize()
{
	if (UGridTurnManagerComponent* TurnManager = BoundTurnManager.Get())
	{
		TurnManager->OnPhaseChanged.RemoveDynamic(this, &UGridMonsterPatrolSubsystem::HandleCombatPhaseChanged);
		TurnManager->OnCombatEnded.RemoveDynamic(this, &UGridMonsterPatrolSubsystem::HandleCombatEnded);
	}
	for (TPair<FGuid, FRuntimeEntry>& Pair : RuntimeEntries)
	{
		CancelScheduledStep(Pair.Value);
		CancelExplorationMotion(Pair.Value);
	}
	RuntimeEntries.Reset();
	BoundRuntimeActor.Reset();
	BoundTurnManager.Reset();
	Super::Deinitialize();
}

void UGridMonsterPatrolSubsystem::RegisterRuntime(AGridLevelRuntimeActor* RuntimeActor)
{
	if (!IsValid(RuntimeActor) || RuntimeActor->GetWorld() != GetWorld())
	{
		return;
	}
	UGridTurnManagerComponent* TurnManager = RuntimeActor->FindComponentByClass<UGridTurnManagerComponent>();
	if (!TurnManager)
	{
		return;
	}
	if (BoundTurnManager.Get() == TurnManager && BoundRuntimeActor.Get() == RuntimeActor)
	{
		return;
	}

	if (UGridTurnManagerComponent* Previous = BoundTurnManager.Get())
	{
		Previous->OnPhaseChanged.RemoveDynamic(this, &UGridMonsterPatrolSubsystem::HandleCombatPhaseChanged);
		Previous->OnCombatEnded.RemoveDynamic(this, &UGridMonsterPatrolSubsystem::HandleCombatEnded);
	}
	BoundRuntimeActor = RuntimeActor;
	BoundTurnManager = TurnManager;
	TurnManager->OnPhaseChanged.AddUniqueDynamic(this, &UGridMonsterPatrolSubsystem::HandleCombatPhaseChanged);
	TurnManager->OnCombatEnded.AddUniqueDynamic(this, &UGridMonsterPatrolSubsystem::HandleCombatEnded);
}

void UGridMonsterPatrolSubsystem::HandlePerceptionEvaluation(AGridLevelRuntimeActor* RuntimeActor, bool bCombatStarted, FName Reason)
{
	RegisterRuntime(RuntimeActor);
	if (!IsValid(RuntimeActor))
	{
		return;
	}
	if (bCombatStarted)
	{
		SuspendAllForCombat();
		return;
	}
	if (!IsRuntimeSafeForExploration(RuntimeActor) || !GetWorld())
	{
		return;
	}

	bHandlingCompletedAutomaticEvaluation = true;
	for (TActorIterator<AGridMonsterActor> It(GetWorld()); It; ++It)
	{
		AGridMonsterActor* Monster = *It;
		if (IsMonsterOwnedByRuntime(Monster, RuntimeActor))
		{
			ProcessMonsterInternal(Monster, true, Reason);
		}
	}
	bHandlingCompletedAutomaticEvaluation = false;
}

void UGridMonsterPatrolSubsystem::BootstrapRuntimeExploration(
	AGridLevelRuntimeActor* RuntimeActor, AGrimrockPartyPawn* ReadyParty, FName Reason)
{
	RegisterRuntime(RuntimeActor);
	UWorld* World = GetWorld();
	if (!IsValid(RuntimeActor) || !World || RuntimeActor->GetWorld() != World)
	{
		return;
	}

	if (!IsValid(ReadyParty) || ReadyParty->LevelRuntimeActor != RuntimeActor)
	{
		ReadyParty = nullptr;
		for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
		{
			AGrimrockPartyPawn* Candidate = *It;
			if (IsValid(Candidate) && Candidate->HasActorBegunPlay() && Candidate->LevelRuntimeActor == RuntimeActor)
			{
				ReadyParty = Candidate;
				break;
			}
		}
	}
	if (!ReadyParty)
	{
		UE_LOG(LogGridMonsterPatrol, Verbose, TEXT("[MON14.3.2] Patrol bootstrap deferred Runtime=%s Reason=%s Cause=PartyNotReady"),
			*GetNameSafe(RuntimeActor), *Reason.ToString());
		return;
	}

	if (RuntimeActor->bIsExecutingRelocation)
	{
		UE_LOG(LogGridMonsterPatrol, Log, TEXT("[MON14.3.2] Patrol bootstrap deferred Runtime=%s Reason=%s Cause=RelocationInProgress"),
			*GetNameSafe(RuntimeActor), *Reason.ToString());
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UGridMonsterPatrolSubsystem::BootstrapRuntimeExploration, RuntimeActor, ReadyParty, Reason));
		return;
	}

	if (!IsRuntimeSafeForExploration(RuntimeActor))
	{
		return;
	}

	int32 EligiblePatrolCount = 0;
	int32 StartedPatrolCount = 0;
	for (TActorIterator<AGridMonsterActor> It(World); It; ++It)
	{
		AGridMonsterActor* Monster = *It;
		if (!IsMonsterOwnedByRuntime(Monster, RuntimeActor))
		{
			continue;
		}

		const bool bAuthoredPatrol =
			Monster->MonsterState == EGridMonsterState::Idle && Monster->PatrolMode != EGridMonsterPatrolMode::None && Monster->PatrolWaypoints.Num() >= 2;
		EligiblePatrolCount += bAuthoredPatrol ? 1 : 0;
		if (ProcessMonsterInternal(Monster, true, Reason) && bAuthoredPatrol)
		{
			++StartedPatrolCount;
		}
	}

	UE_LOG(LogGridMonsterPatrol, Log, TEXT("[MON14.3.2] Patrol bootstrap Runtime=%s Reason=%s Eligible=%d Processed=%d"),
		*GetNameSafe(RuntimeActor), *Reason.ToString(), EligiblePatrolCount, StartedPatrolCount);
}

bool UGridMonsterPatrolSubsystem::ProcessMonsterNow(AGridMonsterActor* Monster, FName Reason)
{
	return ProcessMonsterInternal(Monster, true, Reason);
}

void UGridMonsterPatrolSubsystem::SuspendAllForCombat()
{
	for (TPair<FGuid, FRuntimeEntry>& Pair : RuntimeEntries)
	{
		CancelScheduledStep(Pair.Value);
		CancelExplorationMotion(Pair.Value);
		Pair.Value.Activity = EGridMonsterExplorationActivity::Suspended;
	}
}

EGridMonsterExplorationActivity UGridMonsterPatrolSubsystem::GetMonsterActivity(FGuid MonsterId) const
{
	const FRuntimeEntry* Entry = RuntimeEntries.Find(MonsterId);
	return Entry ? Entry->Activity : EGridMonsterExplorationActivity::Inactive;
}

int32 UGridMonsterPatrolSubsystem::GetMonsterTargetWaypointIndex(FGuid MonsterId) const
{
	const FRuntimeEntry* Entry = RuntimeEntries.Find(MonsterId);
	return Entry ? Entry->TargetWaypointIndex : INDEX_NONE;
}

void UGridMonsterPatrolSubsystem::HandleCombatPhaseChanged(EGridCombatPhase NewPhase)
{
	if (NewPhase != EGridCombatPhase::Exploration)
	{
		SuspendAllForCombat();
	}
}

void UGridMonsterPatrolSubsystem::HandleCombatEnded(EGridCombatPhase ResultPhase)
{
	if (ResultPhase != EGridCombatPhase::Victory)
	{
		return;
	}
	for (TPair<FGuid, FRuntimeEntry>& Pair : RuntimeEntries)
	{
		AGridMonsterActor* Monster = Pair.Value.Monster.Get();
		if (IsValid(Monster) && !Monster->IsDead() && Monster->bMonsterEnabled)
		{
			Pair.Value.Activity = EGridMonsterExplorationActivity::Inactive;
			ScheduleStep(Pair.Value, 0.05f);
		}
	}
}

void UGridMonsterPatrolSubsystem::HandleScheduledStep(FGuid MonsterId)
{
	FRuntimeEntry* Entry = RuntimeEntries.Find(MonsterId);
	if (!Entry)
	{
		return;
	}
	AGridMonsterActor* Monster = Entry->Monster.Get();
	AGridLevelRuntimeActor* RuntimeActor = Entry->RuntimeActor.Get();
	if (!IsValid(Monster) || !IsRuntimeSafeForExploration(RuntimeActor))
	{
		return;
	}

	UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>();
	if (Movement && Movement->IsBusy())
	{
		ScheduleStep(*Entry, 0.02f);
		return;
	}
	ProcessMonsterInternal(Monster, true, TEXT("ScheduledPatrolStep"));
}

bool UGridMonsterPatrolSubsystem::ProcessMonsterInternal(AGridMonsterActor* Monster, bool bRefreshPerception, FName Reason)
{
	if (!IsValid(Monster) || Monster->IsDead() || !Monster->bMonsterEnabled || !Monster->IsRuntimeLevelActive())
	{
		return false;
	}

	FRuntimeEntry* Entry = FindOrAddEntry(Monster);
	if (!Entry)
	{
		return false;
	}
	AGridLevelRuntimeActor* RuntimeActor = Entry->RuntimeActor.Get();
	if (!IsRuntimeSafeForExploration(RuntimeActor))
	{
		Entry->Activity = EGridMonsterExplorationActivity::Suspended;
		return false;
	}

	UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>();
	UGridMonsterBehaviorComponent* Behavior = Monster->FindComponentByClass<UGridMonsterBehaviorComponent>();
	if (!Movement || !Behavior)
	{
		Entry->Activity = EGridMonsterExplorationActivity::Inactive;
		return false;
	}
	if (!Movement->IsInitialized() && !Movement->InitializeMovement(RuntimeActor))
	{
		return false;
	}
	if (!Behavior->IsInitialized() && !Behavior->InitializeBehavior(RuntimeActor, BoundTurnManager.IsValid() ? BoundTurnManager->PartyPawn : nullptr))
	{
		return false;
	}

	if (bRefreshPerception)
	{
		Behavior->RefreshPerception();
	}

	if (Behavior->bCanSeeParty)
	{
		CancelScheduledStep(*Entry);
		if (Movement->IsBusy())
		{
			Movement->CancelCurrentAction();
		}
		Monster->SetMonsterState(EGridMonsterState::Alert);
		Entry->Activity = EGridMonsterExplorationActivity::Engaging;
		if (!bHandlingCompletedAutomaticEvaluation)
		{
			GridAutomaticPerceptionEngagement::Request(RuntimeActor, TEXT("PatrolVision"));
		}
		UE_LOG(LogGridMonsterPatrol, Verbose, TEXT("[MON14.3] Vision Monster=%s Reason=%s Request=%s"), *GetNameSafe(Monster), *Reason.ToString(),
			bHandlingCompletedAutomaticEvaluation ? TEXT("false") : TEXT("true"));
		return true;
	}

	if (Behavior->bCanHearParty)
	{
		CancelScheduledStep(*Entry);
		if (Movement->IsBusy())
		{
			Movement->CancelCurrentAction();
		}
		Monster->SetMonsterState(EGridMonsterState::Alert);
		Entry->Activity = EGridMonsterExplorationActivity::Investigating;
		Entry->SearchTurnsRemaining = 0;
		return ProcessInvestigation(*Entry, Monster);
	}

	if (Entry->Activity == EGridMonsterExplorationActivity::Searching)
	{
		return ProcessSearch(*Entry, Monster);
	}
	if ((Entry->Activity == EGridMonsterExplorationActivity::Investigating || Monster->MonsterState == EGridMonsterState::Alert ||
			Monster->MonsterState == EGridMonsterState::Pursuing) &&
		Behavior->bHasLastKnownPartyCell)
	{
		if (Movement->IsBusy())
		{
			return true;
		}
		Entry->Activity = EGridMonsterExplorationActivity::Investigating;
		return ProcessInvestigation(*Entry, Monster);
	}

	// A fresh world event may re-evaluate perception while a guard is honoring
	// a waypoint wait. Perception may interrupt the wait above, but an unrelated
	// event must never shorten the authored WaitSeconds contract.
	if (Entry->Activity == EGridMonsterExplorationActivity::Waiting && GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(Entry->TimerHandle))
	{
		return true;
	}

	if (Movement->IsBusy())
	{
		return true;
	}
	if (Monster->MonsterState == EGridMonsterState::Dormant)
	{
		CancelScheduledStep(*Entry);
		Entry->Activity = EGridMonsterExplorationActivity::Inactive;
		return true;
	}
	if (Monster->MonsterState != EGridMonsterState::Idle || Monster->PatrolMode == EGridMonsterPatrolMode::None || Monster->PatrolWaypoints.Num() < 2)
	{
		CancelScheduledStep(*Entry);
		Entry->Activity = EGridMonsterExplorationActivity::Inactive;
		return true;
	}
	return ProcessPatrol(*Entry, Monster);
}

bool UGridMonsterPatrolSubsystem::ProcessPatrol(FRuntimeEntry& Entry, AGridMonsterActor* Monster)
{
	UGridMonsterBehaviorComponent* Behavior = IsValid(Monster) ? Monster->FindComponentByClass<UGridMonsterBehaviorComponent>() : nullptr;
	UGridMonsterMovementComponent* Movement = IsValid(Monster) ? Monster->FindComponentByClass<UGridMonsterMovementComponent>() : nullptr;
	if (!Behavior || !Movement || Monster->PatrolWaypoints.Num() < 2)
	{
		return false;
	}

	const bool bNeedsCursorInitialization = !Monster->PatrolWaypoints.IsValidIndex(Entry.TargetWaypointIndex);
	if (bNeedsCursorInitialization &&
		!FGridMonsterPatrolCursorRules::Initialize(
			Monster->CurrentCell, Monster->PatrolMode, Monster->PatrolWaypoints, Entry.TargetWaypointIndex, Entry.PingPongDirection))
	{
		Entry.Activity = EGridMonsterExplorationActivity::Inactive;
		return false;
	}

	if (bNeedsCursorInitialization)
	{
		const FGridMonsterPatrolWaypoint& InitialTarget = Monster->PatrolWaypoints[Entry.TargetWaypointIndex];
		UE_LOG(LogGridMonsterPatrol, Log,
			TEXT("[MON14.3.2] Patrol initialized Monster=%s Mode=%s Waypoints=%d Cell=(%d,%d) TargetIndex=%d TargetCell=(%d,%d)"),
			*GetNameSafe(Monster), *UEnum::GetValueAsString(Monster->PatrolMode), Monster->PatrolWaypoints.Num(), Monster->CurrentCell.X,
			Monster->CurrentCell.Y, Entry.TargetWaypointIndex, InitialTarget.Cell.X, InitialTarget.Cell.Y);
	}

	const FGridMonsterPatrolWaypoint& Target = Monster->PatrolWaypoints[Entry.TargetWaypointIndex];
	if (Monster->CurrentCell == Target.Cell)
	{
		if (IsCardinalFacing(Target.Facing) && Monster->Facing != Target.Facing)
		{
			return StartTurnToward(Entry, Monster, Movement, Target.Facing, EGridMonsterExplorationActivity::Patrolling);
		}

		const int32 ArrivedIndex = Entry.TargetWaypointIndex;
		int32 NextIndex = INDEX_NONE;
		if (!FGridMonsterPatrolCursorRules::Advance(Monster->PatrolMode, Monster->PatrolWaypoints.Num(), ArrivedIndex, Entry.PingPongDirection, NextIndex))
		{
			Entry.Activity = EGridMonsterExplorationActivity::Inactive;
			return false;
		}
		Entry.TargetWaypointIndex = NextIndex;
		Entry.LastUnreachableWaypointIndex = INDEX_NONE;
		Entry.Activity = EGridMonsterExplorationActivity::Waiting;
		ScheduleStep(Entry, FMath::Max(0.01f, Target.WaitSeconds));
		return true;
	}

	if (!Behavior->FindPathToCell(Target.Cell, false))
	{
		if (Entry.LastUnreachableWaypointIndex != Entry.TargetWaypointIndex)
		{
			UE_LOG(LogGridMonsterPatrol, Log,
				TEXT("[MON14.3.2] Patrol path unavailable Monster=%s Cell=(%d,%d) TargetIndex=%d TargetCell=(%d,%d) Action=Retry"),
				*GetNameSafe(Monster), Monster->CurrentCell.X, Monster->CurrentCell.Y, Entry.TargetWaypointIndex, Target.Cell.X, Target.Cell.Y);
			Entry.LastUnreachableWaypointIndex = Entry.TargetWaypointIndex;
		}
		Entry.Activity = EGridMonsterExplorationActivity::Patrolling;
		ScheduleStep(Entry, 0.25f);
		return false;
	}
	Entry.LastUnreachableWaypointIndex = INDEX_NONE;
	EGridEdge Direction = EGridEdge::None;
	if (!Behavior->GetNextPathDirection(Direction))
	{
		Entry.Activity = EGridMonsterExplorationActivity::Patrolling;
		ScheduleStep(Entry, 0.10f);
		return false;
	}
	return StartMoveTowardDirection(Entry, Monster, Movement, Direction, EGridMonsterExplorationActivity::Patrolling);
}

bool UGridMonsterPatrolSubsystem::ProcessInvestigation(FRuntimeEntry& Entry, AGridMonsterActor* Monster)
{
	UGridMonsterBehaviorComponent* Behavior = IsValid(Monster) ? Monster->FindComponentByClass<UGridMonsterBehaviorComponent>() : nullptr;
	UGridMonsterMovementComponent* Movement = IsValid(Monster) ? Monster->FindComponentByClass<UGridMonsterMovementComponent>() : nullptr;
	if (!Behavior || !Movement)
	{
		return false;
	}
	if (!Behavior->bHasLastKnownPartyCell)
	{
		FinishInvestigationAndResumePatrol(Entry, Monster, Behavior);
		return true;
	}

	Monster->SetMonsterState(Behavior->bCanHearParty ? EGridMonsterState::Alert : EGridMonsterState::Pursuing);
	Entry.Activity = EGridMonsterExplorationActivity::Investigating;
	if (!Behavior->FindPathToLastKnownPartyCell())
	{
		// A continuously audible target behind an impassable door/wall is not "lost".
		// Stay stably alert and retry later instead of entering the turn-in-place search
		// loop, which can visually look like the monster is trembling behind the obstacle.
		if (Behavior->bCanHearParty)
		{
			Entry.Activity = EGridMonsterExplorationActivity::Investigating;
			Entry.SearchTurnsRemaining = 0;
			ScheduleStep(Entry, 0.25f);
			UE_LOG(LogGridMonsterPatrol, Verbose,
				TEXT("[MON-AI-BLOCKED-INVESTIGATION01] Audible target unreachable Monster=%s MonsterCell=(%d,%d) PartyCell=(%d,%d) Action=WaitAndRetry"),
				*GetNameSafe(Monster), Monster->CurrentCell.X, Monster->CurrentCell.Y, Behavior->LastKnownPartyCell.X, Behavior->LastKnownPartyCell.Y);
			return true;
		}

		BeginSearch(Entry, Monster);
		return true;
	}
	EGridEdge Direction = EGridEdge::None;
	if (!Behavior->GetNextPathDirection(Direction))
	{
		BeginSearch(Entry, Monster);
		return true;
	}
	return StartMoveTowardDirection(Entry, Monster, Movement, Direction, EGridMonsterExplorationActivity::Investigating);
}

bool UGridMonsterPatrolSubsystem::ProcessSearch(FRuntimeEntry& Entry, AGridMonsterActor* Monster)
{
	UGridMonsterBehaviorComponent* Behavior = IsValid(Monster) ? Monster->FindComponentByClass<UGridMonsterBehaviorComponent>() : nullptr;
	UGridMonsterMovementComponent* Movement = IsValid(Monster) ? Monster->FindComponentByClass<UGridMonsterMovementComponent>() : nullptr;
	if (!Behavior || !Movement)
	{
		return false;
	}
	if (Entry.SearchTurnsRemaining <= 0)
	{
		FinishInvestigationAndResumePatrol(Entry, Monster, Behavior);
		return true;
	}
	if (!Movement->TryTurnRight())
	{
		FinishInvestigationAndResumePatrol(Entry, Monster, Behavior);
		return false;
	}
	--Entry.SearchTurnsRemaining;
	Entry.Activity = EGridMonsterExplorationActivity::Searching;
	ScheduleStep(Entry, GetTurnContinuationDelay(Monster));
	return true;
}

bool UGridMonsterPatrolSubsystem::StartMoveTowardDirection(
	FRuntimeEntry& Entry, AGridMonsterActor* Monster, UGridMonsterMovementComponent* Movement, EGridEdge Direction, EGridMonsterExplorationActivity Activity)
{
	if (!IsValid(Monster) || !Movement || !IsCardinalFacing(Direction))
	{
		return false;
	}
	if (Monster->Facing != Direction)
	{
		return StartTurnToward(Entry, Monster, Movement, Direction, Activity);
	}
	if (!Movement->TryMove(Direction))
	{
		Entry.Activity = Activity;
		ScheduleStep(Entry, 0.25f);
		return false;
	}
	Entry.Activity = Activity;
	ScheduleStep(Entry, GetMoveContinuationDelay(Monster));
	return true;
}

bool UGridMonsterPatrolSubsystem::StartTurnToward(
	FRuntimeEntry& Entry, AGridMonsterActor* Monster, UGridMonsterMovementComponent* Movement, EGridEdge TargetFacing, EGridMonsterExplorationActivity Activity)
{
	if (!IsValid(Monster) || !Movement || !IsCardinalFacing(TargetFacing) || Monster->Facing == TargetFacing)
	{
		return false;
	}
	const bool bStarted = GridDirectionUtils::RotateLeft(Monster->Facing) == TargetFacing ? Movement->TryTurnLeft() : Movement->TryTurnRight();
	if (!bStarted)
	{
		Entry.Activity = Activity;
		ScheduleStep(Entry, 0.25f);
		return false;
	}
	Entry.Activity = Activity;
	ScheduleStep(Entry, GetTurnContinuationDelay(Monster));
	return true;
}

void UGridMonsterPatrolSubsystem::BeginSearch(FRuntimeEntry& Entry, AGridMonsterActor* Monster)
{
	if (!IsValid(Monster))
	{
		return;
	}
	Entry.Activity = EGridMonsterExplorationActivity::Searching;
	Entry.SearchTurnsRemaining = 3;
	ProcessSearch(Entry, Monster);
}

void UGridMonsterPatrolSubsystem::FinishInvestigationAndResumePatrol(FRuntimeEntry& Entry, AGridMonsterActor* Monster, UGridMonsterBehaviorComponent* Behavior)
{
	if (!IsValid(Monster) || !Behavior)
	{
		return;
	}
	Behavior->ClearLastKnownPartyCell();
	Entry.SearchTurnsRemaining = 0;
	Monster->SetMonsterState(EGridMonsterState::Idle);
	if (Monster->PatrolMode != EGridMonsterPatrolMode::None && Monster->PatrolWaypoints.Num() >= 2)
	{
		Entry.Activity = EGridMonsterExplorationActivity::Patrolling;
		ProcessPatrol(Entry, Monster);
	}
	else
	{
		Entry.Activity = EGridMonsterExplorationActivity::Inactive;
	}
}

UGridMonsterPatrolSubsystem::FRuntimeEntry* UGridMonsterPatrolSubsystem::FindOrAddEntry(AGridMonsterActor* Monster)
{
	if (!IsValid(Monster))
	{
		return nullptr;
	}
	const FGuid MonsterId = Monster->ResolvePersistenceId();
	if (!MonsterId.IsValid())
	{
		UE_LOG(LogGridMonsterPatrol, Warning, TEXT("[MON14.3] Monster=%s has no stable persistence id."), *GetNameSafe(Monster));
		return nullptr;
	}

	AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor>(Monster->GetOwner());
	if (!IsValid(RuntimeActor))
	{
		RuntimeActor = BoundRuntimeActor.Get();
	}
	if (!IsValid(RuntimeActor))
	{
		return nullptr;
	}
	RegisterRuntime(RuntimeActor);
	FRuntimeEntry& Entry = RuntimeEntries.FindOrAdd(MonsterId);
	Entry.Monster = Monster;
	Entry.RuntimeActor = RuntimeActor;
	return &Entry;
}

void UGridMonsterPatrolSubsystem::ScheduleStep(FRuntimeEntry& Entry, float DelaySeconds)
{
	UWorld* World = GetWorld();
	AGridMonsterActor* Monster = Entry.Monster.Get();
	if (!World || !IsValid(Monster))
	{
		return;
	}
	const FGuid MonsterId = Monster->ResolvePersistenceId();
	if (!MonsterId.IsValid())
	{
		return;
	}
	World->GetTimerManager().ClearTimer(Entry.TimerHandle);
	World->GetTimerManager().SetTimer(Entry.TimerHandle, FTimerDelegate::CreateUObject(this, &UGridMonsterPatrolSubsystem::HandleScheduledStep, MonsterId),
		FMath::Max(0.01f, DelaySeconds), false);
}

void UGridMonsterPatrolSubsystem::CancelScheduledStep(FRuntimeEntry& Entry)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Entry.TimerHandle);
	}
}

void UGridMonsterPatrolSubsystem::CancelExplorationMotion(FRuntimeEntry& Entry)
{
	AGridMonsterActor* Monster = Entry.Monster.Get();
	if (!IsValid(Monster))
	{
		return;
	}
	if (UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>())
	{
		if (Movement->IsBusy())
		{
			Movement->CancelCurrentAction();
		}
	}
}

bool UGridMonsterPatrolSubsystem::IsRuntimeSafeForExploration(AGridLevelRuntimeActor* RuntimeActor) const
{
	if (!IsValid(RuntimeActor) || RuntimeActor->bIsExecutingRelocation)
	{
		return false;
	}
	const UGridTurnManagerComponent* TurnManager = RuntimeActor->FindComponentByClass<UGridTurnManagerComponent>();
	return !TurnManager || !TurnManager->bCombatActive;
}

bool UGridMonsterPatrolSubsystem::IsMonsterOwnedByRuntime(const AGridMonsterActor* Monster, const AGridLevelRuntimeActor* RuntimeActor) const
{
	if (!IsValid(Monster) || !IsValid(RuntimeActor) || Monster->IsDead() || !Monster->bMonsterEnabled || !Monster->IsRuntimeLevelActive())
	{
		return false;
	}
	if (Monster->GetOwner() == RuntimeActor)
	{
		return true;
	}
	return Monster->ResolveRuntimeDungeonLevelId(RuntimeActor->CurrentDungeonLevelId) == RuntimeActor->CurrentDungeonLevelId;
}

bool UGridMonsterPatrolSubsystem::IsCardinalFacing(EGridEdge Facing)
{
	return Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
}

float UGridMonsterPatrolSubsystem::GetMoveContinuationDelay(const AGridMonsterActor* Monster)
{
	return FMath::Max(0.02f, IsValid(Monster) && IsValid(Monster->MonsterDefinition) ? Monster->MonsterDefinition->MoveDuration + 0.02f : 0.12f);
}

float UGridMonsterPatrolSubsystem::GetTurnContinuationDelay(const AGridMonsterActor* Monster)
{
	return FMath::Max(0.02f, IsValid(Monster) && IsValid(Monster->MonsterDefinition) ? Monster->MonsterDefinition->TurnDuration + 0.02f : 0.12f);
}
