#include "EditorTools/GridEditorLinkPolicy.h"

#include "Core/GridTypes.h"

namespace GridEditorLinkPolicy
{
    TArray<EGridObjectEvent> GetSupportedEventsForSource (
        const FGridLevelObjectData& ObjectData)
    {
        switch (ObjectData.Type)
        {
            case EGridLevelObjectType::Button:
                return {EGridObjectEvent::Activated};

            case EGridLevelObjectType::Lever:
            case EGridLevelObjectType::PressurePlate:
            case EGridLevelObjectType::Trigger:
                return {
                    EGridObjectEvent::Activated,
                    EGridObjectEvent::Deactivated
                };

            case EGridLevelObjectType::Receptacle:
                return {
                    EGridObjectEvent::ItemInserted,
                    EGridObjectEvent::ItemRemoved,
                    EGridObjectEvent::ItemChanged
                };

            case EGridLevelObjectType::MonsterSpawn:
                return {
                    EGridObjectEvent::MonsterDied,
                    EGridObjectEvent::MonsterSpawned,
                    EGridObjectEvent::MonsterDespawned,
                    EGridObjectEvent::MonsterTeleported
                };

            default:
                return {};
        }
    }

    TArray<EGridObjectCommand> GetSupportedCommandsForTarget (
        const FGridLevelObjectData& ObjectData)
    {
        switch (ObjectData.Type)
        {
            case EGridLevelObjectType::Door:
                return {
                    EGridObjectCommand::Open,
                    EGridObjectCommand::Close,
                    EGridObjectCommand::Toggle,
                    EGridObjectCommand::Activate,
                    EGridObjectCommand::Deactivate
                };

            case EGridLevelObjectType::Teleporter:
            case EGridLevelObjectType::Light:
                return {
                    EGridObjectCommand::Activate,
                    EGridObjectCommand::Deactivate,
                    EGridObjectCommand::Toggle
                };

            case EGridLevelObjectType::Receptacle:
                return {
                    EGridObjectCommand::ReceptacleConsumeItem,
                    EGridObjectCommand::ReceptacleConsumeAllItems,
                    EGridObjectCommand::ReceptacleEnableRemoval,
                    EGridObjectCommand::ReceptacleDisableRemoval
                };

            case EGridLevelObjectType::MonsterSpawn:
                return {
                    EGridObjectCommand::Spawn,
                    EGridObjectCommand::Despawn,
                    EGridObjectCommand::Teleport,
                    EGridObjectCommand::Activate,
                    EGridObjectCommand::Deactivate,
                    EGridObjectCommand::Enable,
                    EGridObjectCommand::Disable,
                    EGridObjectCommand::Toggle
                };

            default:
                return {};
        }
    }

    bool CanObjectEmitEvents (
        const FGridLevelObjectData& ObjectData)
    {
        return !GetSupportedEventsForSource (ObjectData).IsEmpty ();
    }

    bool CanObjectReceiveCommands (
        const FGridLevelObjectData& ObjectData)
    {
        return !GetSupportedCommandsForTarget (ObjectData).IsEmpty ();
    }

    TArray<EGridObjectEvent> GetEventDisplayOrder ()
    {
        return {
            EGridObjectEvent::Activated,
            EGridObjectEvent::Deactivated,
            EGridObjectEvent::ItemInserted,
            EGridObjectEvent::ItemRemoved,
            EGridObjectEvent::ItemChanged,
            EGridObjectEvent::Used,
            EGridObjectEvent::Entered,
            EGridObjectEvent::Exited,
            EGridObjectEvent::Opened,
            EGridObjectEvent::Closed,
            EGridObjectEvent::Enabled,
            EGridObjectEvent::Disabled,
            EGridObjectEvent::MonsterDied,
            EGridObjectEvent::MonsterSpawned,
            EGridObjectEvent::MonsterDespawned,
            EGridObjectEvent::MonsterTeleported
        };
    }
}
