#include "Runtime/Map/GridMapReadModel.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridDungeonAsset.h"
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

	struct FGlobalBoundaryKey
	{
		int32 Orientation = 0;
		int32 Line = 0;
		int32 Segment = 0;

		bool operator==(const FGlobalBoundaryKey& Other) const
		{
			return Orientation == Other.Orientation && Line == Other.Line && Segment == Other.Segment;
		}

		friend uint32 GetTypeHash(const FGlobalBoundaryKey& Key)
		{
			return HashCombine(HashCombine(::GetTypeHash(Key.Orientation), ::GetTypeHash(Key.Line)), ::GetTypeHash(Key.Segment));
		}
	};

	bool TryMakeGlobalBoundaryKey(const FIntPoint& MapCell, EGridEdge Edge, FGlobalBoundaryKey& OutKey)
	{
		switch (Edge)
		{
			case EGridEdge::North:
				OutKey = { 0, MapCell.Y + 1, MapCell.X };
				return true;
			case EGridEdge::South:
				OutKey = { 0, MapCell.Y, MapCell.X };
				return true;
			case EGridEdge::East:
				OutKey = { 1, MapCell.X + 1, MapCell.Y };
				return true;
			case EGridEdge::West:
				OutKey = { 1, MapCell.X, MapCell.Y };
				return true;
			default:
				return false;
		}
	}

	FIntPoint ToGlobalMapCell(const FIntVector& LogicalPosition, const FIntPoint& LocalCell)
	{
		return FIntPoint(
			LogicalPosition.X * FGridMapExplorationState::GridSize + LocalCell.X,
			LogicalPosition.Y * FGridMapExplorationState::GridSize + LocalCell.Y);
	}

	void MergeFloorBoundary(
		const FGridMapFloorBoundaryView& Incoming,
		TMap<FGlobalBoundaryKey, int32>& BoundaryIndexByKey,
		FGridMapFloorView& OutView)
	{
		FGlobalBoundaryKey Key;
		if (!TryMakeGlobalBoundaryKey(Incoming.MapCell, Incoming.Edge, Key))
		{
			return;
		}

		if (const int32* ExistingIndex = BoundaryIndexByKey.Find(Key))
		{
			if (!OutView.Boundaries.IsValidIndex(*ExistingIndex))
			{
				return;
			}

			FGridMapFloorBoundaryView& Existing = OutView.Boundaries[*ExistingIndex];
			if (Existing.Kind != Incoming.Kind)
			{
				// Contradictory authored data on a shared technical seam fails closed.
				Existing.Kind = EGridMapBoundaryKind::Wall;
				Existing.bDoorOpen = false;
			}
			else if (Existing.Kind != EGridMapBoundaryKind::Wall)
			{
				// If duplicate door metadata disagrees, closed is the conservative projection.
				Existing.bDoorOpen = Existing.bDoorOpen && Incoming.bDoorOpen;
			}
			return;
		}

		const int32 NewIndex = OutView.Boundaries.Add(Incoming);
		BoundaryIndexByKey.Add(Key, NewIndex);
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


void FGridMapReadModelBuilder::GetAvailableFloorZs(const UGridDungeonAsset& DungeonAsset, TArray<int32>& OutFloorZs)
{
	OutFloorZs.Reset();
	for (const FGridDungeonLevelEntry& Entry : DungeonAsset.Levels)
	{
		if (!Entry.bEnabled || Entry.LevelId.IsNone() || !Entry.LevelAsset)
		{
			continue;
		}
		OutFloorZs.AddUnique(Entry.LogicalPosition.Z);
	}
	OutFloorZs.Sort();
}

bool FGridMapReadModelBuilder::BuildFloorView(
	const UGridDungeonAsset& DungeonAsset,
	const FGridDungeonRuntimeState& DungeonState,
	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>>& WorldObjectDefinitions,
	FName ActiveLevelId,
	const FIntPoint& ActivePartyCell,
	EGridEdge ActivePartyFacing,
	int32 SelectedFloorZ,
	const UGridDoorSystemComponent* ActiveDoorSystem,
	FGridMapFloorView& OutView)
{
	using namespace GridMapReadModelPrivate;

	OutView.Reset();

	TArray<int32> AvailableFloorZs;
	GetAvailableFloorZs(DungeonAsset, AvailableFloorZs);
	if (!AvailableFloorZs.Contains(SelectedFloorZ))
	{
		return false;
	}

	TArray<int32> SelectedEntryIndices;
	for (int32 Index = 0; Index < DungeonAsset.Levels.Num(); ++Index)
	{
		const FGridDungeonLevelEntry& Entry = DungeonAsset.Levels[Index];
		if (Entry.bEnabled && !Entry.LevelId.IsNone() && Entry.LevelAsset && Entry.LogicalPosition.Z == SelectedFloorZ)
		{
			SelectedEntryIndices.Add(Index);
		}
	}

	SelectedEntryIndices.Sort(
		[&DungeonAsset](int32 LeftIndex, int32 RightIndex)
		{
			const FGridDungeonLevelEntry& Left = DungeonAsset.Levels[LeftIndex];
			const FGridDungeonLevelEntry& Right = DungeonAsset.Levels[RightIndex];
			if (Left.LogicalPosition.X != Right.LogicalPosition.X)
			{
				return Left.LogicalPosition.X < Right.LogicalPosition.X;
			}
			if (Left.LogicalPosition.Y != Right.LogicalPosition.Y)
			{
				return Left.LogicalPosition.Y < Right.LogicalPosition.Y;
			}
			return Left.LevelId.LexicalLess(Right.LevelId);
		});

	FGridMapFloorView Result;
	Result.SelectedFloorZ = SelectedFloorZ;
	Result.AvailableFloorZs = AvailableFloorZs;
	TMap<FGlobalBoundaryKey, int32> BoundaryIndexByKey;

	for (const int32 EntryIndex : SelectedEntryIndices)
	{
		const FGridDungeonLevelEntry& Entry = DungeonAsset.Levels[EntryIndex];

		FGridLevelRuntimeState EmptyState;
		EmptyState.LevelId = Entry.LevelId;
		const FGridLevelRuntimeState* LevelState = DungeonState.LevelStates.Find(Entry.LevelId);
		if (!LevelState)
		{
			LevelState = &EmptyState;
		}

		FGridMapTileView TileView;
		const UGridDoorSystemComponent* LiveDoorSystem = Entry.LevelId == ActiveLevelId ? ActiveDoorSystem : nullptr;
		if (!BuildTileView(Entry.LevelId, *Entry.LevelAsset, *LevelState, WorldObjectDefinitions, LiveDoorSystem, TileView))
		{
			return false;
		}

		for (const FGridMapCellView& TileCell : TileView.Cells)
		{
			FGridMapFloorCellView& FloorCell = Result.Cells.AddDefaulted_GetRef();
			FloorCell.MapCell = ToGlobalMapCell(Entry.LogicalPosition, TileCell.LocalCell);
			FloorCell.CellType = TileCell.CellType;
		}

		for (const FGridMapBoundaryView& TileBoundary : TileView.Boundaries)
		{
			FGridMapFloorBoundaryView FloorBoundary;
			FloorBoundary.MapCell = ToGlobalMapCell(Entry.LogicalPosition, TileBoundary.LocalCell);
			FloorBoundary.Edge = TileBoundary.Edge;
			FloorBoundary.Kind = TileBoundary.Kind;
			FloorBoundary.bDoorOpen = TileBoundary.bDoorOpen;
			MergeFloorBoundary(FloorBoundary, BoundaryIndexByKey, Result);
		}
	}

	const FGridDungeonLevelEntry* ActiveEntry = DungeonAsset.FindLevelEntry(ActiveLevelId);
	if (ActiveEntry && ActiveEntry->bEnabled && ActiveEntry->LevelAsset && ActiveEntry->LogicalPosition.Z == SelectedFloorZ &&
		FGridMapExplorationState::IsValidCell(ActivePartyCell))
	{
		Result.bHasPartyMarker = true;
		Result.PartyMapCell = ToGlobalMapCell(ActiveEntry->LogicalPosition, ActivePartyCell);
		Result.PartyFacing = ActivePartyFacing;
	}

	OutView = MoveTemp(Result);
	return true;
}
