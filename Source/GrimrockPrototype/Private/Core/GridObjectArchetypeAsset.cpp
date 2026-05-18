#include "Core/GridObjectArchetypeAsset.h"

namespace
{
    const TCHAR* ToValidationSeverityText (EGridArchetypeValidationSeverity Severity)
    {
        switch (Severity)
        {
            case EGridArchetypeValidationSeverity::Info:
                return TEXT ("Info");

            case EGridArchetypeValidationSeverity::Warning:
                return TEXT ("Warning");

            case EGridArchetypeValidationSeverity::Error:
                return TEXT ("Error");

            default:
                return TEXT ("Unknown");
        }
    }

    void AddValidationMessage (
        TArray<FGridArchetypeValidationMessage>& Messages,
        EGridArchetypeValidationSeverity Severity,
        const TCHAR* Message)
    {
        Messages.Emplace (Severity, FString (Message));
    }

    bool HasAnyMesh (const UGridObjectArchetypeAsset& Archetype)
    {
        return Archetype.PreviewMesh || Archetype.FixedMesh || Archetype.MovingMesh;
    }

    bool HasPreviewOrMovingMesh (const UGridObjectArchetypeAsset& Archetype)
    {
        return Archetype.PreviewMesh || Archetype.MovingMesh;
    }

    bool IsFloorOrCenterPlacement (EGridObjectPlacementKind PlacementKind)
    {
        return PlacementKind == EGridObjectPlacementKind::Floor ||
            PlacementKind == EGridObjectPlacementKind::Center;
    }

    bool IsWallOrEdgePlacement (EGridObjectPlacementKind PlacementKind)
    {
        return PlacementKind == EGridObjectPlacementKind::Wall ||
            PlacementKind == EGridObjectPlacementKind::Edge;
    }
}

bool UGridObjectArchetypeAsset::ValidateArchetype (TArray<FGridArchetypeValidationMessage>& OutMessages) const
{
    OutMessages.Reset ();

    if (ArchetypeId.IsNone ())
    {
        AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("ArchetypeId is not set."));
    }

    if (SupportedType == EGridLevelObjectType::None)
    {
        AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("SupportedType must not be None."));
    }

    if (RequiresRuntimeActorClass () && !RuntimeActorClass)
    {
        AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("RuntimeActorClass is required for this SupportedType."));
    }

    switch (SupportedType)
    {
        case EGridLevelObjectType::Door:
        {
            if (!IsWallOrEdgePlacement (PlacementKind))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("Door PlacementKind must be Edge or Wall."));
            }
            if (!HasPreviewOrMovingMesh (*this))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("Door requires PreviewMesh or MovingMesh."));
            }
            if (bCanShareAnchor)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Door should generally have bCanShareAnchor set to false."));
            }
            if (!bBlocksMovement)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Info, TEXT ("Door has bBlocksMovement=false; this is acceptable when blocking is handled by GridDoorSystemComponent."));
            }
            break;
        }

        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
        {
            if (!IsWallOrEdgePlacement (PlacementKind))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("Button and Lever PlacementKind must be Wall or Edge."));
            }
            if (!bIsInteractable)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Button and Lever should generally be interactable."));
            }
            if (!HasPreviewOrMovingMesh (*this))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Button and Lever should generally define PreviewMesh or MovingMesh."));
            }
            break;
        }

        case EGridLevelObjectType::PressurePlate:
        {
            if (!IsFloorOrCenterPlacement (PlacementKind))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("PressurePlate PlacementKind must be Floor or Center."));
            }
            if (bIsInteractable)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Info, TEXT ("PressurePlate is marked interactable, which is usually unnecessary."));
            }
            if (!HasPreviewOrMovingMesh (*this))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("PressurePlate should generally define PreviewMesh or MovingMesh."));
            }
            break;
        }

        case EGridLevelObjectType::Trigger:
        {
            if (!IsFloorOrCenterPlacement (PlacementKind))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("Trigger PlacementKind must be Floor or Center."));
            }
            break;
        }

        case EGridLevelObjectType::Decoration:
        {
            if (!HasAnyMesh (*this))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Decoration should generally define a mesh."));
            }
            if (bIsReadable && !bIsInteractable)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Readable decoration should generally also be interactable."));
            }
            if (bIsReadable && ReadableText.IsEmpty ())
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Info, TEXT ("ReadableText is empty; this is acceptable when OverrideReadableText is defined on level object data."));
            }
            break;
        }

        case EGridLevelObjectType::Light:
        {
            if (!bIsLightSource)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Light should generally have bIsLightSource set to true."));
            }
            if (bIsLightSource && LightIntensity <= 0.f)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("LightIntensity must be greater than 0 when bIsLightSource is true."));
            }
            if (bIsLightSource && LightRadius <= 0.f)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("LightRadius must be greater than 0 when bIsLightSource is true."));
            }
            break;
        }

        case EGridLevelObjectType::Teleporter:
        {
            if (!IsFloorOrCenterPlacement (PlacementKind))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("Teleporter PlacementKind must be Floor or Center."));
            }
            if (DefaultBehavior.TargetCellX == INDEX_NONE || DefaultBehavior.TargetCellY == INDEX_NONE)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Teleporter should define DefaultBehavior TargetCellX and TargetCellY."));
            }
            break;
        }

        case EGridLevelObjectType::Receptacle:
        {
            if (!bIsInteractable)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Receptacle should generally be interactable."));
            }
            if (!DefaultBehavior.bAcceptAnyItem &&
                DefaultBehavior.AcceptedItemTags.Num () == 0 &&
                DefaultBehavior.AcceptedArchetypeIds.Num () == 0)
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("Receptacle with bAcceptAnyItem=false must define AcceptedItemTags or AcceptedArchetypeIds."));
            }
            break;
        }

        case EGridLevelObjectType::MonsterSpawn:
        case EGridLevelObjectType::ItemSpawn:
        {
            if (!IsFloorOrCenterPlacement (PlacementKind))
            {
                AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Error, TEXT ("MonsterSpawn and ItemSpawn PlacementKind must be Floor or Center."));
            }
            break;
        }

        case EGridLevelObjectType::None:
        default:
            break;
    }

    for (const FGridArchetypeValidationMessage& Message : OutMessages)
    {
        if (Message.Severity == EGridArchetypeValidationSeverity::Error)
        {
            return false;
        }
    }
    return true;
}

