#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Templates/Function.h"

/** Immutable source state of one cell, read before catalyst mutations. */
struct FGridCatalystCellSnapshot
{
	FIntPoint Cell = FIntPoint::ZeroValue;
	FGridCombatSurfaceState Surface;
	bool bHasSurface = false;
};

/** One independently classified cell; non-reactive cells are valid outcomes. */
struct FGridCatalystCellOutcome
{
	FIntPoint Cell = FIntPoint::ZeroValue;
	EGridCombatSurfaceType InputType = EGridCombatSurfaceType::None;
	FGridCombatSurfaceReactionResult Reaction;
};

/** ARMEMENT01.17.3A: pure area and C6 classification reusable in combat/exploration. */
class FGridCatalystAreaResolver
{
public:
	static bool IsApprovedItem(FName ItemId, FName ActionId);
	static bool IsApprovedAction(const FGridCombatActionDefinition& Action);

	/** Flood inside square; walls/closed doors stop propagation. */
	static void BuildReachableSquare(
		FIntPoint Center, int32 Radius,
		TFunctionRef<bool(FIntPoint)> IsWalkable,
		TFunctionRef<bool(FIntPoint, EGridEdge)> CanTraverse,
		TArray<FIntPoint>& OutCells);

	/** Exactly one outcome per cell, preserving snapshot order. */
	static bool ResolveSnapshot(
		const TArray<FGridCatalystCellSnapshot>& Snapshot,
		EGridCombatSurfaceInteraction Interaction,
		const FGridResolvedCombatModifiers& Modifiers,
		TArray<FGridCatalystCellOutcome>& OutOutcomes);
};
