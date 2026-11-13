#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"

// Editor-only identity projection. No persistent item, no implicit level,
// catalogue, icon or gameplay property is transferred to the concrete item.
namespace GridArmClean06ItemIdentityProjection
{
	bool TryProject(const UGridItemDefinitionAsset& Source,
		UGridArmamentDefinitionAsset& Destination);
	bool Matches(const UGridItemDefinitionAsset& Source,
		const UGridArmamentDefinitionAsset& Destination);
}
