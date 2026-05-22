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
struct FGridObjectBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Teleporter")
    FGridTeleporterBehaviorParams Teleporter;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    FGridReceptacleBehaviorParams Receptacle;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    FGridButtonAnimationParams ButtonAnimation;
};
