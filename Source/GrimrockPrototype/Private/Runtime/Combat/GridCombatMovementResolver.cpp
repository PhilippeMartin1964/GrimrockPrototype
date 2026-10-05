#include "Runtime/Combat/GridCombatMovementResolver.h"

#include "Core/GridDirectionUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
	EGridEdge ResolveAwayDirection(const FIntPoint& SubjectCell, const FIntPoint& SourceCell)
	{
		const FIntPoint Delta = SubjectCell - SourceCell;
		if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y) && Delta.X != 0)
		{
			return Delta.X > 0 ? EGridEdge::East : EGridEdge::West;
		}
		if (Delta.Y != 0)
		{
			return Delta.Y > 0 ? EGridEdge::North : EGridEdge::South;
		}
		return EGridEdge::None;
	}
}

EGridEdge FGridCombatMovementResolver::ResolveDirection(
	EGridCombatMovementDirection Direction, EGridEdge SubjectFacing, const FIntPoint& SubjectCell, const FIntPoint& SourceCell)
{
	switch (Direction)
	{
		case EGridCombatMovementDirection::ForwardFromFacing:
			return GridDirectionUtils::GetForward(SubjectFacing);
		case EGridCombatMovementDirection::BackwardFromFacing:
			return GridDirectionUtils::GetBackward(SubjectFacing);
		case EGridCombatMovementDirection::LeftFromFacing:
			return GridDirectionUtils::GetLeft(SubjectFacing);
		case EGridCombatMovementDirection::RightFromFacing:
			return GridDirectionUtils::GetRight(SubjectFacing);
		case EGridCombatMovementDirection::AwayFromSource:
			return ResolveAwayDirection(SubjectCell, SourceCell);
		default:
			return EGridEdge::None;
	}
}

bool FGridCombatMovementResolver::ResolveDestination(const FGridCombatMovementEffectProfile& Profile, const FIntPoint& FromCell,
	EGridEdge SubjectFacing, const FIntPoint& SourceCell, const AGridLevelRuntimeActor* RuntimeActor, const UGridMonsterOccupancySubsystem* Occupancy,
	FGridCombatMovementResolution& OutResolution)
{
	OutResolution = FGridCombatMovementResolution();
	if (!Profile.IsValid() || !IsValid(RuntimeActor))
	{
		return false;
	}

	const EGridEdge Direction = ResolveDirection(Profile.Direction, SubjectFacing, FromCell, SourceCell);
	if (!GridDirectionUtils::IsCardinal(Direction))
	{
		return false;
	}

	FIntPoint Current = FromCell;
	for (int32 Step = 0; Step < Profile.DistanceCells; ++Step)
	{
		int32 NextX = INDEX_NONE;
		int32 NextY = INDEX_NONE;
		if (!RuntimeActor->TryGetNeighborCell(Current.X, Current.Y, Direction, NextX, NextY) ||
			!RuntimeActor->CanMove(Current.X, Current.Y, Direction))
		{
			return false;
		}

		const FIntPoint Next(NextX, NextY);
		if (IsValid(Occupancy) && IsValid(Occupancy->GetOccupantAtCell(Next)))
		{
			return false;
		}
		Current = Next;
	}

	OutResolution.FromCell = FromCell;
	OutResolution.ToCell = Current;
	OutResolution.Direction = Direction;
	OutResolution.DistanceCells = Profile.DistanceCells;
	return OutResolution.IsValid();
}
