#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridMapVisualThemeAsset.generated.h"

class UTexture2D;

/**
 * MAP-THEME01 presentation-only texture set for the native dungeon map renderer.
 *
 * The theme owns no exploration, topology, navigation or SaveGame state.
 * Textures are strong references so a configured theme is cook-safe and requires
 * no synchronous asset loading from NativePaint.
 */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridMapVisualThemeAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Textures")
	TObjectPtr<UTexture2D> ParchmentTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Textures")
	TObjectPtr<UTexture2D> WallTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Textures")
	TObjectPtr<UTexture2D> DoorTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Textures")
	TObjectPtr<UTexture2D> SecretDoorTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Symbols")
	TObjectPtr<UTexture2D> StairsUpTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Symbols")
	TObjectPtr<UTexture2D> StairsDownTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Symbols")
	TObjectPtr<UTexture2D> RelocationTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Symbols")
	TObjectPtr<UTexture2D> PitTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Symbols")
	TObjectPtr<UTexture2D> PointOfInterestTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Symbols")
	TObjectPtr<UTexture2D> PartyMarkerTexture;

	/** Boundary strip thickness as a fraction of the current rendered cell size. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Layout", meta = (ClampMin = "0.03", ClampMax = "0.50"))
	float BoundaryThicknessRatio = 0.16f;

	/** Square symbol size as a fraction of the current rendered cell size. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Layout", meta = (ClampMin = "0.20", ClampMax = "1.00"))
	float SymbolScale = 0.72f;

	/** Party-marker square size as a fraction of the current rendered cell size. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Layout", meta = (ClampMin = "0.20", ClampMax = "1.00"))
	float PartyMarkerScale = 0.78f;

	/** Hide textured cell symbols below this rendered cell size. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Theme|Layout", meta = (ClampMin = "4.0", ClampMax = "64.0"))
	float SymbolMinCellPixels = 12.0f;
};
