#pragma once

#include "CoreMinimal.h"
#include "UI/GrimrockDesignSurfaceWidget.h"
#include "Runtime/Map/GridMapReadModel.h"
#include "GridMapWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UTextBlock;

/**
 * MON21.6.8 native presentation surface for the existing WBP_GridMap.
 *
 * This widget owns no gameplay/map authority. RefreshMap() rebuilds one transient
 * FGridMapFloorView from the party/runtime sources and NativePaint renders it
 * directly through Slate draw elements (no per-cell UWidget allocation).
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridMapWidget : public UGrimrockDesignSurfaceWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Map")
	void InitializeMapWidget(AGrimrockPartyPawn* InPartyPawn);

	/** Rebuild the currently selected floor. Falls back to the party floor if no selection exists yet. */
	UFUNCTION(BlueprintCallable, Category = "Map")
	bool RefreshMap();

	/** Select the party's current logical floor and rebuild the transient view. */
	UFUNCTION(BlueprintCallable, Category = "Map|Navigation")
	bool SelectPartyFloor();

	UFUNCTION(BlueprintCallable, Category = "Map|Navigation")
	bool NavigateFloorUp();

	UFUNCTION(BlueprintCallable, Category = "Map|Navigation")
	bool NavigateFloorDown();

	UFUNCTION(BlueprintPure, Category = "Map|Navigation")
	bool CanNavigateFloorUp() const;

	UFUNCTION(BlueprintPure, Category = "Map|Navigation")
	bool CanNavigateFloorDown() const;

	UFUNCTION(BlueprintPure, Category = "Map|Navigation")
	int32 GetSelectedFloorZ() const
	{
		return SelectedFloorZ;
	}

	UFUNCTION(BlueprintPure, Category = "Map")
	bool HasRenderableMap() const
	{
		return bHasRenderableMap;
	}

	const FGridMapFloorView& GetFloorView() const
	{
		return FloorView;
	}

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Map")
	FGridMapFloorView FloorView;

	/** Optional MON21.6.9 UMG controls. Exact widget names are intentional. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Map|Navigation")
	TObjectPtr<UButton> Button_LevelUp;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Map|Navigation")
	TObjectPtr<UButton> Button_LevelDown;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Map|Navigation")
	TObjectPtr<UTextBlock> Text_FloorLabel;

	/** Functional MON21.6.8 drawing inset. Artistic framing is deferred to MON21.6.11. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FMargin MapDrawPadding = FMargin(96.0f, 120.0f, 96.0f, 160.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering", meta = (ClampMin = "4.0", ClampMax = "128.0"))
	float MaxCellPixels = 64.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering", meta = (ClampMin = "0.5", ClampMax = "12.0"))
	float WallThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering", meta = (ClampMin = "0.5", ClampMax = "12.0"))
	float DoorThickness = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor ExploredCellColor = FLinearColor(0.28f, 0.24f, 0.18f, 0.34f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor WallColor = FLinearColor(0.78f, 0.72f, 0.58f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor DoorColor = FLinearColor(0.52f, 0.72f, 0.78f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor SecretDoorColor = FLinearColor(0.78f, 0.62f, 0.34f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor PartyMarkerColor = FLinearColor(0.90f, 0.30f, 0.22f, 1.0f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	bool BuildSelectedFloorView();
	bool ResolvePartyFloorZ(int32& OutFloorZ) const;
	bool FindAdjacentFloorZ(bool bUp, int32& OutFloorZ) const;
	FText ResolveSelectedFloorLabel() const;
	void RefreshFloorNavigationControls();

	UFUNCTION()
	void HandleLevelUpClicked();

	UFUNCTION()
	void HandleLevelDownClicked();

	UPROPERTY(Transient)
	bool bHasRenderableMap = false;

	UPROPERTY(Transient)
	bool bHasFloorSelection = false;

	UPROPERTY(Transient)
	int32 SelectedFloorZ = 0;
};
