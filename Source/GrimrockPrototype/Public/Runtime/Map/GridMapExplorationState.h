#pragma once

#include "CoreMinimal.h"
#include "GridMapExplorationState.generated.h"

/**
 * MON21.6.2 authoritative per-cell exploration state for one canonical 32x32 map tile.
 *
 * This state lives inside FGridLevelRuntimeState, so it survives runtime level transitions
 * while the dungeon session remains alive. It is intentionally NOT SaveGame-persistent
 * until MON21.6.5 changes the persistence boundary and SaveGame schema.
 *
 * Storage is lazy: an untouched tile allocates nothing; the first successful discovery
 * materializes exactly one byte per canonical cell (1024 bytes).
 */
USTRUCT()
struct FGridMapExplorationState
{
	GENERATED_BODY()

	static constexpr int32 GridSize = 32;
	static constexpr int32 CellCount = GridSize * GridSize;

	static bool IsValidCell(const FIntPoint& Cell)
	{
		return Cell.X >= 0 && Cell.X < GridSize && Cell.Y >= 0 && Cell.Y < GridSize;
	}

	bool IsExplored(const FIntPoint& Cell) const
	{
		if (!IsValidCell(Cell) || ExploredCells.Num() != CellCount)
		{
			return false;
		}

		return ExploredCells[ToCellIndex(Cell)] != 0;
	}

	bool TryMarkExplored(const FIntPoint& Cell, bool& bOutNewlyExplored)
	{
		bOutNewlyExplored = false;
		if (!IsValidCell(Cell))
		{
			return false;
		}

		if (ExploredCells.IsEmpty())
		{
			ExploredCells.Init(0, CellCount);
		}
		else if (ExploredCells.Num() != CellCount)
		{
			return false;
		}

		uint8& CellState = ExploredCells[ToCellIndex(Cell)];
		bOutNewlyExplored = CellState == 0;
		CellState = 1;
		return true;
	}

	int32 GetExploredCellCount() const
	{
		if (ExploredCells.IsEmpty())
		{
			return 0;
		}
		if (ExploredCells.Num() != CellCount)
		{
			return 0;
		}

		int32 Count = 0;
		for (const uint8 CellState : ExploredCells)
		{
			Count += CellState != 0 ? 1 : 0;
		}
		return Count;
	}

	int32 GetStorageCellCount() const
	{
		return ExploredCells.Num();
	}

	bool IsSecretDiscovered(const FGuid& ObjectId) const
	{
		return ObjectId.IsValid() && DiscoveredSecretObjectIds.Contains(ObjectId);
	}

	bool TryMarkSecretDiscovered(const FGuid& ObjectId, bool& bOutNewlyDiscovered)
	{
		bOutNewlyDiscovered = false;
		if (!ObjectId.IsValid())
		{
			return false;
		}

		const int32 PreviousCount = DiscoveredSecretObjectIds.Num();
		DiscoveredSecretObjectIds.Add(ObjectId);
		bOutNewlyDiscovered = DiscoveredSecretObjectIds.Num() != PreviousCount;
		return true;
	}

	int32 GetDiscoveredSecretCount() const
	{
		return DiscoveredSecretObjectIds.Num();
	}
	bool IsStructurallyValid() const
	{
		if (!ExploredCells.IsEmpty())
		{
			if (ExploredCells.Num() != CellCount)
			{
				return false;
			}

			for (const uint8 CellState : ExploredCells)
			{
				if (CellState > 1)
				{
					return false;
				}
			}
		}

		for (const FGuid& ObjectId : DiscoveredSecretObjectIds)
		{
			if (!ObjectId.IsValid())
			{
				return false;
			}
		}
		return true;
	}

	void Reset()
	{
		ExploredCells.Reset();
		DiscoveredSecretObjectIds.Reset();
	}

private:
	static int32 ToCellIndex(const FIntPoint& Cell)
	{
		return Cell.Y * GridSize + Cell.X;
	}

	// MON21.6.2: reflected for normal struct copying, deliberately not marked SaveGame before MON21.6.5.
	UPROPERTY()
	TArray<uint8> ExploredCells;

	// MON21.6.4: session-local knowledge of secret world objects revealed to the player.
	UPROPERTY()
	TSet<FGuid> DiscoveredSecretObjectIds;
};
