#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

/** Editor-only exact physical combat-throw conversion of manifest ARM. */
namespace GridArmClean06ProjectilePolicyProjection
{
	bool TryProject(const FGridWeaponPipelineRow& Row,
		const UGridItemDefinitionAsset& Source,
		FGridCanonicalWeaponArchetype& Destination);
	bool Matches(const UGridItemDefinitionAsset& Source,
		const FGridCanonicalProjectilePolicy& Destination);
}
