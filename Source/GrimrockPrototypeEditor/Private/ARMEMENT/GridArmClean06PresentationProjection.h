#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

// Editor-only one-way projection. Never installs the canonical renderer and
// never serializes a production package or manufactures an ActionId.
namespace GridArmClean06PresentationProjection
{
	bool TryProject(FName ExplicitPresentationId,
		const FGridPlayerAttackPresentationProfile& Source,
		FGridCanonicalActionPresentation& OutPresentation);
}
