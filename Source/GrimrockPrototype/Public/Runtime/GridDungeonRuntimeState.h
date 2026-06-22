#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "GridDungeonRuntimeState.generated.h"

class UGridReadableContentAsset;

USTRUCT (BlueprintType)
struct FGridRuntimeDoorState
{
    GENERATED_BODY ()

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bIsOpen = false;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bBlocksMovement = true;
};

USTRUCT (BlueprintType)
struct FGridRuntimeInteractiveState
{
    GENERATED_BODY ()

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bIsActivated = false;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bIsPressed = false;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bIsOn = false;
};

USTRUCT (BlueprintType)
struct FGridRuntimeObjectPresenceState
{
    GENERATED_BODY ()

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bRemovedFromInitialPlacement = false;
};

USTRUCT (BlueprintType)
struct FGridRuntimeItemState
{
    GENERATED_BODY ()

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FName ArchetypeId = NAME_None;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FName ItemDefinitionId = NAME_None;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    int32 Quantity = 1;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    int32 CellX = INDEX_NONE;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    int32 CellY = INDEX_NONE;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    EGridEdge Edge = EGridEdge::None;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FTransform Transform = FTransform::Identity;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bIsSimulatingPhysics = false;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bIsContainedInReceptacle = false;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FGuid ReceptacleObjectId;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bLightsEnabled = true;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    TObjectPtr<UGridReadableContentAsset> ReadableContentAsset = nullptr;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FName ReadableContentId = NAME_None;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FText ReadTitleOverride;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FText ReadTextOverride;
};

USTRUCT (BlueprintType)
struct FGridRuntimeReceptacleState
{
    GENERATED_BODY ()

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    TArray<FGridRuntimeItemState> ContainedItems;
};

USTRUCT (BlueprintType)
struct FGridLevelRuntimeState
{
    GENERATED_BODY ()

    UPROPERTY (SaveGame, BlueprintReadWrite)
    FName LevelId = NAME_None;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeDoorState> Doors;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeInteractiveState> InteractiveObjects;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeObjectPresenceState> ObjectPresence;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeItemState> Items;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    TMap<FGuid, FGridRuntimeReceptacleState> Receptacles;

    UPROPERTY (SaveGame, BlueprintReadWrite)
    bool bHasBeenVisited = false;
};

USTRUCT (BlueprintType)
struct FGridDungeonRuntimeState
{
    GENERATED_BODY ()

    // Runtime state can be embedded in a versioned SaveGame while the actor keeps its owning property transient.
    UPROPERTY (SaveGame, BlueprintReadWrite)
    TMap<FName, FGridLevelRuntimeState> LevelStates;
};
