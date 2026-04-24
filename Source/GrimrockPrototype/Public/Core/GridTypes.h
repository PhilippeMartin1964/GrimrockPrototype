#pragma once

#include "CoreMinimal.h"
#include "GridObjectBehavior.h"
#include "GridTypes.generated.h"

UENUM (BlueprintType)
enum class EGridCellType : uint8
{
    Empty      UMETA (DisplayName = "Empty"),
    Floor      UMETA (DisplayName = "Floor"),
    Pit        UMETA (DisplayName = "Pit"),
    StairsUp   UMETA (DisplayName = "Stairs Up"),
    StairsDown UMETA (DisplayName = "Stairs Down"),
    Teleporter UMETA (DisplayName = "Teleporter")
};

UENUM (BlueprintType)
enum class EGridWallType : uint8
{
    None        UMETA (DisplayName = "None"),
    Solid       UMETA (DisplayName = "Solid"),
    Door        UMETA (DisplayName = "Door"),
    DoorOpen    UMETA (DisplayName = "Door Open"),
    Secret      UMETA (DisplayName = "Secret")
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

UENUM (BlueprintType)
enum class EGridLevelObjectType : uint8
{
    None            UMETA (DisplayName = "None"),
    Door            UMETA (DisplayName = "Door"),
    Button          UMETA (DisplayName = "Button"),
    PressurePlate   UMETA (DisplayName = "Pressure Plate"),
    Lever           UMETA (DisplayName = "Lever"),
    Decoration      UMETA (DisplayName = "Decoration"),
    MonsterSpawn    UMETA (DisplayName = "Monster Spawn"),
    ItemSpawn       UMETA (DisplayName = "Item Spawn"),
    Light           UMETA (DisplayName = "Light"),
    Teleporter      UMETA (DisplayName = "Teleporter"),
    Trigger         UMETA (DisplayName = "Trigger")
};

UENUM (BlueprintType)
enum class EGridLinkAction : uint8
{
    Toggle      UMETA (DisplayName = "Toggle"),
    Open        UMETA (DisplayName = "Open"),
    Close       UMETA (DisplayName = "Close"),
    Activate    UMETA (DisplayName = "Activate"),
    Deactivate  UMETA (DisplayName = "Deactivate")
};

USTRUCT (BlueprintType)
struct FGridLevelCellData
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridCellType CellType = EGridCellType::Empty;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridWallType NorthWall = EGridWallType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridWallType EastWall = EGridWallType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridWallType SouthWall = EGridWallType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridWallType WestWall = EGridWallType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    bool bHasCeiling = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    bool bBlocksOccupancy = false;
};

USTRUCT (BlueprintType)
struct FGridLevelObjectData
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridLevelObjectType Type = EGridLevelObjectType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    int32 CellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    int32 CellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridEdge Edge = EGridEdge::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FName ArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    bool bInitiallyEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    bool bInitiallyActive = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FName Tag = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FString Notes;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FName PaletteEntryId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FGridObjectBehaviorParams Behavior;
};

USTRUCT (BlueprintType)
struct FGridLevelLinkData
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FGuid SourceObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FGuid TargetObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridLinkAction Action = EGridLinkAction::Toggle;
};