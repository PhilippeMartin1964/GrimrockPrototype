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

	UFUNCTION(BlueprintCallable, Category = "Map|View")
	bool RecenterMap();

	UFUNCTION(BlueprintCallable, Category = "Map|View")
	bool AdjustZoom(float WheelDelta);

	UFUNCTION(BlueprintCallable, Category = "Map|View")
	void PanMapByPixels(const FVector2D& DeltaPixels);

	UFUNCTION(BlueprintPure, Category = "Map|View")
	float GetZoomScale() const
	{
		return ZoomScale;
	}

	UFUNCTION(BlueprintPure, Category = "Map|View")
	FVector2D GetPanOffsetPixels() const
	{
		return PanOffsetPixels;
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

	/** MON21.6.11 stable presentation noise: same grid primitive + salt always yields the same [-1,1] value. */
	static float ComputeDeterministicArtNoise(const FIntPoint& MapCell, EGridEdge Edge, int32 Salt);

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

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Map|View")
	TObjectPtr<UButton> Button_Recenter;

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
	FLinearColor ExploredCellColor = FLinearColor(0.24f, 0.14f, 0.06f, 0.16f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor WallColor = FLinearColor(0.16f, 0.09f, 0.035f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor DoorColor = FLinearColor(0.20f, 0.11f, 0.04f, 0.98f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor SecretDoorColor = FLinearColor(0.34f, 0.17f, 0.055f, 0.98f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Rendering")
	FLinearColor PartyMarkerColor = FLinearColor(0.55f, 0.055f, 0.025f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art")
	bool bEnableParchmentStyle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art")
	FLinearColor ParchmentColor = FLinearColor(0.55f, 0.37f, 0.20f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art")
	FLinearColor ParchmentGrainColor = FLinearColor(0.12f, 0.065f, 0.025f, 0.10f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art")
	FLinearColor CellHatchColor = FLinearColor(0.14f, 0.075f, 0.025f, 0.20f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art")
	FLinearColor FogFeatherColor = FLinearColor(0.16f, 0.085f, 0.03f, 0.14f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art", meta = (ClampMin = "0.0", ClampMax = "6.0"))
	float HandDrawnJitterPixels = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SecondaryStrokeAlpha = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art", meta = (ClampMin = "0", ClampMax = "96"))
	int32 ParchmentGrainLineCount = 28;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Art", meta = (ClampMin = "0", ClampMax = "8"))
	int32 CellHatchLineCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Symbols")
	FLinearColor NavigationSymbolColor = FLinearColor(0.14f, 0.075f, 0.025f, 0.98f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Symbols")
	FLinearColor HazardSymbolColor = FLinearColor(0.48f, 0.075f, 0.025f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Symbols", meta = (ClampMin = "0.5", ClampMax = "8.0"))
	float SymbolStrokeThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Symbols", meta = (ClampMin = "0.4", ClampMax = "1.0"))
	float SymbolScale = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Symbols", meta = (ClampMin = "4.0", ClampMax = "64.0"))
	float SymbolMinCellPixels = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|View", meta = (ClampMin = "0.10", ClampMax = "4.0"))
	float MinZoomScale = 0.50f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|View", meta = (ClampMin = "1.0", ClampMax = "8.0"))
	float MaxZoomScale = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|View", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ZoomStep = 0.20f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

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
	void RefreshFloorNavigationControls();
	void ResetViewTransform(bool bCenterOnParty);

	UFUNCTION()
	void HandleLevelUpClicked();

	UFUNCTION()
	void HandleLevelDownClicked();

	UFUNCTION()
	void HandleRecenterClicked();

	UPROPERTY(Transient)
	bool bHasRenderableMap = false;

	UPROPERTY(Transient)
	bool bHasFloorSelection = false;

	UPROPERTY(Transient)
	int32 SelectedFloorZ = 0;

	UPROPERTY(Transient)
	float ZoomScale = 1.0f;

	UPROPERTY(Transient)
	FVector2D PanOffsetPixels = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	bool bCenterViewOnParty = false;

	UPROPERTY(Transient)
	bool bIsPanning = false;
};
