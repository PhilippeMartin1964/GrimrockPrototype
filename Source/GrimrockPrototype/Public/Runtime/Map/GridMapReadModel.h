#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "GridMapReadModel.generated.h"

class UGridDoorSystemComponent;
class UGridLevelAsset;
class UGridWorldObjectDefinitionAsset;
struct FGridLevelRuntimeState;

UENUM(BlueprintType)
enum class EGridMapBoundaryKind : uint8
{
	Wall UMETA(DisplayName = "Wall"),
	Door UMETA(DisplayName = "Door"),
	SecretDoor UMETA(DisplayName = "Secret Door")
};

UENUM(BlueprintType)
enum class EGridMapSymbolKind : uint8
{
	StairsUp UMETA(DisplayName = "Stairs Up"),
	StairsDown UMETA(DisplayName = "Stairs Down"),
	Relocation UMETA(DisplayName = "Relocation"),
	Pit UMETA(DisplayName = "Pit"),
	PointOfInterest UMETA(DisplayName = "Point of Interest")
};

/** One explored cell intentionally exposed to the Map presentation layer. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridMapCellView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FIntPoint LocalCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridCellType CellType = EGridCellType::Floor;
};

/**
 * One known boundary intentionally exposed to the Map presentation layer.
 * No persistent ObjectId or authored definition identity is exposed: an undiscovered secret
 * can therefore be projected as an ordinary Wall without metadata leakage to Blueprint.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridMapBoundaryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FIntPoint LocalCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridEdge Edge = EGridEdge::None;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridMapBoundaryKind Kind = EGridMapBoundaryKind::Wall;

	/** Meaningful only for Door / SecretDoor. This represents traversable/open passage state. */
	UPROPERTY(BlueprintReadOnly, Category = "Map")
	bool bDoorOpen = false;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridMapSymbolView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FIntPoint LocalCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridMapSymbolKind Kind = EGridMapSymbolKind::Relocation;
};

/** MON21.6.6 filtered, transient projection for exactly one canonical 32x32 LevelAsset tile. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridMapTileView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FName LevelId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FGridMapCellView> Cells;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FGridMapBoundaryView> Boundaries;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FGridMapSymbolView> Symbols;

	void Reset()
	{
		LevelId = NAME_None;
		Cells.Reset();
		Boundaries.Reset();
		Symbols.Reset();
	}
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridMapFloorCellView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FIntPoint MapCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridCellType CellType = EGridCellType::Floor;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridMapFloorBoundaryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FIntPoint MapCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridEdge Edge = EGridEdge::None;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridMapBoundaryKind Kind = EGridMapBoundaryKind::Wall;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	bool bDoorOpen = false;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridMapFloorSymbolView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FIntPoint MapCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridMapSymbolKind Kind = EGridMapSymbolKind::Relocation;
};

/** MON21.6.7 seamless projection of all enabled 32x32 tiles sharing one logical Z floor. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridMapFloorView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	int32 SelectedFloorZ = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<int32> AvailableFloorZs;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FGridMapFloorCellView> Cells;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FGridMapFloorBoundaryView> Boundaries;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FGridMapFloorSymbolView> Symbols;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	bool bHasPartyMarker = false;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FIntPoint PartyMapCell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	EGridEdge PartyFacing = EGridEdge::North;

	void Reset()
	{
		SelectedFloorZ = 0;
		AvailableFloorZs.Reset();
		Cells.Reset();
		Boundaries.Reset();
		Symbols.Reset();
		bHasPartyMarker = false;
		PartyMapCell = FIntPoint::ZeroValue;
		PartyFacing = EGridEdge::North;
	}
};

/**
 * MON21.6.6 read-only Map projection.
 *
 * WorldObjectDefinitions classify authored Door variants without hard-coded definition ids.
 * LiveDoorSystem is optional and must only be supplied for the active tile; when absent,
 * persisted door state (then authored initial state) is used.
 */
class GRIMROCKPROTOTYPE_API FGridMapReadModelBuilder
{
public:
	static bool BuildTileView(
		FName LevelId,
		const UGridLevelAsset& LevelAsset,
		const FGridLevelRuntimeState& LevelState,
		const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>>& WorldObjectDefinitions,
		const UGridDoorSystemComponent* LiveDoorSystem,
		FGridMapTileView& OutView);

	/** Sorted distinct logical Z values from enabled dungeon entries with valid LevelAsset data. */
	static void GetAvailableFloorZs(const class UGridDungeonAsset& DungeonAsset, TArray<int32>& OutFloorZs);

	/**
	 * MON21.6.7 composes all enabled tiles on SelectedFloorZ into global map coordinates.
	 * ActivePartyCell/Facing are runtime inputs, never copied into authoritative Map state.
	 */
	static bool BuildFloorView(
		const class UGridDungeonAsset& DungeonAsset,
		const struct FGridDungeonRuntimeState& DungeonState,
		const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>>& WorldObjectDefinitions,
		FName ActiveLevelId,
		const FIntPoint& ActivePartyCell,
		EGridEdge ActivePartyFacing,
		int32 SelectedFloorZ,
		const UGridDoorSystemComponent* ActiveDoorSystem,
		FGridMapFloorView& OutView);
};
