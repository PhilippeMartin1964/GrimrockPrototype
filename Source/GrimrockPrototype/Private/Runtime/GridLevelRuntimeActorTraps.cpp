#include "Runtime/GridLevelRuntimeActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridCombatTrapRuntime, Log, All);

bool AGridLevelRuntimeActor::ApplyCombatTrapAtCell(int32 CellX, int32 CellY, const FGridCombatTrapEffectProfile& Profile,
	const FGuid& SourceCombatantId, FName SourceActionId, int32 ResolvedRawDamage)
{
	if (!Profile.bPlaceTrap || !Profile.IsValid() || !IsValidCell(CellX, CellY) || !IsWalkableCell(CellX, CellY) ||
		!SourceCombatantId.IsValid() || SourceActionId.IsNone() || ResolvedRawDamage < 0)
	{
		return false;
	}
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return false;
	}

	FGridCombatTrapState State;
	State.TrapId = Profile.TrapId;
	State.RemainingRounds = Profile.DurationRounds;
	State.SourceCombatantId = SourceCombatantId;
	State.SourceActionId = SourceActionId;
	State.RawDamage = ResolvedRawDamage;
	State.DamageType = Profile.DamageType;
	State.PhysicalSubtype = Profile.PhysicalSubtype;
	State.StatusApplications = Profile.StatusApplications;
	State.bConsumeOnTrigger = Profile.bConsumeOnTrigger;
	if (!State.IsValid())
	{
		return false;
	}

	RuntimeState->CombatTraps.Add(FIntPoint(CellX, CellY), State);
	UE_LOG(LogGridCombatTrapRuntime, Log, TEXT("[RPG03.9.2] TrapApplied Cell=(%d,%d) Trap=%s Rounds=%d Damage=%d"),
		CellX, CellY, *State.TrapId.ToString(), State.RemainingRounds, State.RawDamage);
	return true;
}

const FGridCombatTrapState* AGridLevelRuntimeActor::FindCombatTrapAtCell(int32 CellX, int32 CellY) const
{
	const FGridLevelRuntimeState* RuntimeState = FindRuntimeStateForCurrentLevel();
	return RuntimeState ? RuntimeState->CombatTraps.Find(FIntPoint(CellX, CellY)) : nullptr;
}

bool AGridLevelRuntimeActor::ConsumeCombatTrapAtCell(int32 CellX, int32 CellY, FGridCombatTrapState& OutState)
{
	OutState = FGridCombatTrapState();
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return false;
	}
	const FIntPoint Cell(CellX, CellY);
	FGridCombatTrapState* State = RuntimeState->CombatTraps.Find(Cell);
	if (!State || !State->IsValid())
	{
		return false;
	}
	OutState = *State;
	RuntimeState->CombatTraps.Remove(Cell);
	return true;
}

int32 AGridLevelRuntimeActor::AdvanceCombatTrapRound(TArray<FIntPoint>* OutExpiredCells)
{
	if (OutExpiredCells) OutExpiredCells->Reset();
	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return 0;
	}

	TArray<FIntPoint> Cells;
	RuntimeState->CombatTraps.GetKeys(Cells);
	Cells.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.Y == B.Y ? A.X < B.X : A.Y < B.Y; });

	int32 Expired = 0;
	for (const FIntPoint& Cell : Cells)
	{
		FGridCombatTrapState* State = RuntimeState->CombatTraps.Find(Cell);
		if (!State) continue;
		State->RemainingRounds = FMath::Max(0, State->RemainingRounds - 1);
		if (State->RemainingRounds > 0) continue;
		if (OutExpiredCells) OutExpiredCells->Add(Cell);
		RuntimeState->CombatTraps.Remove(Cell);
		++Expired;
	}
	return Expired;
}
