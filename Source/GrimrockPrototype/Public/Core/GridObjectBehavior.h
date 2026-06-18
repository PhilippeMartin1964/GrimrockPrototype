#pragma once

#include "CoreMinimal.h"
#include "GridObjectBehavior.generated.h"

class UGridItemDefinitionAsset;
class UGridReadableContentAsset;

UENUM (BlueprintType)
enum class EGridReceptacleVisualPlacementMode : uint8
{
    AttachedSocket = 0,
    PhysicalAtHit = 1
};

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

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item|Reading")
    TObjectPtr<UGridReadableContentAsset> DefaultReadableContentAsset = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item|Reading")
    FName DefaultReadableContentId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item|Reading")
    FText DefaultReadTitleOverride;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item|Reading", meta = (MultiLine = "true"))
    FText DefaultReadTextOverride;
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
struct FGridReceptacleInitialItemConfig
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Initial Content")
    TObjectPtr<UGridItemDefinitionAsset> ItemDefinition = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Initial Content", meta = (ClampMin = "1"))
    int32 Quantity = 1;
};

USTRUCT (BlueprintType)
struct FGridReceptacleAcceptedItemConfig
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Accepted Items")
    TObjectPtr<UGridItemDefinitionAsset> ItemDefinition = nullptr;
};

USTRUCT (BlueprintType)
struct FGridReceptacleBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bAcceptAnyItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Accepted Items",
        meta = (EditCondition = "!bAcceptAnyItem", EditConditionHides))
    TArray<FGridReceptacleAcceptedItemConfig> AcceptedItems;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Initial Content")
    TArray<FGridReceptacleInitialItemConfig> InitialContent;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Max Contained Items", ClampMin = "1"))
    int32 MaxContainedItems = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    EGridReceptacleVisualPlacementMode VisualPlacementMode =
        EGridReceptacleVisualPlacementMode::AttachedSocket;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Simulate Physics When Placed",
            ToolTip = "If true, an item placed at the clicked surface point keeps physics and gravity enabled. If false, it is placed statically but remains clickable."))
    bool bSimulatePhysicsWhenPlaced = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Physical Placement Surface Offset",
            ClampMin = "0.0",
            ToolTip = "Distance in centimeters added along the clicked surface normal before physics starts."))
    float PhysicalPlacementSurfaceOffset = 10.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle",
        meta = (DisplayName = "Physical Placement Initial Rotation Offset",
            ToolTip = "Rotation offset applied relative to the receptacle attach-point orientation before physics starts."))
    FRotator PhysicalPlacementInitialRotationOffset = FRotator::ZeroRotator;
};

USTRUCT (BlueprintType)
struct FGridLockBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lock")
    bool bStartsUnlocked = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lock")
    bool bConsumeKeyOnUnlock = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lock|Accepted Keys")
    TArray<TObjectPtr<UGridItemDefinitionAsset>> AcceptedKeyItems;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lock|Accepted Keys")
    TArray<FName> AcceptedKeyIds;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lock|Messages")
    FText LockedMessage;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lock|Messages")
    FText UnlockedMessage;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lock|Messages")
    FText MissingKeyMessage;
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
struct FGridPressurePlateWeightParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate|Weight")
    bool bActivateWhenPartyPresent = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate|Weight")
    bool bUseItemWeight = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate|Weight", meta = (ClampMin = "0.0", EditCondition = "bUseItemWeight"))
    float RequiredItemWeight = 1.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate|Weight", meta = (EditCondition = "bUseItemWeight"))
    bool bCountEdgeItems = false;
};

USTRUCT (BlueprintType)
struct FGridDoorAnimationParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door")
    float OpenHeight = 180.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door")
    float MoveDuration = 2.5f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door|Chain")
    bool bHasChainMechanism = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door|Chain", meta = (EditCondition = "bHasChainMechanism", ClampMin = "0.0"))
    float ChainPullDistance = 20.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door|Chain", meta = (EditCondition = "bHasChainMechanism", ClampMin = "0.01"))
    float ChainPullDuration = 0.25f;
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

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lock")
    FGridLockBehaviorParams Lock;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    FGridButtonAnimationParams ButtonAnimation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lever")
    FGridLeverAnimationParams LeverAnimation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate")
    FGridPressurePlateAnimationParams PressurePlateAnimation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Pressure Plate")
    FGridPressurePlateWeightParams PressurePlateWeight;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Door")
    FGridDoorAnimationParams DoorAnimation;
};
