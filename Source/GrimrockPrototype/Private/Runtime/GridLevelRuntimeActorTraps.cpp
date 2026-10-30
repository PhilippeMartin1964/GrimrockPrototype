#include "Runtime/GridLevelRuntimeActor.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

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


// ARMEMENT01.16.2A — Persistent authored dungeon traps are Trigger objects,
// not FGridCombatTrapState (which is temporary and keyed by cell).
bool AGridLevelRuntimeActor::IsTriggerTrapDisarmed(FGuid TrapObjectId) const
{
    const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel();
    return TrapObjectId.IsValid() && State &&
        State->DisarmedTriggerTrapIds.Contains(TrapObjectId);
}

bool AGridLevelRuntimeActor::FindDisarmableTriggerTrapForParty(
    const AGrimrockPartyPawn* PartyPawn, FGuid& OutTrapObjectId) const
{
    OutTrapObjectId.Invalidate();
    if (!IsValid(PartyPawn) || PartyPawn->LevelRuntimeActor != this || !IsValid(LevelAsset))
    {
        return false;
    }

    const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
    FIntPoint FrontCell(INDEX_NONE, INDEX_NONE);
    int32 FrontX = INDEX_NONE;
    int32 FrontY = INDEX_NONE;
    if (TryGetNeighborCell(PartyCell.X, PartyCell.Y, PartyPawn->Facing, FrontX, FrontY) &&
        CanMove(PartyCell.X, PartyCell.Y, PartyPawn->Facing))
    {
        FrontCell = FIntPoint(FrontX, FrontY);
    }

    // Prefer the trigger in the party cell, then the next traversable cell.
    // Two targets in the same cell are ambiguous: never choose randomly.
    for (const FIntPoint CandidateCell : { PartyCell, FrontCell })
    {
        if (CandidateCell.X == INDEX_NONE)
        {
            continue;
        }
        FGuid Candidate;
        for (const FGridWorldObjectInstance& Object : LevelAsset->WorldObjectInstances)
        {
            if (Object.Type != EGridLevelObjectType::Trigger ||
                Object.CellX != CandidateCell.X || Object.CellY != CandidateCell.Y)
            {
                continue;
            }
            int32 Difficulty = 0;
            if (!GetTriggerTrapDisarmDifficulty(Object.InstanceId, Difficulty))
            {
                continue;
            }
            if (Candidate.IsValid())
            {
                OutTrapObjectId.Invalidate();
                return false;
            }
            Candidate = Object.InstanceId;
        }
        if (Candidate.IsValid())
        {
            OutTrapObjectId = Candidate;
            return true;
        }
    }
    return false;
}

bool AGridLevelRuntimeActor::GetTriggerTrapDisarmDifficulty(
    FGuid TrapObjectId, int32& OutDifficulty) const
{
    OutDifficulty = 0;
    if (!TrapObjectId.IsValid() || !IsValid(LevelAsset) ||
        IsTriggerTrapDisarmed(TrapObjectId))
    {
        return false;
    }

    const FGridWorldObjectInstance* Trap =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const UGridWorldObjectDefinitionAsset* Definition = Trap
        ? FindWorldObjectDefinition(Trap->WorldObjectDefinitionId) : nullptr;

    if (!Trap || Trap->Type != EGridLevelObjectType::Trigger ||
        !IsValid(Definition) || !Definition->bCanBeDisarmed ||
        Definition->SupportedType != EGridLevelObjectType::Trigger ||
        Definition->DisarmDifficulty < 1 || Definition->DisarmDifficulty > 40)
    {
        return false;
    }
    OutDifficulty = Definition->DisarmDifficulty;
    return true;
}

bool AGridLevelRuntimeActor::TryDisarmTriggerTrap(
    AGrimrockPartyPawn* PartyPawn, FGuid TrapObjectId,
    int32 CharacterIndex, const URPGSkillAsset* SkillDefinition,
    FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult)
{
    OutResult = FRPGSkillCheckResult();

    int32 Difficulty = 0;
    if (!GetTriggerTrapDisarmDifficulty(TrapObjectId, Difficulty) ||
        !IsValid(PartyPawn) || PartyPawn->LevelRuntimeActor != this ||
        !IsValid(PartyPawn->PartyInventoryComponent) ||
        !IsValid(SkillDefinition) ||
        SkillDefinition->SkillId != FName(TEXT("Skill_Traps")))
    {
        return false;
    }

    const FGridWorldObjectInstance* Target =
        LevelAsset->FindWorldObjectInstanceById(TrapObjectId);
    const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
    const FIntPoint TargetCell(Target->CellX, Target->CellY);
    // The player may disarm a trap directly underfoot or immediately ahead.
    // A wall, closed door or non-walkable destination prevents reaching
    // a trap in the adjacent cell (same grid collision authority as movement).
    if (PartyCell != TargetCell)
    {
        int32 FrontX = INDEX_NONE;
        int32 FrontY = INDEX_NONE;
        if (!TryGetNeighborCell(PartyCell.X, PartyCell.Y, PartyPawn->Facing, FrontX, FrontY) ||
            TargetCell != FIntPoint(FrontX, FrontY) ||
            !CanMove(PartyCell.X, PartyCell.Y, PartyPawn->Facing))
        {
            return false;
        }
    }

    UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
    if (!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
    {
        return false;
    }
    const FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (Character.Resources.CurrentHealth <= 0)
    {
        return false;
    }

    const FName ToolId(TEXT("Item_DisarmTools_01"));
    UGridItemDefinitionAsset* Tool = Inventory->FindItemDefinition(ToolId);
    if (!IsValid(Tool) || Tool->ItemDefinitionId != ToolId ||
        !Tool->ItemTags.Contains(TEXT("Tool.Disarm")) ||
        Inventory->CountItemDefinitionInCharacterInventory(CharacterIndex, ToolId) < 1)
    {
        return false;
    }

    // The resolver is the single authority for N1 ranks, d20, attributes,
    // and eventual level-2 Expert Disarm progression modifiers.
    if (!FRPGSkillCheckService::TryResolveSkillCheck(
        Character, SkillDefinition, Difficulty, RandomStream, OutResult))
    {
        return false;
    }
    if (!OutResult.bSuccess)
    {
        UE_LOG(LogGridCombatTrapRuntime, Log,
            TEXT("[ARMEMENT01.16.2A] TrapDisarm FAILED Target=%s SkillTotal=%d DC=%d"),
            *TrapObjectId.ToString(), OutResult.Total, Difficulty);
        return true; // Resolved check, not a successful disarm.
    }

    FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
    if (!State || State->DisarmedTriggerTrapIds.Contains(TrapObjectId))
    {
        OutResult = FRPGSkillCheckResult(); // No confirmed commit.
        return false;
    }
    State->DisarmedTriggerTrapIds.Add(TrapObjectId);

    // Trigger's normal Activated/Deactivated links are suppressed by the
    // activation component; Disarmed is emitted once for puzzle links.
    ExecuteLinksFromRuntimeObject(TrapObjectId, EGridObjectEvent::Disarmed);
    UE_LOG(LogGridCombatTrapRuntime, Log,
        TEXT("[ARMEMENT01.16.2A] TrapDisarm SUCCEEDED Target=%s SkillTotal=%d DC=%d"),
        *TrapObjectId.ToString(), OutResult.Total, Difficulty);
    return true;
}
