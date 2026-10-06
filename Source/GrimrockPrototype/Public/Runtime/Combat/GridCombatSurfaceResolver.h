#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatTypes.h"

/** Pure C6 surface state construction and canonical elemental reaction rules. */
class GRIMROCKPROTOTYPE_API FGridCombatSurfaceResolver
{
public:
	static bool BuildState(const FGridCombatSurfaceEffectProfile& Profile, const FGuid& SourceCombatantId, FName SourceActionId,
		const FGridResolvedCombatModifiers& SourceModifiers, FGridCombatSurfaceState& OutState);

	static bool ResolveReaction(const FGridCombatSurfaceState& ExistingSurface, EGridCombatSurfaceInteraction Interaction,
		const FGridResolvedCombatModifiers& SourceModifiers, FGridCombatSurfaceReactionResult& OutResult);

	/** Resolve one authored conversion against an existing surface or an empty cell. */
	static bool ResolveConversion(const FGridCombatSurfaceConversionProfile& Profile, const FGridCombatSurfaceState* ExistingSurface,
		const FGuid& SourceCombatantId, FName SourceActionId, const FGridResolvedCombatModifiers& SourceModifiers,
		FGridCombatSurfaceState& OutState);

	static void ApplyReactionToState(
		const FGridCombatSurfaceReactionResult& Reaction, FGridCombatSurfaceState& InOutState);
};
