#pragma once

#include "CoreMinimal.h"
#include "Magic/GridSpellEffectResolver.h"
#include "Magic/GridSpellTargeting.h"

class UGridStatusEffectDefinitionAsset;

/**
 * Atomic runtime result for one Spellbook-backed hotbar cast.
 *
 * The service works exclusively on copies supplied by the caller. Runtime
 * authorities (TurnManager, party inventory, monsters) are only mutated after
 * this result has been accepted, so a targeting/effect failure never consumes
 * PA or mana.
 */
struct GRIMROCKPROTOTYPE_API FGridSpellHotbarExecutionResult
{
	FRPGDerivedStats CasterStats;
	FGridPlayerCharacterTurnState CasterTurnState;
	int32 TargetCurrentHealth = 0;
	FGridStatusEffectCollection TargetStatusEffects;
	FGridSpellResolvedTarget ResolvedTarget;
	FGridSpellCastCostReceipt CostReceipt;
	FGridSpellEffectResolutionResult EffectResult;
	EGridSpellCastPipelineRejectStage PipelineRejectStage = EGridSpellCastPipelineRejectStage::None;
	EGridSpellTargetingRejectReason TargetingRejectReason = EGridSpellTargetingRejectReason::None;
	EGridSpellCastTransactionRejectReason TransactionRejectReason = EGridSpellCastTransactionRejectReason::None;
	EGridSpellEffectResolutionRejectReason EffectRejectReason = EGridSpellEffectResolutionRejectReason::None;
	FString Error;
};

/**
 * UI01.4.3e.2 bridge between the resolved hotbar action and the validated
 * MON18.3-MON18.5 spell services. This class owns no persistent state.
 */
struct GRIMROCKPROTOTYPE_API FGridSpellHotbarExecutionService
{
	static bool TryExecute(const FGridSpellDefinition& Definition, const FGridSpellCastRequest& Request, const FGridSpellTargetingContext& TargetingContext,
		const FGridCharacterSpellbookState& Spellbook, const FRPGDerivedStats& CasterStats, const FGridPlayerCharacterTurnState& CasterTurnState,
		int32 TargetMaxHealth, int32 TargetCurrentHealth, const FGridStatusEffectCollection& TargetStatusEffects,
		TFunctionRef<const UGridStatusEffectDefinitionAsset*(FName)> StatusDefinitionResolver, FGridSpellHotbarExecutionResult& OutResult);
};
