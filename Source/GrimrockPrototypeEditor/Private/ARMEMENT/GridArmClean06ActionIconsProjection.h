#pragma once

#include "CoreMinimal.h"
#include "UI/GridEquippedWeaponActionIconPalette.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

// Editor-only migration preview for the one existing UE palette.
// The icon map does not produce an action or an attack permission.
namespace GridArmClean06ActionIconsProjection
{
	bool TryProject(const UGridEquippedWeaponActionIconPalette& Source,
		UGridCanonicalWeaponVisualCatalogueAsset& Destination);
	bool IsIdentical(const UGridEquippedWeaponActionIconPalette& Source,
		const UGridCanonicalWeaponVisualCatalogueAsset& Destination);
}
