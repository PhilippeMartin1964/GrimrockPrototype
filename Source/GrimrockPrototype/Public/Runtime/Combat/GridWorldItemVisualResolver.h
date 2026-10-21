#pragma once

#include "CoreMinimal.h"

class UGridItemDefinitionAsset;
class UStaticMesh;
struct FGridItemInstance;

/** One shared, presentation-only mesh choice for runtime pickups and editor previews. */
struct GRIMROCKPROTOTYPE_API FGridResolvedWorldItemMesh
{
	UStaticMesh* Mesh = nullptr;
	FTransform RelativeTransform = FTransform::Identity;
	FName VisualId = NAME_None;
};

/**
 * ARMEMENT01.6.5. Load only at item materialization / editor preview rebuild.
 * Gameplay physics, item identity and material ownership remain unchanged.
 */
class GRIMROCKPROTOTYPE_API FGridWorldItemVisualResolver final
{
public:
	static FGridResolvedWorldItemMesh Resolve(
		const UGridItemDefinitionAsset& Definition, const FGridItemInstance* Instance = nullptr);
};