bool UGridObjectArchetypeAsset::IsValidArchetype () const
{
    TArray<FGridArchetypeValidationMessage> Messages;
    return ValidateArchetype (Messages);
}

FString UGridObjectArchetypeAsset::GetValidationSummary () const
{
    TArray<FGridArchetypeValidationMessage> Messages;
    ValidateArchetype (Messages);

    int32 ErrorCount = 0;
    int32 WarningCount = 0;
    int32 InfoCount = 0;

    for (const FGridArchetypeValidationMessage& Message : Messages)
    {
        switch (Message.Severity)
        {
            case EGridArchetypeValidationSeverity::Error:
                ++ErrorCount;
                break;

            case EGridArchetypeValidationSeverity::Warning:
                ++WarningCount;
                break;

            case EGridArchetypeValidationSeverity::Info:
                ++InfoCount;
                break;
        }
    }

    const FString ArchetypeName = ArchetypeId.IsNone () ? GetName () : ArchetypeId.ToString ();
    FString Summary = FString::Printf (
        TEXT ("Grid archetype validation: %s | Errors=%d Warnings=%d Info=%d"),
        *ArchetypeName,
        ErrorCount,
        WarningCount,
        InfoCount);

    for (const FGridArchetypeValidationMessage& Message : Messages)
    {
        Summary += FString::Printf (
            TEXT ("\n- [%s] %s"),
            ToValidationSeverityText (Message.Severity),
            *Message.Message);
    }

    return Summary;
}

bool UGridObjectArchetypeAsset::RequiresEdgePlacement () const
{
    switch (SupportedType)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
            return true;

        default:
            return false;
    }
}

bool UGridObjectArchetypeAsset::SupportsCenterPlacement () const
{
    switch (SupportedType)
    {
        case EGridLevelObjectType::PressurePlate:
        case EGridLevelObjectType::Trigger:
        case EGridLevelObjectType::Decoration:
        case EGridLevelObjectType::Light:
        case EGridLevelObjectType::Teleporter:
        case EGridLevelObjectType::MonsterSpawn:
        case EGridLevelObjectType::ItemSpawn:
            return true;

        default:
            return false;
    }
}

bool UGridObjectArchetypeAsset::SupportsWallPlacement () const
{
    switch (SupportedType)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
        case EGridLevelObjectType::Decoration:
        case EGridLevelObjectType::Light:
        case EGridLevelObjectType::Receptacle:
            return true;

        default:
            return false;
    }
}

bool UGridObjectArchetypeAsset::RequiresRuntimeActorClass () const
{
    switch (SupportedType)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
        case EGridLevelObjectType::PressurePlate:
        case EGridLevelObjectType::Teleporter:
        case EGridLevelObjectType::Receptacle:
            return true;

        default:
            return false;
    }
}

bool UGridObjectArchetypeAsset::AllowsInvisibleRuntimeObject () const
{
    switch (SupportedType)
    {
        case EGridLevelObjectType::Trigger:
        case EGridLevelObjectType::MonsterSpawn:
        case EGridLevelObjectType::ItemSpawn:
            return true;

        default:
            return false;
    }
}
