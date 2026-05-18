#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridObjectBehavior.h"
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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    FGridObjectBehaviorParams DefaultBehavior;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item")
    TArray<FName> ItemTags;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette",
        meta = (DisplayName = "Palette Category", ToolTip = "Editor palette grouping only. Does not affect gameplay. Examples: Doors, Mechanisms, Wall Decorations, Floor Decorations, Receptacles, Lights, Spawns."))
    FName Category = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Archetype")
    EGridObjectCategory ObjectCategory = EGridObjectCategory::Decoration;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    EGridObjectPlacementKind PlacementKind = EGridObjectPlacementKind::Center;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Legacy",
        meta = (DisplayName = "Legacy Place On Edge", ToolTip = "Legacy compatibility flag only. PlacementKind is now the source of truth for edge and wall placement."))
    bool bPlaceOnEdge = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Legacy",
        meta = (DisplayName = "Legacy Place At Cell Center", ToolTip = "Legacy compatibility flag only. PlacementKind is now the source of truth for center, floor, and ceiling placement."))
    bool bPlaceAtCellCenter = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    bool bCanShareCell = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    bool bCanShareAnchor = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    bool bBlocksMovement = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    bool bIsInteractable = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    bool bIsReadable = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (MultiLine = "true"))
    FText ReadableText;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    bool bShowReadableOnlyOnce = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light")
    bool bIsLightSource = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light")
    FLinearColor LightColor = FLinearColor::White;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light", meta = (ClampMin = "0.0"))
    float LightIntensity = 500.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light", meta = (ClampMin = "0.0"))
    float LightRadius = 500.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Light")
    bool bUseLightFlicker = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMesh> PreviewMesh = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UMaterialInterface> PreviewMaterial = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMesh> FixedMesh = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMesh> MovingMesh = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UMaterialInterface> FixedMaterial = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UMaterialInterface> MovingMaterial = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime")
    TSubclassOf<AGridRuntimeObjectActor> RuntimeActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime")
    TSubclassOf<AGridItemActor> ItemActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0"))
    float PlacementZOffset = 12.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
        meta = (ClampMin = "0.0", ToolTip = "Used only when PlacementKind is Wall or Edge."))
    float WallInset = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
        meta = (ToolTip = "Used only when PlacementKind is Wall or Edge."))
    float LocalOffsetAlongWall = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
        meta = (ToolTip = "Used only when PlacementKind is Wall or Edge."))
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
};
