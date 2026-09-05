#pragma once

#include "CoreMinimal.h"
#include "GridObjectBehavior.h"
#include "GridBoundary.generated.h"

/**
 * Canonical key for a physical boundary shared by two adjacent grid cells.
 * North/South descriptions normalize to North; East/West descriptions normalize to East.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridBoundaryKey
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Boundary")
	int32 CellX = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Boundary")
	int32 CellY = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Boundary")
	EGridEdge Side = EGridEdge::None;

	static FGridBoundaryKey MakeCanonical(int32 X, int32 Y, EGridEdge Edge)
	{
		FGridBoundaryKey Key;
		switch (Edge)
		{
			case EGridEdge::North:
				Key.CellX = X;
				Key.CellY = Y;
				Key.Side = EGridEdge::North;
				break;

			case EGridEdge::South:
				Key.CellX = X;
				Key.CellY = Y - 1;
				Key.Side = EGridEdge::North;
				break;

			case EGridEdge::East:
				Key.CellX = X;
				Key.CellY = Y;
				Key.Side = EGridEdge::East;
				break;

			case EGridEdge::West:
				Key.CellX = X - 1;
				Key.CellY = Y;
				Key.Side = EGridEdge::East;
				break;

			case EGridEdge::None:
			default:
				Key.CellX = X;
				Key.CellY = Y;
				Key.Side = EGridEdge::None;
				break;
		}
		return Key;
	}

	bool IsValid() const
	{
		return Side == EGridEdge::North || Side == EGridEdge::East;
	}

	bool operator==(const FGridBoundaryKey& Other) const
	{
		return CellX == Other.CellX && CellY == Other.CellY && Side == Other.Side;
	}
};

FORCEINLINE uint32 GetTypeHash(const FGridBoundaryKey& Key)
{
	uint32 Hash = HashCombine(::GetTypeHash(Key.CellX), ::GetTypeHash(Key.CellY));
	return HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Key.Side)));
}
