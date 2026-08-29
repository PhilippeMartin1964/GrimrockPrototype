#include "Runtime/Monsters/GridMonsterPathfinder.h"

#include "Algo/Reverse.h"

namespace
{
	bool ContainsGoal(const TArray<FIntPoint>& Goals, const FIntPoint& Cell)
	{
		return Goals.Contains(Cell);
	}

	void ReconstructPath(const FIntPoint& Start, const FIntPoint& Goal, const TMap<FIntPoint, FIntPoint>& CameFrom, TArray<FIntPoint>& OutCells)
	{
		OutCells.Reset();
		if (Goal == Start)
		{
			return;
		}

		FIntPoint Current = Goal;
		while (Current != Start)
		{
			OutCells.Add(Current);
			const FIntPoint* Previous = CameFrom.Find(Current);
			if (!Previous)
			{
				OutCells.Reset();
				return;
			}
			Current = *Previous;
		}
		Algo::Reverse(OutCells);
	}
}

bool FGridMonsterPathfinder::FindPath(const FGridMonsterPathQuery& Query, const FGridMonsterPathContext& Context, FGridMonsterPathResult& OutResult)
{
	OutResult.Reset();

	if (!Context.IsComplete() || Query.Goals.IsEmpty() || Query.MaxVisitedCells <= 0 || !Context.IsValidCell(Query.Start) ||
		!Context.IsWalkableCell(Query.Start))
	{
		return false;
	}

	TArray<FIntPoint> ValidGoals;
	ValidGoals.Reserve(Query.Goals.Num());
	for (const FIntPoint& Goal : Query.Goals)
	{
		if (!Context.IsValidCell(Goal) || !Context.IsWalkableCell(Goal))
		{
			continue;
		}
		if (!Query.bAllowBlockedGoal && Context.IsCellBlocked(Goal) && Goal != Query.Start)
		{
			continue;
		}
		ValidGoals.AddUnique(Goal);
	}

	if (ValidGoals.IsEmpty())
	{
		return false;
	}

	if (ContainsGoal(ValidGoals, Query.Start))
	{
		OutResult.bFound = true;
		OutResult.ReachedGoal = Query.Start;
		OutResult.VisitedCellCount = 1;
		return true;
	}

	TArray<FIntPoint> Queue;
	Queue.Reserve(FMath::Min(Query.MaxVisitedCells, 1024));
	Queue.Add(Query.Start);

	TSet<FIntPoint> Visited;
	Visited.Reserve(FMath::Min(Query.MaxVisitedCells, 1024));
	Visited.Add(Query.Start);

	TMap<FIntPoint, FIntPoint> CameFrom;
	CameFrom.Reserve(FMath::Min(Query.MaxVisitedCells, 1024));

	int32 QueueIndex = 0;
	while (Queue.IsValidIndex(QueueIndex) && Visited.Num() <= Query.MaxVisitedCells)
	{
		const FIntPoint Current = Queue[QueueIndex++];
		++OutResult.VisitedCellCount;

		for (const EGridEdge Direction : GetOrderedDirections())
		{
			const FIntPoint Neighbor = GetNeighborCell(Current, Direction);
			if (Visited.Contains(Neighbor) || !Context.IsValidCell(Neighbor) || !Context.IsWalkableCell(Neighbor) || !Context.CanTraverse(Current, Neighbor))
			{
				continue;
			}

			const bool bIsGoal = ContainsGoal(ValidGoals, Neighbor);
			if (Context.IsCellBlocked(Neighbor) && !(bIsGoal && Query.bAllowBlockedGoal))
			{
				continue;
			}

			Visited.Add(Neighbor);
			CameFrom.Add(Neighbor, Current);

			if (bIsGoal)
			{
				OutResult.bFound = true;
				OutResult.ReachedGoal = Neighbor;
				ReconstructPath(Query.Start, Neighbor, CameFrom, OutResult.Cells);
				return true;
			}

			if (Visited.Num() >= Query.MaxVisitedCells)
			{
				break;
			}
			Queue.Add(Neighbor);
		}
	}

	return false;
}

const TArray<EGridEdge>& FGridMonsterPathfinder::GetOrderedDirections()
{
	static const TArray<EGridEdge> Directions = { EGridEdge::North, EGridEdge::East, EGridEdge::South, EGridEdge::West };
	return Directions;
}

FIntPoint FGridMonsterPathfinder::GetNeighborCell(const FIntPoint& Cell, EGridEdge Direction)
{
	switch (Direction)
	{
		case EGridEdge::North:
			return FIntPoint(Cell.X, Cell.Y + 1);
		case EGridEdge::East:
			return FIntPoint(Cell.X + 1, Cell.Y);
		case EGridEdge::South:
			return FIntPoint(Cell.X, Cell.Y - 1);
		case EGridEdge::West:
			return FIntPoint(Cell.X - 1, Cell.Y);
		default:
			return Cell;
	}
}

