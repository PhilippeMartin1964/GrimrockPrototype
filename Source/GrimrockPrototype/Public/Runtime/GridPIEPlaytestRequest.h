#pragma once

#include "CoreMinimal.h"

class AGridLevelRuntimeActor;
class UWorld;

/**
 * Process-local editor PIE request. It is never serialized into a map,
 * Blueprint or save game and only matches duplicated EWorldType::PIE actors.
 */
namespace GridPIEPlaytestRequest
{
    GRIMROCKPROTOTYPE_API void BeginFreshPlaytest (
        const AGridLevelRuntimeActor* PreparedEditorActor);

    GRIMROCKPROTOTYPE_API void Clear (const TCHAR* Reason);

    GRIMROCKPROTOTYPE_API bool IsActiveForWorld (const UWorld* World);

    GRIMROCKPROTOTYPE_API bool Matches (
        const AGridLevelRuntimeActor* RuntimeActor);

    GRIMROCKPROTOTYPE_API AGridLevelRuntimeActor* ResolveMatchingRuntimeActor (
        UWorld* World);
}
