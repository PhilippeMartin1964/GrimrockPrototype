#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "Widgets/SCompoundWidget.h"
#include "GridItemWorkshopOrbit.h"
#include "InputCoreTypes.h"
#include "Runtime/Inspection/GridItemInspectionTypes.h"

class FAdvancedPreviewScene;
class AGridItemPreviewActor;
class FGridItemWorkshopViewportClient;
class UStaticMeshComponent;

class SGridItemWorkshopViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SGridItemWorkshopViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SGridItemWorkshopViewport() override;

	EGridItemPreviewResult Display(const FGridItemInspectionView& View, bool bRecenter);
	void Frame(const FRotator& Rotation = FRotator(-12.0, 35.0, 0.0));
	void Clear();
	void OrbitDrag(const FVector2D& Delta);
	void OrbitZoom(double WheelDelta);
	void OrbitPan(const FVector2D& Delta, double ProjectionAxisPixels);
	bool HasOrbitTarget() const { return Orbit.bReady; }
	UStaticMeshComponent* GetMesh() const;

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override { return nullptr; }

private:
	void ApplyOrbit();
	FGridItemWorkshopOrbit Orbit;
	TSharedPtr<FAdvancedPreviewScene> Scene;
	TSharedPtr<FGridItemWorkshopViewportClient> WorkshopClient;
	TWeakObjectPtr<AGridItemPreviewActor> Preview;
};

/** Hit-testable overlay exclusively owning pointer navigation. Native
 * SViewport remains the renderer, not the input surface. */
class SGridItemWorkshopOrbitInput : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridItemWorkshopOrbitInput) {}
		SLATE_ARGUMENT(TSharedPtr<SGridItemWorkshopViewport>, PreviewViewport)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	TWeakPtr<SGridItemWorkshopViewport> PreviewViewport;
	FKey ActiveDragButton;
};
