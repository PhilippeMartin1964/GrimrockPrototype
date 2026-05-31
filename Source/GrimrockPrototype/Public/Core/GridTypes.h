#pragma once

#include "CoreMinimal.h"
#include "GridObjectBehavior.h"
#include "GridTypes.generated.h"

class UGridItemDefinitionAsset;

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
    Solid       UMETA (DisplayName = "Solid")
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
    Trigger         UMETA (DisplayName = "Trigger"),
    Receptacle      UMETA (DisplayName = "Receptacle"),
    Item            UMETA (DisplayName = "Item")
};

UENUM (BlueprintType)
enum class EGridObjectPlacementKind : uint8
{
    Center  UMETA (DisplayName = "Center"),
    Edge    UMETA (DisplayName = "Edge"),
    Floor   UMETA (DisplayName = "Floor"),
    Wall    UMETA (DisplayName = "Wall"),
    Ceiling UMETA (DisplayName = "Ceiling")
};

UENUM (BlueprintType)
enum class EGridObjectCategory : uint8
{
    Mechanism  UMETA (DisplayName = "Mechanism"),
    Decoration UMETA (DisplayName = "Decoration"),
    Prop       UMETA (DisplayName = "Prop"),
    Receptacle UMETA (DisplayName = "Receptacle"),
    Light      UMETA (DisplayName = "Light"),
    Readable   UMETA (DisplayName = "Readable"),
    Spawn      UMETA (DisplayName = "Spawn"),
    Teleporter UMETA (DisplayName = "Teleporter"),
    Item       UMETA (DisplayName = "Item")
};

UENUM (BlueprintType)
enum class EGridObjectCommand : uint8
{
    Toggle      UMETA (DisplayName = "Toggle"),
    Open        UMETA (DisplayName = "Open"),
    Close       UMETA (DisplayName = "Close"),
    Activate    UMETA (DisplayName = "Activate"),
    Deactivate  UMETA (DisplayName = "Deactivate"),
    Enable      UMETA (DisplayName = "Enable"),
    Disable     UMETA (DisplayName = "Disable"),
    Lock        UMETA (DisplayName = "Lock"),
    Unlock      UMETA (DisplayName = "Unlock"),
    Spawn       UMETA (DisplayName = "Spawn"),
    Despawn     UMETA (DisplayName = "Despawn"),
    Teleport    UMETA (DisplayName = "Teleport"),
    ShowMessage UMETA (DisplayName = "Show Message")
};

UENUM (BlueprintType)
enum class EGridObjectEvent : uint8
{
    Activated    UMETA (DisplayName = "Activated"),
    Deactivated  UMETA (DisplayName = "Deactivated"),
    ItemInserted UMETA (DisplayName = "Item Inserted"),
    ItemRemoved  UMETA (DisplayName = "Item Removed"),
    ItemChanged  UMETA (DisplayName = "Item Changed"),
    Used         UMETA (DisplayName = "Used"),
    Entered      UMETA (DisplayName = "Entered"),
    Exited       UMETA (DisplayName = "Exited"),
    Opened       UMETA (DisplayName = "Opened"),
    Closed       UMETA (DisplayName = "Closed"),
    Enabled      UMETA (DisplayName = "Enabled"),
    Disabled     UMETA (DisplayName = "Disabled")
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
    float LocalYaw = 0.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FName ArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    TObjectPtr<UGridItemDefinitionAsset> ItemDefinitionAsset = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ItemDefinitionId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    bool bInitiallyEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    bool bInitiallyActive = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FName Tag = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FString Notes;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Readable", meta = (MultiLine = "true"))
    FText OverrideReadableText;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FName PaletteEntryId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FGridObjectBehaviorParams Behavior;
};

USTRUCT (BlueprintType)
struct FGridObjectLink
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FGuid SourceObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    FGuid TargetObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridObjectEvent SourceEvent = EGridObjectEvent::Activated;

    UPROPERTY (EditAnywhere, BlueprintReadWrite)
    EGridObjectCommand Command = EGridObjectCommand::Toggle;
};

USTRUCT (BlueprintType)
struct FGridEdgeKey
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 X = INDEX_NONE;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 Y = INDEX_NONE;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Grid")
    EGridEdge Edge = EGridEdge::None;

    FGridEdgeKey () = default;

    FGridEdgeKey (int32 InX, int32 InY, EGridEdge InEdge)
        : X (InX)
        , Y (InY)
        , Edge (InEdge)
    {}

    friend bool operator==(const FGridEdgeKey& A, const FGridEdgeKey& B)
    {
        return A.X == B.X && A.Y == B.Y && A.Edge == B.Edge;
    }
};

FORCEINLINE uint32 GetTypeHash (const FGridEdgeKey& Key)
{
    uint32 Hash = GetTypeHash (Key.X);
    Hash = HashCombine (Hash, GetTypeHash (Key.Y));
    Hash = HashCombine (Hash, GetTypeHash (static_cast<uint8>(Key.Edge)));
    return Hash;
}
