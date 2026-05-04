#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridObjectBehavior.h"
#include "GridObjectArchetypeAsset.generated.h"

class AGridRuntimeObjectActor;

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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FName Category = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    bool bPlaceOnEdge = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement")
    bool bPlaceAtCellCenter = true;

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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0"))
    float PlacementZOffset = 12.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0"))
    float WallInset = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (EditCondition = "bPlaceOnEdge"))
    float LocalOffsetAlongWall = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (EditCondition = "bPlaceOnEdge"))
    float LocalOffsetVertical = 0.f;
};