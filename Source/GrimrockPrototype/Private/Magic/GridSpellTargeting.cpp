#include "Magic/GridSpellTargeting.h"

namespace
{
	int32 GridSpellTargetingDistance(const FIntPoint& From, const FIntPoint& To)
	{
		return FMath::Abs(To.X - From.X) + FMath::Abs(To.Y - From.Y);
	}

	bool GridSpellTargetingIsAxial(const FIntPoint& From, const FIntPoint& To)
	{
		return From.X == To.X || From.Y == To.Y;
	}

	bool GridSpellTargetingIsInRange(const FGridSpellDefinition& Definition, int32 Distance)
	{
		return Distance >= Definition.MinRangeCells && Distance <= Definition.MaxRangeCells;
	}
}

EGridSpellTargetingRejectReason FGridSpellTargetingService::ValidateAndResolveTarget(const FGridSpellDefinition& Definition,
	const FGridSpellCastRequest& Request, const FGridSpellTargetingContext& Context, FGridSpellResolvedTarget& OutResolvedTarget)
{
	OutResolvedTarget = FGridSpellResolvedTarget();

	if (FGridSpellContract::ValidateDefinition(Definition) != EGridSpellValidationError::None)
	{
		return EGridSpellTargetingRejectReason::InvalidSpellDefinition;
	}

	if (!Request.CasterCharacterId.IsValid() || Request.SpellId != Definition.SpellId)
	{
		return EGridSpellTargetingRejectReason::InvalidRequest;
	}

	FIntPoint TargetCell = Context.CasterCell;
	FGuid TargetId;
	bool bHasGridCell = true;

	switch (Definition.TargetingPolicy)
	{
		case EGridCombatTargetingPolicy::Self:
			TargetId = Request.CasterCharacterId;
			break;

		case EGridCombatTargetingPolicy::Ally:
			if (!Request.Target.TargetId.IsValid() || !Context.ResolvedTargetId.IsValid() || !Context.bHasResolvedTargetCell)
			{
				return EGridSpellTargetingRejectReason::MissingTarget;
			}
			if (Request.Target.TargetId != Context.ResolvedTargetId)
			{
				return EGridSpellTargetingRejectReason::TargetIdentityMismatch;
			}
			if (!Context.bResolvedTargetIsAlly)
			{
				return EGridSpellTargetingRejectReason::InvalidTargetRelation;
			}
			TargetId = Context.ResolvedTargetId;
			TargetCell = Context.ResolvedTargetCell;
			break;

		case EGridCombatTargetingPolicy::FirstAxialTarget:
			if (!Request.Target.TargetId.IsValid() || !Context.ResolvedTargetId.IsValid() || !Context.bHasResolvedTargetCell)
			{
				return EGridSpellTargetingRejectReason::MissingTarget;
			}
			if (Request.Target.TargetId != Context.ResolvedTargetId)
			{
				return EGridSpellTargetingRejectReason::TargetIdentityMismatch;
			}
			if (!Context.bResolvedTargetIsHostile)
			{
				return EGridSpellTargetingRejectReason::InvalidTargetRelation;
			}
			TargetId = Context.ResolvedTargetId;
			TargetCell = Context.ResolvedTargetCell;
			if (!GridSpellTargetingIsAxial(Context.CasterCell, TargetCell))
			{
				return EGridSpellTargetingRejectReason::TargetNotAxial;
			}
			break;

		case EGridCombatTargetingPolicy::Cell:
		case EGridCombatTargetingPolicy::Area:
			if (!Request.Target.bHasGridCell)
			{
				return EGridSpellTargetingRejectReason::MissingTarget;
			}
			TargetCell = Request.Target.GridCell;
			TargetId = Request.Target.TargetId;
			break;

		case EGridCombatTargetingPolicy::None:
		default:
			return EGridSpellTargetingRejectReason::InvalidSpellDefinition;
	}

	const int32 Distance = GridSpellTargetingDistance(Context.CasterCell, TargetCell);
	if (!GridSpellTargetingIsInRange(Definition, Distance))
	{
		return EGridSpellTargetingRejectReason::TargetOutOfRange;
	}

	if (Definition.bRequiresLineOfSight && !Context.bLineOfSightClear)
	{
		return EGridSpellTargetingRejectReason::LineOfSightBlocked;
	}

	OutResolvedTarget.TargetId = TargetId;
	OutResolvedTarget.GridCell = TargetCell;
	OutResolvedTarget.bHasGridCell = bHasGridCell;
	return EGridSpellTargetingRejectReason::None;
}

bool FGridSpellCastPipelineService::TryValidateTargetAndCommitCosts(const FGridSpellDefinition& Definition, const FGridSpellCastRequest& Request,
	const FGridSpellTargetingContext& TargetingContext, const FGridCharacterSpellbookState& Spellbook, FRPGCharacterResources& InOutCharacterResources,
	FGridPlayerCharacterTurnState& InOutTurnState, FGridSpellResolvedTarget& OutResolvedTarget, FGridSpellCastCostReceipt& OutReceipt,
	EGridSpellCastPipelineRejectStage& OutRejectStage, EGridSpellTargetingRejectReason& OutTargetingRejectReason,
	EGridSpellCastTransactionRejectReason& OutTransactionRejectReason)
{
	OutResolvedTarget = FGridSpellResolvedTarget();
	OutReceipt = FGridSpellCastCostReceipt();
	OutRejectStage = EGridSpellCastPipelineRejectStage::None;
	OutTargetingRejectReason = EGridSpellTargetingRejectReason::None;
	OutTransactionRejectReason = EGridSpellCastTransactionRejectReason::None;

	OutTargetingRejectReason = FGridSpellTargetingService::ValidateAndResolveTarget(Definition, Request, TargetingContext, OutResolvedTarget);
	if (OutTargetingRejectReason != EGridSpellTargetingRejectReason::None)
	{
		OutRejectStage = EGridSpellCastPipelineRejectStage::Targeting;
		OutResolvedTarget = FGridSpellResolvedTarget();
		return false;
	}

	if (!FGridSpellCastTransactionService::TryCommitCosts(
			Definition, Request, Spellbook, InOutCharacterResources, InOutTurnState, OutReceipt, OutTransactionRejectReason))
	{
		OutRejectStage = EGridSpellCastPipelineRejectStage::Transaction;
		OutResolvedTarget = FGridSpellResolvedTarget();
		return false;
	}

	return true;
}
