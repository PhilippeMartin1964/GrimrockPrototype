#pragma once

#include "CoreMinimal.h"
#include "GridObjectBehavior.generated.h"

UENUM (BlueprintType)
enum class EGridObjectTriggerMode : uint8
{
    Instant     UMETA (DisplayName = "Instant"),
    Hold        UMETA (DisplayName = "Hold"),
    Toggle      UMETA (DisplayName = "Toggle"),
    OneShot     UMETA (DisplayName = "One Shot")
};

USTRUCT (BlueprintType)
struct FGridObjectBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    EGridObjectTriggerMode TriggerMode = EGridObjectTriggerMode::Instant;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0"))
    float Delay = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0"))
    float Duration = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    bool bInvertLinks = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    bool bFireOnEnter = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    bool bFireOnExit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    bool bOneShotConsumed = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    int32 TargetCellX = INDEX_NONE;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    int32 TargetCellY = INDEX_NONE;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bAcceptAnyItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    TArray<FName> AcceptedItemTags;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    TArray<FName> AcceptedArchetypeIds;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    FName InitialContainedItemArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    float ButtonPressDistance = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    float ButtonPressDuration = 0.08f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    float ButtonReleaseDuration = 0.10f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    float ButtonHoldTime = 0.15f;
};
