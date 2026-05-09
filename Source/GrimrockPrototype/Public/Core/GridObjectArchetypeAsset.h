#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridObjectBehavior.h"
#include "GridObjectArchetypeAsset.generated.h"

class AGridRuntimeObjectActor;
class AGridItemActor;

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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FName Category = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Archetype")
    EGridObjectCategory ObjectCategory = EGridObjectCategory::Decoration;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    EGridObjectPlacementKind PlacementKind = EGridObjectPlacementKind::Center;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    bool bPlaceOnEdge = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0"))
    float WallInset = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (EditCondition = "bPlaceOnEdge"))
    float LocalOffsetAlongWall = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (EditCondition = "bPlaceOnEdge"))
    float LocalOffsetVertical = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement|Rotation", meta = (ClampMin = "0.0"))
    float RotationStepYaw = 90.f;

    bool IsEdgePlaced () const
    {
        return PlacementKind == EGridObjectPlacementKind::Edge ||
            PlacementKind == EGridObjectPlacementKind::Wall ||
            bPlaceOnEdge;
    }

    bool IsCenterPlaced () const
    {
        return PlacementKind == EGridObjectPlacementKind::Center ||
            PlacementKind == EGridObjectPlacementKind::Floor ||
            PlacementKind == EGridObjectPlacementKind::Ceiling ||
            bPlaceAtCellCenter;
    }

    bool IsWallPlaced () const
    {
        return PlacementKind == EGridObjectPlacementKind::Wall ||
            bPlaceOnEdge;
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
};
