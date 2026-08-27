#include "Magic/GridSpellHotbarExecution.h"

bool FGridSpellHotbarExecutionService::TryExecute(const FGridSpellDefinition& Definition, const FGridSpellCastRequest& Request,
	const FGridSpellTargetingContext& TargetingContext, const FGridCharacterSpellbookState& Spellbook, const FRPGCharacterResources& CasterResources,
	const FGridPlayerCharacterTurnState& CasterTurnState, int32 TargetMaxHealth, int32 TargetCurrentHealth,
	const FGridStatusEffectCollection& TargetStatusEffects, TFunctionRef<const UGridStatusEffectDefinitionAsset*(FName)> StatusDefinitionResolver,
	FGridSpellHotbarExecutionResult& OutResult)
{
	OutResult = FGridSpellHotbarExecutionResult();

	FRPGCharacterResources WorkingCasterResources = CasterResources;
	FGridPlayerCharacterTurnState WorkingTurnState = CasterTurnState;
	FGridSpellResolvedTarget ResolvedTarget;
	FGridSpellCastCostReceipt CostReceipt;
	if (!FGridSpellCastPipelineService::TryValidateTargetAndCommitCosts(Definition, Request, TargetingContext, Spellbook, WorkingCasterResources, WorkingTurnState,
			ResolvedTarget, CostReceipt, OutResult.PipelineRejectStage, OutResult.TargetingRejectReason, OutResult.TransactionRejectReason))
	{
		return false;
	}

	int32 WorkingTargetHealth = TargetCurrentHealth;
	FGridStatusEffectCollection WorkingTargetStatuses = TargetStatusEffects;
	FGridSpellEffectResolutionResult EffectResult;
	if (!FGridSpellEffectResolver::ResolveEffects(Definition, Request.CasterCharacterId, TargetMaxHealth, WorkingTargetHealth, WorkingTargetStatuses,
			StatusDefinitionResolver, EffectResult, OutResult.EffectRejectReason, OutResult.Error))
	{
		// Costs were only committed to local copies. The caller's authoritative
		// caster state therefore remains untouched on any effect rejection.
		return false;
	}

	// MON18.9.2: a spell that cannot change its target must not consume AP or
	// mana. Costs still live only on the working copies at this point, so this
	// rejection remains fully atomic for the authoritative runtime state.
	if (!EffectResult.DidMutateTarget())
	{
		OutResult.EffectRejectReason = EGridSpellEffectResolutionRejectReason::NoEffectWouldApply;
		OutResult.Error = TEXT("Spell effects would not change the target.");
		return false;
	}

	OutResult.CasterResources = MoveTemp(WorkingCasterResources);
	OutResult.CasterTurnState = MoveTemp(WorkingTurnState);
	OutResult.TargetCurrentHealth = WorkingTargetHealth;
	OutResult.TargetStatusEffects = MoveTemp(WorkingTargetStatuses);
	OutResult.ResolvedTarget = ResolvedTarget;
	OutResult.CostReceipt = CostReceipt;
	OutResult.EffectResult = MoveTemp(EffectResult);
	return true;
}
