#pragma once

#include "CoreMinimal.h"
#include "GridObjectBehavior.generated.h"

class UGridItemDefinitionAsset;

UENUM (BlueprintType)
enum class EGridEdge : uint8
{
    None    UMETA (DisplayName = "None"),
    North   UMETA (DisplayName = "North"),
    East    UMETA (DisplayName = "East"),
    South   UMETA (DisplayName = "South"),
    West    UMETA (DisplayName = "West")
};

USTRUCT (BlueprintType)
struct FGridItemBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    TObjectPtr<UGridItemDefinitionAsset> ItemDefinitionAsset = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ItemDefinitionId = NAME_None;
};

USTRUCT (BlueprintType)
struct FGridTeleporterBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Teleporter",
        meta = (ToolTip = "Target cell X used only by SupportedType=Teleporter."))
    int32 TargetCellX = INDEX_NONE;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Teleporter",
        meta = (ToolTip = "Target cell Y used only by SupportedType=Teleporter."))
    int32 TargetCellY = INDEX_NONE;
};

USTRUCT (BlueprintType)
struct FGridObjectTransitionParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Transition")
    bool bIsTransition = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Transition",
        meta = (EditCondition = "bIsTransition"))
    FName TargetLevelId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Transition",
        meta = (EditCondition = "bIsTransition"))
    int32 TargetCellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Transition",
        meta = (EditCondition = "bIsTransition"))
    int32 TargetCellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Transition",
        meta = (EditCondition = "bIsTransition"))
    EGridEdge TargetFacing = EGridEdge::North;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Transition",
        meta = (EditCondition = "bIsTransition"))
    bool bRequireUseAction = false;
};

USTRUCT (BlueprintType)
struct FGridReceptacleBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bAcceptAnyItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (EditCondition = "!bAcceptAnyItem", EditConditionHides))
    TArray<FName> AcceptedItemTags;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (EditCondition = "!bAcceptAnyItem", EditConditionHides))
    TArray<FName> AcceptedArchetypeIds;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    TArray<FName> RejectedItemArchetypeIds;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    FName InitialContainedItemArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Item")
    TObjectPtr<UGridItemDefinitionAsset> InitialContainedItemDefinition = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Item")
    FName InitialContainedItemDefinitionId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Max Contained Items", ClampMin = "1"))
    int32 MaxContainedItems = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Physical Placement",
            ToolTip = "Place inserted items at the clicked surface point with gravity and physics instead of attaching them to the fixed socket."))
    bool bUsePhysicalPlacement = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Extinguish Item On Physical Placement",
            EditCondition = "bUsePhysicalPlacement", EditConditionHides,
            ToolTip = "Disable an item's runtime light when it is inserted using physical placement."))
    bool bExtinguishItemOnPhysicalPlacement = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Physical Placement Surface Offset",
            ClampMin = "0.0", EditCondition = "bUsePhysicalPlacement", EditConditionHides,
            ToolTip = "Distance in centimeters added along the clicked surface normal before physics starts."))
    float PhysicalPlacementSurfaceOffset = 10.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Physical Placement Initial Rotation Offset",
            EditCondition = "bUsePhysicalPlacement", EditConditionHides,
            ToolTip = "Rotation offset applied relative to the receptacle attach-point orientation before physics starts."))
    FRotator PhysicalPlacementInitialRotationOffset = FRotator::ZeroRotator;
};

USTRUCT (BlueprintType)
struct FGridButtonAnimationParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    float ButtonPressDistance = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    float ButtonPressDuration = 0.08f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    float ButtonReleaseDuration = 0.10f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    float ButtonHoldTime = 0.15f;
};

USTRUCT (BlueprintType)
struct FGridLeverAnimationParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lever")
    float LeverOffPitch = 45.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lever")
    float LeverOnPitch = 135.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lever")
    float ToggleDuration = 0.10f;
};

USTRUCT (BlueprintType)
struct FGridPressurePlateAnimationParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate")
    float ReleasedHeightAboveFloor = 4.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate")
    float PressedHeightAboveFloor = 1.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate")
    float MoveDuration = 0.08f;
};

USTRUCT (BlueprintType)
struct FGridDoorAnimationParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door")
    float OpenHeight = 180.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door")
    float MoveDuration = 2.5f;
};

USTRUCT (BlueprintType)
struct FGridObjectBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    FGridItemBehaviorParams Item;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Teleporter")
    FGridTeleporterBehaviorParams Teleporter;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Transition")
    FGridObjectTransitionParams Transition;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    FGridReceptacleBehaviorParams Receptacle;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    FGridButtonAnimationParams ButtonAnimation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lever")
    FGridLeverAnimationParams LeverAnimation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate")
    FGridPressurePlateAnimationParams PressurePlateAnimation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door")
    FGridDoorAnimationParams DoorAnimation;
};
