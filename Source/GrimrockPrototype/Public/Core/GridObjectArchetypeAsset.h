#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridObjectBehavior.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "GridObjectArchetypeAsset.generated.h"

class AGridRuntimeObjectActor;
class AGridItemActor;

UENUM (BlueprintType)
enum class EGridArchetypeValidationSeverity : uint8
{
    Info    UMETA (DisplayName = "Info"),
    Warning UMETA (DisplayName = "Warning"),
    Error   UMETA (DisplayName = "Error")
};

USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridArchetypeValidationMessage
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
    EGridArchetypeValidationSeverity Severity = EGridArchetypeValidationSeverity::Info;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
    FString Message;

    FGridArchetypeValidationMessage () = default;

    FGridArchetypeValidationMessage (EGridArchetypeValidationSeverity InSeverity, const FString& InMessage)
        : Severity (InSeverity)
        , Message (InMessage)
    {}
};

UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API UGridObjectArchetypeAsset : public UDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Archetype")
    FName ArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Archetype")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Archetype")
    EGridLevelObjectType SupportedType = EGridLevelObjectType::None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Archetype")
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    bool bDefaultInitiallyEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    bool bDefaultInitiallyActive = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    FName DefaultTag = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Defaults",
        meta = (ToolTip = "Default behavior copied to placed object instances. Currently contains trigger/link, teleporter, receptacle and button parameters."))
    FGridObjectBehaviorParams DefaultBehavior;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item")
    TArray<FName> ItemTags;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette",
        meta = (DisplayName = "Palette Category", ToolTip = "Editor palette grouping only. Does not affect gameplay. Examples: Doors, Mechanisms, Wall Decorations, Floor Decorations, Receptacles, Lights, Spawns."))
    FName Category = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Archetype",
        meta = (ToolTip = "Editor/validation functional category. Does not directly drive runtime gameplay. SupportedType remains the gameplay type."))
    EGridObjectCategory ObjectCategory = EGridObjectCategory::Decoration;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    EGridObjectPlacementKind PlacementKind = EGridObjectPlacementKind::Center;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Legacy",
        meta = (AdvancedDisplay,DisplayName = "Legacy Place On Edge", ToolTip = "Legacy compatibility flag only. PlacementKind is now the source of truth. Clear this value on migrated assets."))
    bool bPlaceOnEdge = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Legacy",
        meta = (AdvancedDisplay, DisplayName = "Legacy Place At Cell Center", ToolTip = "Legacy compatibility flag only. PlacementKind is now the source of truth. Clear this value on migrated assets."))
    bool bPlaceAtCellCenter = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement",
        meta = (ToolTip = "Editor placement rule: allows this object to share a cell with other objects."))
    bool bCanShareCell = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement",
        meta = (ToolTip = "Editor placement rule: allows this object to share the same edge/anchor with another object."))
    bool bCanShareAnchor = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement",
        meta = (ToolTip = "Generic movement blocking flag for non-door objects. Door passage blocking is handled by GridDoorSystemComponent."))
    bool bBlocksMovement = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    bool bIsInteractable = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Interaction",
        meta = (ToolTip = "Runtime readable flag. ObjectCategory=Readable is editor classification only."))
    bool bIsReadable = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Interaction",
        meta = (MultiLine = "true", EditCondition = "bIsReadable", EditConditionHides))
    FText ReadableText;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Interaction",
        meta = (EditCondition = "bIsReadable", EditConditionHides))
    bool bShowReadableOnlyOnce = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light",
        meta = (ToolTip = "Runtime light flag. SupportedType=Light is classification; this flag controls whether generic light settings are used."))
    bool bIsLightSource = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light",
        meta = (EditCondition = "bIsLightSource", EditConditionHides))
    FLinearColor LightColor = FLinearColor::White;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light",
        meta = (ClampMin = "0.0", EditCondition = "bIsLightSource", EditConditionHides))
    float LightIntensity = 500.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light",
        meta = (ClampMin = "0.0", EditCondition = "bIsLightSource", EditConditionHides))
    float LightRadius = 500.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light",
        meta = (EditCondition = "bIsLightSource", EditConditionHides))
    bool bUseLightFlicker = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual",
        meta = (DisplayName = "Main Mesh / Preview Mesh",
            ToolTip = "Primary mesh used for simple visible objects and editor preview. For composite/animated objects, use FixedMesh and/or MovingMesh."))
    TObjectPtr<UStaticMesh> PreviewMesh = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual",
        meta = (DisplayName = "Main Material / Preview Material",
            ToolTip = "Primary mesh used for simple visible objects and editor preview. For composite/animated objects, use FixedMesh and/or MovingMesh."))
    TObjectPtr<UMaterialInterface> PreviewMaterial = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
        meta = (ToolTip = "Static fixed part of a composite object, typically secret doors or multi-part mechanisms. Do not use for simple decorations; use Main Mesh / Preview Mesh instead."))
    TObjectPtr<UStaticMesh> FixedMesh = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
        meta = (ToolTip = "Moving or animated part of a composite object, such as doors, buttons, levers, or item visuals used by receptacles/items."))
    TObjectPtr<UStaticMesh> MovingMesh = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
        meta = (ToolTip = "Static fixed part of a composite object, typically secret doors or multi-part mechanisms. Do not use for simple decorations; use Main Mesh / Preview Mesh instead."))
    TObjectPtr<UMaterialInterface> FixedMaterial = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
        meta = (ToolTip = "Moving or animated part of a composite object, such as doors, buttons, levers, or item visuals used by receptacles/items."))
    TObjectPtr<UMaterialInterface> MovingMaterial = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime",
        meta = (ToolTip = "Runtime actor class used to spawn this archetype. SupportedType defines what the object is; RuntimeActorClass defines how it is instantiated."))
    TSubclassOf<AGridRuntimeObjectActor> RuntimeActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime")
    TSubclassOf<AGridItemActor> ItemActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0"))
    float PlacementZOffset = 12.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
        meta = (ClampMin = "0.0", EditCondition = "PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge",
            EditConditionHides, ToolTip = "Used only when PlacementKind is Wall or Edge."))
    float WallInset = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
        meta = (EditCondition = "PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge",
            EditConditionHides, ToolTip = "Used only when PlacementKind is Wall or Edge."))
    float LocalOffsetAlongWall = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
        meta = (EditCondition = "PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge",
            EditConditionHides, ToolTip = "Used only when PlacementKind is Wall or Edge."))
    float LocalOffsetVertical = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Rotation", meta = (ClampMin = "0.0"))
    float RotationStepYaw = 90.f;

    bool IsEdgePlaced () const
    {
        return PlacementKind == EGridObjectPlacementKind::Edge ||
            PlacementKind == EGridObjectPlacementKind::Wall;
    }

    bool IsCenterPlaced () const
    {
        return PlacementKind == EGridObjectPlacementKind::Center ||
            PlacementKind == EGridObjectPlacementKind::Floor ||
            PlacementKind == EGridObjectPlacementKind::Ceiling;
    }

    bool IsWallPlaced () const
    {
        return PlacementKind == EGridObjectPlacementKind::Wall;
    }

    bool IsCeilingPlaced () const
    {
        return PlacementKind == EGridObjectPlacementKind::Ceiling;
    }

    bool IsReadable () const
    {
        return bIsReadable;
    }

    bool IsLightSource () const
    {
        return bIsLightSource;
    }

    bool ValidateArchetype (TArray<FGridArchetypeValidationMessage>& OutMessages) const;
    bool IsValidArchetype () const;
    FString GetValidationSummary () const;
    bool RequiresEdgePlacement () const;
    bool SupportsCenterPlacement () const;
    bool SupportsWallPlacement () const;
    bool RequiresRuntimeActorClass () const;
    bool AllowsInvisibleRuntimeObject () const;
    bool UsesWallPlacementParams () const;
    bool UsesCenterPlacementParams () const;
    bool UsesReadableParams () const;
    bool UsesLightParams () const;
    bool UsesItemParams () const;
    bool UsesItemSpawnParams () const;
    bool UsesReceptacleParams () const;
    bool UsesTeleporterParams () const;
    bool UsesButtonAnimationParams () const;
    bool UsesTriggerParams () const;
    bool UsesMovingMeshParams () const;
    bool UsesFixedMeshParams () const;
    bool UsesRuntimeActorClass () const;
};
