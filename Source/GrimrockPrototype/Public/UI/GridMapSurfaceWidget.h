#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GridMapSurfaceWidget.generated.h"

class UGridMapWidget;

/**
 * Presentation-only native surface for WBP_GridMap.
 *
 * This widget owns no map/gameplay state. It renders and interacts with the transient
 * FGridMapFloorView owned by its UGridMapWidget parent/controller.
 */
UCLASS(Blueprintable)
class GRIMROCKPROTOTYPE_API UGridMapSurfaceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeMapSurface(UGridMapWidget* InOwnerMapWidget);

	/** Request a paint invalidation from outside the UUserWidget protected API boundary. */
	void RequestRepaint();

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGridMapWidget> OwnerMapWidget;
};
