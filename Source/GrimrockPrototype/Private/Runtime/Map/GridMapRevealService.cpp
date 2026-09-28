#include "Runtime/Map/GridMapRevealService.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/Map/GridMapExplorationState.h"

namespace GridMapRevealPrivate
{
	bool TryGetNeighbour(const UGridLevelAsset& LevelAsset, const FIntPoint& Cell, EGridEdge Direction, FIntPoint& OutNeighbour)
	{
		OutNeighbour = Cell;
		switch (Direction)
		{
			case EGridEdge::North:
				++OutNeighbour.Y;
				break;
			case EGridEdge::East:
				++OutNeighbour.X;
				break;
			case EGridEdge::South:
				--OutNeighbour.Y;
				break;
			case EGridEdge::West:
				--OutNeighbour.X;
				break;
			default:
				return false;
		}
		return LevelAsset.IsValidCoord(OutNeighbour.X, OutNeighbour.Y);
	}

	EGridWallType GetWall(const UGridLevelAsset& LevelAsset, const FIntPoint& Cell, EGridEdge Edge)
	{
		if (!LevelAsset.IsValidCoord(Cell.X, Cell.Y))
		{
			return EGridWallType::Solid;
		}

		const FGridLevelCellData& CellData = LevelAsset.GetCell(Cell.X, Cell.Y);
		switch (Edge)
		{
			case EGridEdge::North:
				return CellData.NorthWall;
			case EGridEdge::East:
				return CellData.EastWall;
			case EGridEdge::South:
				return CellData.SouthWall;
			case EGridEdge::West:
				return CellData.WestWall;
			default:
				return EGridWallType::Solid;
		}
	}

	bool HasDoorAtExactEdge(const UGridLevelAsset& LevelAsset, const FIntPoint& Cell, EGridEdge Edge)
	{
		return LevelAsset.WorldObjectInstances.ContainsByPredicate(
			[Cell, Edge](const FGridWorldObjectInstance& Instance)
			{
				return Instance.Type == EGridLevelObjectType::Door && Instance.CellX == Cell.X && Instance.CellY == Cell.Y && Instance.WallSide == Edge;
			});
	}

	bool IsMapCell(const UGridLevelAsset& LevelAsset, const FIntPoint& Cell)
	{
		return LevelAsset.IsValidCoord(Cell.X, Cell.Y) && LevelAsset.GetCell(Cell.X, Cell.Y).CellType != EGridCellType::Empty;
	}
}

bool FGridMapRevealService::CanRevealAcrossEdge(
	const UGridLevelAsset& LevelAsset, const UGridDoorSystemComponent* DoorSystem, const FIntPoint& FromCell, EGridEdge Direction)
{
	using namespace GridMapRevealPrivate;

	if (!GridDirectionUtils::IsCardinal(Direction) || !IsMapCell(LevelAsset, FromCell))
	{
		return false;
	}

	FIntPoint Neighbour;
	if (!TryGetNeighbour(LevelAsset, FromCell, Direction, Neighbour) || !IsMapCell(LevelAsset, Neighbour))
	{
		return false;
	}

	const EGridEdge Opposite = GridDirectionUtils::GetOpposite(Direction);
	if (Opposite == EGridEdge::None || GetWall(LevelAsset, FromCell, Direction) == EGridWallType::Solid ||
		GetWall(LevelAsset, Neighbour, Opposite) == EGridWallType::Solid)
	{
		return false;
	}

	const bool bDirectDoor = HasDoorAtExactEdge(LevelAsset, FromCell, Direction);
	const bool bReverseDoor = HasDoorAtExactEdge(LevelAsset, Neighbour, Opposite);
	if (!bDirectDoor && !bReverseDoor)
	{
		return true;
	}

	// Door data without a live door-system authority fails closed.
	if (!DoorSystem)
	{
		return false;
	}

	const bool bDoorIndexed = DoorSystem->HasDoorOnEdge(FromCell.X, FromCell.Y, Direction) ||
		DoorSystem->HasDoorOnEdge(Neighbour.X, Neighbour.Y, Opposite);
	if (!bDoorIndexed)
	{
		return false;
	}

	return !DoorSystem->IsDoorPassageBlocked(FromCell.X, FromCell.Y, Direction);
}

int32 FGridMapRevealService::RevealAroundCell(const UGridLevelAsset& LevelAsset, const UGridDoorSystemComponent* DoorSystem, const FIntPoint& Origin,
	FGridMapExplorationState& ExplorationState)
{
	static_assert(RevealRadiusCells >= 1.0f && RevealRadiusCells < 1.4142136f, "MON21.6.3 radius must include cardinals and exclude diagonals.");

	if (LevelAsset.Width != FGridMapExplorationState::GridSize || LevelAsset.Height != FGridMapExplorationState::GridSize ||
		!GridMapRevealPrivate::IsMapCell(LevelAsset, Origin))
	{
		return 0;
	}

	int32 NewlyExploredCount = 0;
	bool bNewlyExplored = false;
	if (ExplorationState.TryMarkExplored(Origin, bNewlyExplored) && bNewlyExplored)
	{
		++NewlyExploredCount;
	}

	static constexpr EGridEdge Directions[] = { EGridEdge::North, EGridEdge::East, EGridEdge::South, EGridEdge::West };
	for (const EGridEdge Direction : Directions)
	{
		if (!CanRevealAcrossEdge(LevelAsset, DoorSystem, Origin, Direction))
		{
			continue;
		}

		FIntPoint Neighbour;
		if (!GridMapRevealPrivate::TryGetNeighbour(LevelAsset, Origin, Direction, Neighbour))
		{
			continue;
		}

		bNewlyExplored = false;
		if (ExplorationState.TryMarkExplored(Neighbour, bNewlyExplored) && bNewlyExplored)
		{
			++NewlyExploredCount;
		}
	}

	return NewlyExploredCount;
}
