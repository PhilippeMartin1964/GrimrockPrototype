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

    bool IsCenterFloorOrCeilingPlacement (EGridObjectPlacementKind PlacementKind)
    {
        return PlacementKind == EGridObjectPlacementKind::Center ||
            PlacementKind == EGridObjectPlacementKind::Floor ||
            PlacementKind == EGridObjectPlacementKind::Ceiling;
    }

    bool IsWallOrEdgePlacement (EGridObjectPlacementKind PlacementKind)
    {
        return PlacementKind == EGridObjectPlacementKind::Wall ||
            PlacementKind == EGridObjectPlacementKind::Edge;
    }

    const TCHAR* ToSupportedTypeText (EGridLevelObjectType SupportedType)
    {
        switch (SupportedType)
        {
            case EGridLevelObjectType::Door:
                return TEXT ("Door");

            case EGridLevelObjectType::Button:
                return TEXT ("Button");

            case EGridLevelObjectType::PressurePlate:
                return TEXT ("PressurePlate");

            case EGridLevelObjectType::Lever:
                return TEXT ("Lever");

            case EGridLevelObjectType::Decoration:
                return TEXT ("Decoration");

            case EGridLevelObjectType::MonsterSpawn:
                return TEXT ("MonsterSpawn");

            case EGridLevelObjectType::ItemSpawn:
                return TEXT ("ItemSpawn");

            case EGridLevelObjectType::Light:
                return TEXT ("Light");

            case EGridLevelObjectType::Teleporter:
                return TEXT ("Teleporter");

            case EGridLevelObjectType::Trigger:
                return TEXT ("Trigger");

            case EGridLevelObjectType::Receptacle:
                return TEXT ("Receptacle");

            case EGridLevelObjectType::None:
            default:
                return TEXT ("None");
        }
    }

    const TCHAR* ToObjectCategoryText (EGridObjectCategory Category)
    {
        switch (Category)
        {
            case EGridObjectCategory::Mechanism:
                return TEXT ("Mechanism");

            case EGridObjectCategory::Decoration:
                return TEXT ("Decoration");

            case EGridObjectCategory::Prop:
                return TEXT ("Prop");

            case EGridObjectCategory::Receptacle:
                return TEXT ("Receptacle");

            case EGridObjectCategory::Light:
                return TEXT ("Light");

            case EGridObjectCategory::Readable:
                return TEXT ("Readable");

            case EGridObjectCategory::Spawn:
                return TEXT ("Spawn");

            case EGridObjectCategory::Teleporter:
                return TEXT ("Teleporter");

            default:
                return TEXT ("Unknown");
        }
    }

    EGridObjectCategory GetRecommendedObjectCategory (EGridLevelObjectType SupportedType, bool bIsReadable)
    {
        switch (SupportedType)
        {
            case EGridLevelObjectType::Door:
            case EGridLevelObjectType::Button:
            case EGridLevelObjectType::PressurePlate:
            case EGridLevelObjectType::Lever:
            case EGridLevelObjectType::Trigger:
                return EGridObjectCategory::Mechanism;

            case EGridLevelObjectType::Receptacle:
                return EGridObjectCategory::Receptacle;

            case EGridLevelObjectType::Decoration:
                return bIsReadable ? EGridObjectCategory::Readable : EGridObjectCategory::Decoration;

            case EGridLevelObjectType::Light:
                return EGridObjectCategory::Light;

            case EGridLevelObjectType::Teleporter:
                return EGridObjectCategory::Teleporter;

            case EGridLevelObjectType::MonsterSpawn:
            case EGridLevelObjectType::ItemSpawn:
                return EGridObjectCategory::Spawn;

            case EGridLevelObjectType::None:
            default:
                return EGridObjectCategory::Decoration;
        }
    }

    bool IsObjectCategoryCompatible (EGridLevelObjectType SupportedType, EGridObjectCategory ObjectCategory, bool bIsReadable)
    {
        if (SupportedType == EGridLevelObjectType::None)
        {
            return true;
        }

        if (SupportedType == EGridLevelObjectType::Decoration)
        {
            if (bIsReadable)
            {
                return ObjectCategory == EGridObjectCategory::Readable;
            }

            return ObjectCategory == EGridObjectCategory::Decoration ||
                ObjectCategory == EGridObjectCategory::Readable ||
                ObjectCategory == EGridObjectCategory::Prop;
        }

        return ObjectCategory == GetRecommendedObjectCategory (SupportedType, bIsReadable);
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

    if (bPlaceOnEdge && !IsWallOrEdgePlacement (PlacementKind))
    {
        AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Legacy bPlaceOnEdge=true but PlacementKind is not Wall or Edge. PlacementKind is now the source of truth."));
    }

    if (bPlaceAtCellCenter && !IsCenterFloorOrCeilingPlacement (PlacementKind))
    {
        AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Legacy bPlaceAtCellCenter=true but PlacementKind is not Center, Floor, or Ceiling. PlacementKind is now the source of truth."));
    }

    if (!IsObjectCategoryCompatible (SupportedType, ObjectCategory, bIsReadable))
    {
        if (SupportedType == EGridLevelObjectType::Decoration && bIsReadable)
        {
            AddValidationMessage (OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT ("Readable Decoration should generally use ObjectCategory=Readable."));
        }
        else
        {
            const EGridObjectCategory RecommendedCategory = GetRecommendedObjectCategory (SupportedType, bIsReadable);
            OutMessages.Emplace (
                EGridArchetypeValidationSeverity::Warning,
                FString::Printf (
                    TEXT ("%s should generally use ObjectCategory=%s, but currently uses %s."),
                    ToSupportedTypeText (SupportedType),
                    ToObjectCategoryText (RecommendedCategory),
                    ToObjectCategoryText (ObjectCategory)));
        }
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
