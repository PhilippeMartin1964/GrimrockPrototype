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


bool FGridCombatMovementResolver::CanRelocatePartyToSelectedCell(const FIntPoint& FromCell, const FIntPoint& TargetCell, int32 RangeCells,
	const AGridLevelRuntimeActor* RuntimeActor, const UGridMonsterOccupancySubsystem* Occupancy)
{
	if (!IsValid(RuntimeActor) || RangeCells < 1 || RangeCells > 32 || FromCell == TargetCell ||
		!RuntimeActor->IsValidCell(TargetCell.X, TargetCell.Y) || !RuntimeActor->IsWalkableCell(TargetCell.X, TargetCell.Y))
	{
		return false;
	}
	const int32 ManhattanDistance = FMath::Abs(TargetCell.X - FromCell.X) + FMath::Abs(TargetCell.Y - FromCell.Y);
	if (ManhattanDistance <= 0 || ManhattanDistance > RangeCells)
	{
		return false;
	}
	if (IsValid(Occupancy) && IsValid(Occupancy->GetOccupantAtCell(TargetCell)))
	{
		return false;
	}

	struct FOpenCell
	{
		FIntPoint Cell = FIntPoint::ZeroValue;
		int32 Distance = 0;
	};
	TArray<FOpenCell> Queue;
	FOpenCell Start;
	Start.Cell = FromCell;
	Start.Distance = 0;
	Queue.Add(Start);
	TSet<FIntPoint> Visited;
	Visited.Add(FromCell);

	for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
	{
		const FOpenCell Current = Queue[QueueIndex];
		if (Current.Distance >= RangeCells)
		{
			continue;
		}
		for (const EGridEdge Direction : { EGridEdge::North, EGridEdge::East, EGridEdge::South, EGridEdge::West })
		{
			int32 NextX = INDEX_NONE;
			int32 NextY = INDEX_NONE;
			if (!RuntimeActor->TryGetNeighborCell(Current.Cell.X, Current.Cell.Y, Direction, NextX, NextY) ||
				!RuntimeActor->CanMove(Current.Cell.X, Current.Cell.Y, Direction))
			{
				continue;
			}
			const FIntPoint NextCell(NextX, NextY);
			if (!RuntimeActor->IsWalkableCell(NextCell.X, NextCell.Y) || Visited.Contains(NextCell))
			{
				continue;
			}
			if (NextCell == TargetCell)
			{
				return true;
			}
			Visited.Add(NextCell);
			FOpenCell Next;
			Next.Cell = NextCell;
			Next.Distance = Current.Distance + 1;
			Queue.Add(Next);
		}
	}
	return false;
}
