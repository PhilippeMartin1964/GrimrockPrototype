#include "Magic/GridSpellHotbarExecution.h"

bool FGridSpellHotbarExecutionService::TryExecute (
    const FGridSpellDefinition& Definition,
    const FGridSpellCastRequest& Request,
    const FGridSpellTargetingContext& TargetingContext,
    const FGridCharacterSpellbookState& Spellbook,
    const FRPGDerivedStats& CasterStats,
    const FGridPlayerCharacterTurnState& CasterTurnState,
    int32 TargetMaxHealth,
    int32 TargetCurrentHealth,
    const FGridStatusEffectCollection& TargetStatusEffects,
    TFunctionRef<const UGridStatusEffectDefinitionAsset* (FName)>
        StatusDefinitionResolver,
    FGridSpellHotbarExecutionResult& OutResult)
{
    OutResult = FGridSpellHotbarExecutionResult ();

    FRPGDerivedStats WorkingCasterStats = CasterStats;
    FGridPlayerCharacterTurnState WorkingTurnState = CasterTurnState;
    FGridSpellResolvedTarget ResolvedTarget;
    FGridSpellCastCostReceipt CostReceipt;
    if (!FGridSpellCastPipelineService::TryValidateTargetAndCommitCosts (
            Definition,
            Request,
            TargetingContext,
            Spellbook,
            WorkingCasterStats,
            WorkingTurnState,
            ResolvedTarget,
            CostReceipt,
            OutResult.PipelineRejectStage,
            OutResult.TargetingRejectReason,
            OutResult.TransactionRejectReason))
    {
        return false;
    }

    int32 WorkingTargetHealth = TargetCurrentHealth;
    FGridStatusEffectCollection WorkingTargetStatuses = TargetStatusEffects;
    FGridSpellEffectResolutionResult EffectResult;
    if (!FGridSpellEffectResolver::ResolveEffects (
            Definition,
            Request.CasterCharacterId,
            TargetMaxHealth,
            WorkingTargetHealth,
            WorkingTargetStatuses,
            StatusDefinitionResolver,
            EffectResult,
            OutResult.EffectRejectReason,
            OutResult.Error))
    {
        // Costs were only committed to local copies. The caller's authoritative
        // caster state therefore remains untouched on any effect rejection.
        return false;
    }

    OutResult.CasterStats = MoveTemp (WorkingCasterStats);
    OutResult.CasterTurnState = MoveTemp (WorkingTurnState);
    OutResult.TargetCurrentHealth = WorkingTargetHealth;
    OutResult.TargetStatusEffects = MoveTemp (WorkingTargetStatuses);
    OutResult.ResolvedTarget = ResolvedTarget;
    OutResult.CostReceipt = CostReceipt;
    OutResult.EffectResult = MoveTemp (EffectResult);
    return true;
}
