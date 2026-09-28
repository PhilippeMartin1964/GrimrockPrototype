#include "Runtime/Map/GridMapReadModel.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridSecretDoorActor.h"
#include "Runtime/Map/GridMapExplorationState.h"

namespace GridMapReadModelPrivate
{
	enum class EDoorClassification : uint8
	{
		Unresolved,
		Normal,
		Secret
	};

	EGridWallType GetWall(const UGridLevelAsset& LevelAsset, const FIntPoint& Cell, EGridEdge Edge)
	{
		if (!LevelAsset.IsValidCoord(Cell.X, Cell.Y))
		{
			return EGridWallType::None;
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
				return EGridWallType::None;
		}
	}

	bool TryGetNeighbour(const UGridLevelAsset& LevelAsset, const FIntPoint& Cell, EGridEdge Edge, FIntPoint& OutNeighbour)
	{
		OutNeighbour = Cell;
		switch (Edge)
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

	uint32 MakePhysicalBoundaryKey(const FIntPoint& Cell, EGridEdge Edge)
	{
		uint32 Orientation = 0;
		uint32 Line = 0;
		uint32 Segment = 0;
		switch (Edge)
		{
			case EGridEdge::North:
				Orientation = 0;
				Line = static_cast<uint32>(Cell.Y + 1);
				Segment = static_cast<uint32>(Cell.X);
				break;
			case EGridEdge::South:
				Orientation = 0;
				Line = static_cast<uint32>(Cell.Y);
				Segment = static_cast<uint32>(Cell.X);
				break;
			case EGridEdge::East:
				Orientation = 1;
				Line = static_cast<uint32>(Cell.X + 1);
				Segment = static_cast<uint32>(Cell.Y);
				break;
			case EGridEdge::West:
				Orientation = 1;
				Line = static_cast<uint32>(Cell.X);
				Segment = static_cast<uint32>(Cell.Y);
				break;
			default:
				return MAX_uint32;
		}
		return (Orientation << 16) | (Line << 8) | Segment;
	}

	const FGridWorldObjectInstance* FindDoorAtBoundary(
		const TMap<FGridEdgeKey, const FGridWorldObjectInstance*>& DoorByEdge,
		const UGridLevelAsset& LevelAsset,
		const FIntPoint& Cell,
		EGridEdge Edge)
	{
		if (const FGridWorldObjectInstance* const* Direct = DoorByEdge.Find(FGridEdgeKey(Cell.X, Cell.Y, Edge)))
		{
			return *Direct;
		}

		FIntPoint Neighbour;
		if (!TryGetNeighbour(LevelAsset, Cell, Edge, Neighbour))
		{
			return nullptr;
		}
		const EGridEdge Opposite = GridDirectionUtils::GetOpposite(Edge);
		if (Opposite == EGridEdge::None)
		{
			return nullptr;
		}
		if (const FGridWorldObjectInstance* const* Reverse = DoorByEdge.Find(FGridEdgeKey(Neighbour.X, Neighbour.Y, Opposite)))
		{
			return *Reverse;
		}
		return nullptr;
	}

	EDoorClassification ClassifyDoor(
		const FGridWorldObjectInstance& Door,
		const TMap<FName, const UGridWorldObjectDefinitionAsset*>& DefinitionById)
	{
		const UGridWorldObjectDefinitionAsset* const* DefinitionPtr = DefinitionById.Find(Door.WorldObjectDefinitionId);
		const UGridWorldObjectDefinitionAsset* Definition = DefinitionPtr ? *DefinitionPtr : nullptr;
		UClass* RuntimeClass = Definition ? Definition->RuntimeActorClass.Get() : nullptr;
		if (!Definition || Definition->SupportedType != EGridLevelObjectType::Door || !RuntimeClass)
		{
			return EDoorClassification::Unresolved;
		}
		if (RuntimeClass->IsChildOf(AGridSecretDoorActor::StaticClass()))
		{
			return EDoorClassification::Secret;
		}
		if (RuntimeClass->IsChildOf(AGridDoorActor::StaticClass()))
		{
			return EDoorClassification::Normal;
		}
		return EDoorClassification::Unresolved;
	}

	bool ResolveDoorOpen(
		const FGridWorldObjectInstance& Door,
		const FGridLevelRuntimeState& LevelState,
		const UGridDoorSystemComponent* LiveDoorSystem)
	{
		if (LiveDoorSystem && LiveDoorSystem->HasDoorOnEdge(Door.CellX, Door.CellY, Door.WallSide))
		{
			return LiveDoorSystem->IsDoorOpenOnEdge(Door.CellX, Door.CellY, Door.WallSide);
		}

		if (const FGridRuntimeDoorState* Persisted = LevelState.Doors.Find(Door.InstanceId))
		{
			return !Persisted->bBlocksMovement;
		}

		return Door.InstanceConfig.bDoorInitiallyOpen;
	}

	bool IsSolidBoundary(const UGridLevelAsset& LevelAsset, const FIntPoint& Cell, EGridEdge Edge)
	{
		if (GetWall(LevelAsset, Cell, Edge) == EGridWallType::Solid)
		{
			return true;
		}

		FIntPoint Neighbour;
		if (!TryGetNeighbour(LevelAsset, Cell, Edge, Neighbour))
		{
			return false;
		}
		const EGridEdge Opposite = GridDirectionUtils::GetOpposite(Edge);
		return Opposite != EGridEdge::None && GetWall(LevelAsset, Neighbour, Opposite) == EGridWallType::Solid;
	}
}

bool FGridMapReadModelBuilder::BuildTileView(
	FName LevelId,
	const UGridLevelAsset& LevelAsset,
	const FGridLevelRuntimeState& LevelState,
	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>>& WorldObjectDefinitions,
	const UGridDoorSystemComponent* LiveDoorSystem,
	FGridMapTileView& OutView)
{
	using namespace GridMapReadModelPrivate;

	OutView.Reset();
	if (LevelId.IsNone() || LevelAsset.Width != FGridMapExplorationState::GridSize || LevelAsset.Height != FGridMapExplorationState::GridSize ||
		!LevelState.MapExploration.IsStructurallyValid())
	{
		return false;
	}

	TMap<FGridEdgeKey, const FGridWorldObjectInstance*> DoorByEdge;
	for (const FGridWorldObjectInstance& Instance : LevelAsset.WorldObjectInstances)
	{
		if (Instance.Type == EGridLevelObjectType::Door && Instance.InstanceId.IsValid() && GridDirectionUtils::IsCardinal(Instance.WallSide))
		{
			DoorByEdge.Add(FGridEdgeKey(Instance.CellX, Instance.CellY, Instance.WallSide), &Instance);
		}
	}

	TMap<FName, const UGridWorldObjectDefinitionAsset*> DefinitionById;
	for (const UGridWorldObjectDefinitionAsset* Definition : WorldObjectDefinitions)
	{
		if (Definition && !Definition->DefinitionId.IsNone())
		{
			DefinitionById.Add(Definition->DefinitionId, Definition);
		}
	}

	OutView.LevelId = LevelId;
	TSet<uint32> EmittedBoundaries;
	static constexpr EGridEdge Directions[] = { EGridEdge::North, EGridEdge::East, EGridEdge::South, EGridEdge::West };

	for (int32 Y = 0; Y < FGridMapExplorationState::GridSize; ++Y)
	{
		for (int32 X = 0; X < FGridMapExplorationState::GridSize; ++X)
		{
			const FIntPoint Cell(X, Y);
			if (!LevelState.MapExploration.IsExplored(Cell))
			{
				continue;
			}

			const FGridLevelCellData& CellData = LevelAsset.GetCell(X, Y);
			if (CellData.CellType == EGridCellType::Empty)
			{
				continue;
			}

			FGridMapCellView& CellView = OutView.Cells.AddDefaulted_GetRef();
			CellView.LocalCell = Cell;
			CellView.CellType = CellData.CellType;

			for (const EGridEdge Edge : Directions)
			{
				const uint32 BoundaryKey = MakePhysicalBoundaryKey(Cell, Edge);
				if (BoundaryKey == MAX_uint32 || EmittedBoundaries.Contains(BoundaryKey))
				{
					continue;
				}

				const FGridWorldObjectInstance* Door = FindDoorAtBoundary(DoorByEdge, LevelAsset, Cell, Edge);
				if (Door)
				{
					const EDoorClassification Classification = ClassifyDoor(*Door, DefinitionById);
					FGridMapBoundaryView Boundary;
					Boundary.LocalCell = Cell;
					Boundary.Edge = Edge;

					if (Classification == EDoorClassification::Secret)
					{
						if (LevelState.MapExploration.IsSecretDiscovered(Door->InstanceId))
						{
							Boundary.Kind = EGridMapBoundaryKind::SecretDoor;
							Boundary.bDoorOpen = ResolveDoorOpen(*Door, LevelState, LiveDoorSystem);
						}
						else
						{
							Boundary.Kind = EGridMapBoundaryKind::Wall;
							Boundary.bDoorOpen = false;
						}
					}
					else if (Classification == EDoorClassification::Normal)
					{
						Boundary.Kind = EGridMapBoundaryKind::Door;
						Boundary.bDoorOpen = ResolveDoorOpen(*Door, LevelState, LiveDoorSystem);
					}
					else
					{
						// Fail closed: unresolved door metadata can never expose a possible secret.
						Boundary.Kind = EGridMapBoundaryKind::Wall;
						Boundary.bDoorOpen = false;
					}

					OutView.Boundaries.Add(Boundary);
					EmittedBoundaries.Add(BoundaryKey);
					continue;
				}

				if (IsSolidBoundary(LevelAsset, Cell, Edge))
				{
					FGridMapBoundaryView& Boundary = OutView.Boundaries.AddDefaulted_GetRef();
					Boundary.LocalCell = Cell;
					Boundary.Edge = Edge;
					Boundary.Kind = EGridMapBoundaryKind::Wall;
					Boundary.bDoorOpen = false;
					EmittedBoundaries.Add(BoundaryKey);
				}
			}
		}
	}

	return true;
}
