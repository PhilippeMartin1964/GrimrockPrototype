#pragma once

#include "CoreMinimal.h"

class AGridLevelRuntimeActor;
struct FGridLooseItemInstance;
struct FGridWorldObjectInstance;

/**
 * WORLDOBJ-MIG09 typed placement transform authority shared by runtime and editor preview.
 * It consumes the persistent typed placement directly and never reconstructs FGridLevelObjectData.
 */
namespace GridPlacementTransformResolver
{
	GRIMROCKPROTOTYPE_API bool ResolveWorldObject(
		const AGridLevelRuntimeActor& RuntimeActor, const FGridWorldObjectInstance& Instance, FTransform& OutTransform);

	GRIMROCKPROTOTYPE_API bool ResolveLooseItem(
		const AGridLevelRuntimeActor& RuntimeActor, const FGridLooseItemInstance& Instance, FTransform& OutTransform);
}
