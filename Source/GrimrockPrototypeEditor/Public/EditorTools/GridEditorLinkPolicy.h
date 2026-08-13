#pragma once

#include "CoreMinimal.h"

enum class EGridObjectCommand : uint8;
enum class EGridObjectEvent : uint8;
struct FGridLevelObjectData;

/** Shared Grid Editor connector policy used by Slate and automation tests. */
namespace GridEditorLinkPolicy
{
    GRIMROCKPROTOTYPEEDITOR_API bool CanObjectEmitEvents (
        const FGridLevelObjectData& ObjectData);

    GRIMROCKPROTOTYPEEDITOR_API bool CanObjectReceiveCommands (
        const FGridLevelObjectData& ObjectData);

    GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectEvent>
        GetSupportedEventsForSource (
            const FGridLevelObjectData& ObjectData);

    GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectCommand>
        GetSupportedCommandsForTarget (
            const FGridLevelObjectData& ObjectData);

    GRIMROCKPROTOTYPEEDITOR_API TArray<EGridObjectEvent>
        GetEventDisplayOrder ();
}
