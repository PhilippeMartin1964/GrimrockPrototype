#include "Magic/GridSpellCastTransaction.h"

EGridSpellCastTransactionRejectReason FGridSpellCastTransactionService::ValidateCostCommit(const FGridSpellDefinition& Definition,
	const FGridSpellCastRequest& Request, const FGridCharacterSpellbookState& Spellbook, const FRPGCharacterResources& CharacterResources,
	const FGridPlayerCharacterTurnState& TurnState)
{
	if (FGridSpellContract::ValidateDefinition(Definition) != EGridSpellValidationError::None)
	{
		return EGridSpellCastTransactionRejectReason::InvalidSpellDefinition;
	}

	if (!Request.CasterCharacterId.IsValid() || Request.SpellId.IsNone() || Request.SpellId != Definition.SpellId)
	{
		return EGridSpellCastTransactionRejectReason::InvalidRequest;
	}

	if (!Spellbook.IsValid())
	{
		return EGridSpellCastTransactionRejectReason::InvalidSpellbook;
	}

	if (Spellbook.CharacterId != Request.CasterCharacterId || TurnState.CharacterId != Request.CasterCharacterId)
	{
		return EGridSpellCastTransactionRejectReason::CharacterMismatch;
	}

	if (!Spellbook.KnowsSpell(Definition.SpellId))
	{
		return EGridSpellCastTransactionRejectReason::SpellNotKnown;
	}

	if (TurnState.State != EGridCombatantTurnState::Active)
	{
		return EGridSpellCastTransactionRejectReason::TurnInactive;
	}

	if (Definition.ActionPointCost > TurnState.RemainingActionPoints)
	{
		return EGridSpellCastTransactionRejectReason::InsufficientActionPoints;
	}

	if (CharacterResources.CurrentMana < 0 || Definition.ManaCost > CharacterResources.CurrentMana)
	{
		return EGridSpellCastTransactionRejectReason::InsufficientMana;
	}

	return EGridSpellCastTransactionRejectReason::None;
}

bool FGridSpellCastTransactionService::TryCommitCosts(const FGridSpellDefinition& Definition, const FGridSpellCastRequest& Request,
	const FGridCharacterSpellbookState& Spellbook, FRPGCharacterResources& InOutCharacterResources, FGridPlayerCharacterTurnState& InOutTurnState,
	FGridSpellCastCostReceipt& OutReceipt, EGridSpellCastTransactionRejectReason& OutRejectReason)
{
	OutReceipt = FGridSpellCastCostReceipt();
	OutRejectReason = ValidateCostCommit(Definition, Request, Spellbook, InOutCharacterResources, InOutTurnState);

	if (OutRejectReason != EGridSpellCastTransactionRejectReason::None)
	{
		return false;
	}

	const int32 PreviousActionPoints = InOutTurnState.RemainingActionPoints;
	const int32 PreviousMana = InOutCharacterResources.CurrentMana;

	InOutTurnState.RemainingActionPoints = PreviousActionPoints - Definition.ActionPointCost;
	InOutCharacterResources.CurrentMana = PreviousMana - Definition.ManaCost;

	OutReceipt.CharacterId = Request.CasterCharacterId;
	OutReceipt.SpellId = Definition.SpellId;
	OutReceipt.ActionPointsSpent = Definition.ActionPointCost;
	OutReceipt.ManaSpent = Definition.ManaCost;
	return true;
}
