#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

// Editor-only migration candidate for advisory class metadata.
// No equip restrictions, gameplay policy or source spreadsheet inference.
namespace GridArmClean06ClassAffinityProjection
{
	bool TryProject(const UGridWeaponCatalogueAsset& Source,
		UGridCanonicalWeaponCatalogueAsset& Destination);
	bool EqualsSource(const UGridWeaponCatalogueAsset& Source,
		const UGridCanonicalWeaponCatalogueAsset& Destination);
}
