#pragma once

#include "CoreMinimal.h"
#include "GridCellTypes.h"
#include "GridLevelTypes.generated.h"

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

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    EGridCellType CellType = EGridCellType::Empty;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    EGridWallType NorthWall = EGridWallType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    EGridWallType EastWall = EGridWallType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    EGridWallType SouthWall = EGridWallType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    EGridWallType WestWall = EGridWallType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    bool bHasCeiling = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    bool bBlocksOccupancy = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    FName ZoneId = NAME_None;
};

USTRUCT (BlueprintType)
struct FGridLevelObjectData
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    FGuid ObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    EGridLevelObjectType Type = EGridLevelObjectType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    int32 CellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    int32 CellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    EGridEdge Edge = EGridEdge::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    FName ArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    bool bInitiallyEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    bool bInitiallyActive = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    FName Tag = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object")
    FString Notes;
};

USTRUCT (BlueprintType)
struct FGridLevelLinkData
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Link")
    FGuid SourceObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Link")
    FGuid TargetObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Link")
    EGridLinkAction Action = EGridLinkAction::Toggle;
};