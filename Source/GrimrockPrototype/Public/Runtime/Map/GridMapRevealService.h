#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"

class UGridDoorSystemComponent;
class UGridLevelAsset;
struct FGridMapExplorationState;

/** MON21.6.3 deterministic topology-aware fog-of-war reveal for one canonical map tile. */
class GRIMROCKPROTOTYPE_API FGridMapRevealService
{
public:
	static constexpr float RevealRadiusCells = 1.25f;

	/** Marks the origin and visible cardinal neighbours. Returns the number of newly explored cells. */
	static int32 RevealAroundCell(const UGridLevelAsset& LevelAsset, const UGridDoorSystemComponent* DoorSystem, const FIntPoint& Origin,
		FGridMapExplorationState& ExplorationState);

	/**
	 * Visibility topology is independent from occupancy/pathfinding.
	 * Structural walls on either side of the boundary and blocked doors stop reveal.
	 */
	static bool CanRevealAcrossEdge(
		const UGridLevelAsset& LevelAsset, const UGridDoorSystemComponent* DoorSystem, const FIntPoint& FromCell, EGridEdge Direction);
};
