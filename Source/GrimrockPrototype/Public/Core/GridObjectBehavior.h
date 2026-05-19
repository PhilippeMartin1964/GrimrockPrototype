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
struct FGridActivationBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Activation")
    EGridObjectTriggerMode TriggerMode = EGridObjectTriggerMode::Instant;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Activation",
        meta = (ClampMin = "0.0", ToolTip = "Currently planned for delayed/timed activation. Not fully used by runtime yet."))
    float Delay = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Activation",
        meta = (ClampMin = "0.0", ToolTip = "Currently planned for delayed/timed activation. Not fully used by runtime yet."))
    float Duration = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Activation")
    bool bInvertLinks = false;
};

USTRUCT (BlueprintType)
struct FGridTriggerBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    bool bFireOnEnter = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    bool bFireOnExit = false;
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
struct FGridItemSpawnBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item Spawn",
        meta = (ToolTip = "Item archetype spawned or represented by this ItemSpawn. Runtime spawning is planned but may not be implemented yet."))
    FName SpawnedItemArchetypeId = NAME_None;
};

USTRUCT (BlueprintType)
struct FGridObjectBehaviorParams
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Activation")
    FGridActivationBehaviorParams Activation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    FGridTriggerBehaviorParams Trigger;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Teleporter")
    FGridTeleporterBehaviorParams Teleporter;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    FGridReceptacleBehaviorParams Receptacle;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Button")
    FGridButtonAnimationParams ButtonAnimation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item Spawn")
    FGridItemSpawnBehaviorParams ItemSpawn;
};
