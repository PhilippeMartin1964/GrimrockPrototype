#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

/** Editor-only exact copy of legacy flight visuals into the shared visual catalogue. */
namespace GridArmClean06ProjectileVisualProjection
{
	bool TryProject(const FGridWeaponPipelineRow& Row,
		const UGridItemDefinitionAsset& Source,
		FGridCanonicalWeaponVisual& Destination);
	bool Matches(const UGridItemDefinitionAsset& Source,
		const FGridCanonicalWeaponVisual& Destination);
}
