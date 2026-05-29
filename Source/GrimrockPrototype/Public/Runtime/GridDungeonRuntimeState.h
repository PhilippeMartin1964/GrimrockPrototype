#pragma once

#include "CoreMinimal.h"
#include "GridDungeonRuntimeState.generated.h"

USTRUCT (BlueprintType)
struct FGridRuntimeDoorState
{
    GENERATED_BODY ()

    UPROPERTY (Transient, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bIsOpen = false;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bBlocksMovement = true;
};

USTRUCT (BlueprintType)
struct FGridRuntimeInteractiveState
{
    GENERATED_BODY ()

    UPROPERTY (Transient, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bIsActivated = false;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bIsPressed = false;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bIsOn = false;
};

USTRUCT (BlueprintType)
struct FGridRuntimeObjectPresenceState
{
    GENERATED_BODY ()

    UPROPERTY (Transient, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bRemovedFromInitialPlacement = false;
};

USTRUCT (BlueprintType)
struct FGridRuntimeItemState
{
    GENERATED_BODY ()

    UPROPERTY (Transient, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (Transient, BlueprintReadWrite)
    FName ArchetypeId = NAME_None;

    UPROPERTY (Transient, BlueprintReadWrite)
    FTransform Transform = FTransform::Identity;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bIsSimulatingPhysics = false;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bIsContainedInReceptacle = false;

    UPROPERTY (Transient, BlueprintReadWrite)
    FGuid ReceptacleObjectId;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bLightsEnabled = true;
};

USTRUCT (BlueprintType)
struct FGridRuntimeReceptacleState
{
    GENERATED_BODY ()

    UPROPERTY (Transient, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (Transient, BlueprintReadWrite)
    TArray<FGuid> ContainedItemObjectIds;

    UPROPERTY (Transient, BlueprintReadWrite)
    TArray<FGridRuntimeItemState> ContainedItems;
};

USTRUCT (BlueprintType)
struct FGridRuntimeRemovedInitialReceptacleItemState
{
    GENERATED_BODY ()

    UPROPERTY (Transient, BlueprintReadWrite)
    FGuid ReceptacleObjectId;

    UPROPERTY (Transient, BlueprintReadWrite)
    FGuid InitialItemObjectId;

    UPROPERTY (Transient, BlueprintReadWrite)
    FName InitialItemArchetypeId = NAME_None;
};

USTRUCT (BlueprintType)
struct FGridLevelRuntimeState
{
    GENERATED_BODY ()

    UPROPERTY (Transient, BlueprintReadWrite)
    FName LevelId = NAME_None;

    UPROPERTY (Transient, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeDoorState> Doors;

    UPROPERTY (Transient, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeInteractiveState> InteractiveObjects;

    UPROPERTY (Transient, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeObjectPresenceState> ObjectPresence;

    UPROPERTY (Transient, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeItemState> Items;

    UPROPERTY (Transient, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeReceptacleState> Receptacles;

    // Initial contents removed from a receptacle/support, tracked separately from loose floor item pickups.
    UPROPERTY (Transient, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeRemovedInitialReceptacleItemState> RemovedInitialReceptacleItems;

    UPROPERTY (Transient, BlueprintReadWrite)
    bool bHasBeenVisited = false;
};

USTRUCT (BlueprintType)
struct FGridDungeonRuntimeState
{
    GENERATED_BODY ()

    // Live in-memory dungeon state for the current PIE/gameplay session only.
    UPROPERTY (Transient, BlueprintReadWrite)
    TMap<FName, FGridLevelRuntimeState> LevelStates;
};
