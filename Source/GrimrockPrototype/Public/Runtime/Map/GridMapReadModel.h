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

	void Reset()
	{
		LevelId = NAME_None;
		Cells.Reset();
		Boundaries.Reset();
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
};
