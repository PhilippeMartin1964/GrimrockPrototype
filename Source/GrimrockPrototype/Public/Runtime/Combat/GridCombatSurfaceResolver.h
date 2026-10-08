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

	/**
	 * Bridges an incoming authored surface effect to the canonical elemental reaction table.
	 * Only surface types that directly represent a canonical interaction are mapped.
	 */
	static bool ResolveAppliedSurfaceReaction(const FGridCombatSurfaceState& ExistingSurface,
		const FGridCombatSurfaceEffectProfile& IncomingSurface,
		const FGridResolvedCombatModifiers& SourceModifiers,
		FGridCombatSurfaceReactionResult& OutResult);

	/**
	 * Resolves the optional pre-conversion canonical reaction authored by a
	 * conversion profile. The conversion itself is applied separately afterwards.
	 */
	static bool ResolveConversionReaction(const FGridCombatSurfaceState& ExistingSurface,
		const FGridCombatSurfaceConversionProfile& Conversion,
		const FGridResolvedCombatModifiers& SourceModifiers,
		FGridCombatSurfaceReactionResult& OutResult);

	/** Resolve one authored conversion against an existing surface or an empty cell. */
	static bool ResolveConversion(const FGridCombatSurfaceConversionProfile& Profile, const FGridCombatSurfaceState* ExistingSurface,
		const FGuid& SourceCombatantId, FName SourceActionId, const FGridResolvedCombatModifiers& SourceModifiers,
		FGridCombatSurfaceState& OutState);

	static void ApplyReactionToState(
		const FGridCombatSurfaceReactionResult& Reaction, FGridCombatSurfaceState& InOutState);
};
