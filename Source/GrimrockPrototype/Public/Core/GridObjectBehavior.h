#pragma once

#include "CoreMinimal.h"
#include "GridObjectBehavior.generated.h"

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

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Teleporter")
    FGridTeleporterBehaviorParams Teleporter;

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
