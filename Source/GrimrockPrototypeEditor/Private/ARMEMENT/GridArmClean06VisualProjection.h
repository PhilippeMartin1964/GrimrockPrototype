#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

// Editor-only: preserves all visual profile fields and intentional null meshes.
// Does not load resources or author a new binary DataAsset.
namespace GridArmClean06VisualProjection
{
	bool TryProject(const FGridWeaponVisualProfile& Source, FGridCanonicalWeaponVisual& Out);
	bool EqualsSource(const FGridWeaponVisualProfile& Source,
		const FGridCanonicalWeaponVisual& Projection);
}
