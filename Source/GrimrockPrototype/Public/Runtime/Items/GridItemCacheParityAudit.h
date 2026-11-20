#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
class UGridItemSubsystem;
struct FGridItemInstance;

// Post-schema Editor evidence checks real runtime readers against the canonical
// projection. Historical before/after cache proof is preserved at 5G92/5G93.
namespace GridItemCacheParityAudit
{
    GRIMROCKPROTOTYPE_API bool HasCanonicalProjection(
        const UGridItemSubsystem& Registry, const FGridItemInstance& Instance);
}
#endif