EGridEdge FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(const FIntPoint& From, const FIntPoint& To)
{
	const FIntPoint Delta = To - From;
	if (Delta == FIntPoint(0, 1))
	{
		return EGridEdge::North;
	}
	if (Delta == FIntPoint(1, 0))
	{
		return EGridEdge::East;
	}
	if (Delta == FIntPoint(0, -1))
	{
		return EGridEdge::South;
	}
	if (Delta == FIntPoint(-1, 0))
	{
		return EGridEdge::West;
	}
	return EGridEdge::None;
}

int32 FGridMonsterPathfinder::ManhattanDistance(const FIntPoint& A, const FIntPoint& B)
{
	return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

bool FGridMonsterPerception::HasStraightLineOfSight(
	const FIntPoint& ObserverCell, const FIntPoint& TargetCell, int32 SightRangeCells, const TFunction<bool(const FIntPoint&, const FIntPoint&)>& CanTraverse)
{
	if (!CanTraverse || SightRangeCells < 0)
	{
		return false;
	}

	const int32 Distance = FGridMonsterPathfinder::ManhattanDistance(ObserverCell, TargetCell);
	if (Distance > SightRangeCells)
	{
		return false;
	}
	if (ObserverCell == TargetCell)
	{
		return true;
	}
	if (ObserverCell.X != TargetCell.X && ObserverCell.Y != TargetCell.Y)
	{
		return false;
	}

	const FIntPoint Step(FMath::Clamp(TargetCell.X - ObserverCell.X, -1, 1), FMath::Clamp(TargetCell.Y - ObserverCell.Y, -1, 1));

	FIntPoint Current = ObserverCell;
	while (Current != TargetCell)
	{
		const FIntPoint Next = Current + Step;
		if (!CanTraverse(Current, Next))
		{
			return false;
		}
		Current = Next;
	}
	return true;
}

bool FGridMonsterPerception::IsTargetInFacingDirection(const FIntPoint& ObserverCell, EGridEdge Facing, const FIntPoint& TargetCell)
{
	if (ObserverCell == TargetCell)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
	}

	switch (Facing)
	{
		case EGridEdge::North:
			return TargetCell.X == ObserverCell.X && TargetCell.Y > ObserverCell.Y;

		case EGridEdge::East:
			return TargetCell.Y == ObserverCell.Y && TargetCell.X > ObserverCell.X;

		case EGridEdge::South:
			return TargetCell.X == ObserverCell.X && TargetCell.Y < ObserverCell.Y;

		case EGridEdge::West:
			return TargetCell.Y == ObserverCell.Y && TargetCell.X < ObserverCell.X;

		case EGridEdge::None:
		default:
			return false;
	}
}

bool FGridMonsterPerception::HasDirectionalLineOfSight(const FIntPoint& ObserverCell, EGridEdge Facing, const FIntPoint& TargetCell, int32 SightRangeCells,
	const TFunction<bool(const FIntPoint&, const FIntPoint&)>& CanTraverse)
{
	return IsTargetInFacingDirection(ObserverCell, Facing, TargetCell) && HasStraightLineOfSight(ObserverCell, TargetCell, SightRangeCells, CanTraverse);
}

bool FGridMonsterPerception::CanHear(const FIntPoint& ObserverCell, const FIntPoint& TargetCell, int32 HearingRangeCells)
{
	return HearingRangeCells >= 0 && FGridMonsterPathfinder::ManhattanDistance(ObserverCell, TargetCell) <= HearingRangeCells;
}

bool FGridMonsterPerception::CanHearThroughGrid(const FIntPoint& ObserverCell, const FIntPoint& TargetCell, int32 HearingRangeCells,
	const TFunction<bool(const FIntPoint&, const FIntPoint&)>& CanSoundTraverse)
{
	if (!CanSoundTraverse || HearingRangeCells < 0)
	{
		return false;
	}
	if (ObserverCell == TargetCell)
	{
		return true;
	}
	if (HearingRangeCells == 0)
	{
		return false;
	}
	if (FGridMonsterPathfinder::ManhattanDistance(ObserverCell, TargetCell) > HearingRangeCells)
	{
		return false;
	}

	struct FAcousticNode
	{
		FIntPoint Cell = FIntPoint::ZeroValue;
		int32 Distance = 0;
	};

	TArray<FAcousticNode> Queue;
	Queue.Add({ ObserverCell, 0 });

	TSet<FIntPoint> Visited;
	Visited.Add(ObserverCell);

	int32 QueueIndex = 0;
	while (Queue.IsValidIndex(QueueIndex))
	{
		const FAcousticNode Current = Queue[QueueIndex++];
		if (Current.Distance >= HearingRangeCells)
		{
			continue;
		}

		for (const EGridEdge Direction : FGridMonsterPathfinder::GetOrderedDirections())
		{
			const FIntPoint Neighbor = FGridMonsterPathfinder::GetNeighborCell(Current.Cell, Direction);
			if (Visited.Contains(Neighbor) || !CanSoundTraverse(Current.Cell, Neighbor))
			{
				continue;
			}

			const int32 NeighborDistance = Current.Distance + 1;
			if (Neighbor == TargetCell)
			{
				return true;
			}

			Visited.Add(Neighbor);
			Queue.Add({ Neighbor, NeighborDistance });
		}
	}

	return false;
}
