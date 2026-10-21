#include "Runtime/Combat/GridWorldItemVisualResolver.h"

#include "Runtime/Combat/GridWeaponVisualResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Engine/StaticMesh.h"

FGridResolvedWorldItemMesh FGridWorldItemVisualResolver::Resolve(
	const UGridItemDefinitionAsset& Definition, const FGridItemInstance* Instance)
{
	FGridResolvedWorldItemMesh Result;

	// The core visual resolver is intentionally non-loading. At the boundary
	// of materialization, load the optional catalogue once, never on Tick.
	if (Definition.bUseWeaponVisualCatalogue && !Definition.WeaponVisualCatalogue.IsNull() &&
		!Definition.WeaponVisualCatalogue.IsValid())
	{
		Definition.WeaponVisualCatalogue.LoadSynchronous();
	}

	FGridResolvedWeaponVisual Visual;
	EGridWeaponVisualResolutionError Error = EGridWeaponVisualResolutionError::None;
	FGridWeaponVisualResolver::TryResolve(Definition, Instance, Visual, Error);

	if (!Visual.WorldMesh.IsNull())
	{
		Result.Mesh = Visual.WorldMesh.LoadSynchronous();
		if (Result.Mesh)
		{
			// Only a fully valid catalogue world mesh owns its authored local
			// transform. Never apply it to a Legacy fallback mesh.
			if (!Visual.VisualId.IsNone() && !Visual.bLegacyFallback &&
				Error == EGridWeaponVisualResolutionError::None)
			{
				Result.RelativeTransform = Visual.WorldVisualTransform;
				Result.VisualId = Visual.VisualId;
			}
			return Result;
		}
	}

	// A broken/unloaded catalogue mesh cannot hide an otherwise valid Legacy
	// collectible; no unrelated archetype is substituted.
	if (!Definition.WorldMesh.IsNull())
	{
		Result.Mesh = Definition.WorldMesh.LoadSynchronous();
	}
	return Result;
}
