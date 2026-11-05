#include "Runtime/Combat/GridCatalystAreaResolver.h"

#include "Runtime/Combat/GridCombatSurfaceResolver.h"

bool FGridCatalystAreaResolver::IsApprovedItem(FName ItemId, FName ActionId)
{
	return ItemId == FName(TEXT("Item_SurfaceCatalyst_01")) &&
		ActionId == FName(TEXT("Action_Mage_SurfaceCatalyst_01"));
}

bool FGridCatalystAreaResolver::IsApprovedAction(const FGridCombatActionDefinition& Action)
{
	return Action.ActionId == FName(TEXT("Action_Mage_SurfaceCatalyst_01")) &&
		Action.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
		Action.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Action.TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		Action.AreaShape == EGridCombatAreaShape::Square &&
		Action.AreaRadiusCells == 1 &&
		Action.SurfaceInteraction == EGridCombatSurfaceInteraction::Fire &&
		Action.bAllowNoSurfaceReaction;
}

void FGridCatalystAreaResolver::BuildReachableSquare(
	FIntPoint Center, int32 Radius,
	TFunctionRef<bool(FIntPoint)> IsWalkable,
	TFunctionRef<bool(FIntPoint, EGridEdge)> CanTraverse,
	TArray<FIntPoint>& OutCells)
{
	OutCells.Reset();
	if (Radius < 1 || Radius > 8 || !IsWalkable(Center))
	{
		return;
	}
	TSet<FIntPoint> Reached;
	Reached.Add(Center);
	OutCells.Add(Center);
	const EGridEdge Directions[] = { EGridEdge::North, EGridEdge::East, EGridEdge::South, EGridEdge::West };
	const FIntPoint Offsets[] = { FIntPoint(0, 1), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(-1, 0) };
	for (int32 Index = 0; Index < OutCells.Num(); ++Index)
	{
		const FIntPoint From = OutCells[Index];
		for (int32 DirectionIndex = 0; DirectionIndex < 4; ++DirectionIndex)
		{
			const FIntPoint Next = From + Offsets[DirectionIndex];
			if (FMath::Max(FMath::Abs(Next.X - Center.X), FMath::Abs(Next.Y - Center.Y)) > Radius ||
				Reached.Contains(Next) || !IsWalkable(Next) || !CanTraverse(From, Directions[DirectionIndex]))
			{
				continue;
			}
			Reached.Add(Next);
			OutCells.Add(Next);
		}
	}
	OutCells.Sort([Center](const FIntPoint& A, const FIntPoint& B)
	{
		const int32 DA = FMath::Abs(A.X - Center.X) + FMath::Abs(A.Y - Center.Y);
		const int32 DB = FMath::Abs(B.X - Center.X) + FMath::Abs(B.Y - Center.Y);
		return DA == DB ? (A.Y == B.Y ? A.X < B.X : A.Y < B.Y) : DA < DB;
	});
}

bool FGridCatalystAreaResolver::ResolveSnapshot(
	const TArray<FGridCatalystCellSnapshot>& Snapshot,
	EGridCombatSurfaceInteraction Interaction,
	const FGridResolvedCombatModifiers& Modifiers,
	TArray<FGridCatalystCellOutcome>& OutOutcomes)
{
	OutOutcomes.Reset();
	if (Snapshot.IsEmpty() || Interaction == EGridCombatSurfaceInteraction::None)
	{
		return false;
	}
	TSet<FIntPoint> Seen;
	for (const FGridCatalystCellSnapshot& Input : Snapshot)
	{
		if (Seen.Contains(Input.Cell))
		{
			OutOutcomes.Reset();
			return false;
		}
		Seen.Add(Input.Cell);
		FGridCatalystCellOutcome& Output = OutOutcomes.AddDefaulted_GetRef();
		Output.Cell = Input.Cell;
		if (Input.bHasSurface && Input.Surface.IsValid())
		{
			Output.InputType = Input.Surface.SurfaceType;
			// No reaction is a valid Fizzle: do not mutate a dry/wet cell.
			FGridCombatSurfaceResolver::ResolveReaction(Input.Surface, Interaction, Modifiers, Output.Reaction);
		}
	}
	return true;
}
