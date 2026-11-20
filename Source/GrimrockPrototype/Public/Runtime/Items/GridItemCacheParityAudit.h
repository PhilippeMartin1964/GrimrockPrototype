#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
class UGridItemSubsystem;
struct FGridItemInstance;

// Editor evidence invokes the same implementation as the actual runtime
// consumers. Not compiled as a game/Shipping compatibility mechanism.
namespace GridItemCacheParityAudit
{
    GRIMROCKPROTOTYPE_API bool HasSameCanonicalProjection(
        const UGridItemSubsystem& Registry, const FGridItemInstance& Instance);
}
#endif
