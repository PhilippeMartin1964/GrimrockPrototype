#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"

class AGridLevelRuntimeActor;
class UGridMonsterOccupancySubsystem;

/** Pure/shared C5 grid-displacement rules. */
class GRIMROCKPROTOTYPE_API FGridCombatMovementResolver
{
public:
	static EGridEdge ResolveDirection(
		EGridCombatMovementDirection Direction, EGridEdge SubjectFacing, const FIntPoint& SubjectCell, const FIntPoint& SourceCell);

	static bool ResolveDestination(const FGridCombatMovementEffectProfile& Profile, const FIntPoint& FromCell, EGridEdge SubjectFacing,
		const FIntPoint& SourceCell, const AGridLevelRuntimeActor* RuntimeActor, const UGridMonsterOccupancySubsystem* Occupancy,
		FGridCombatMovementResolution& OutResolution);
};
