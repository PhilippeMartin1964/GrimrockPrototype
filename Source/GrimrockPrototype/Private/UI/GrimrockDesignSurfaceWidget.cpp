#include "UI/GrimrockDesignSurfaceWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"

void UGrimrockDesignSurfaceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyDesignSurfaceViewportLimit();
}

void UGrimrockDesignSurfaceWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const FVector2D ViewportPx = UWidgetLayoutLibrary::GetViewportSize(this);
	const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
	if (!ViewportPx.Equals(LastAppliedViewportPx) || !FMath::IsNearlyEqual(ViewportScale, LastAppliedViewportScale))
	{
		ApplyDesignSurfaceViewportLimit();
	}
}

void UGrimrockDesignSurfaceWidget::ApplyDesignSurfaceViewportLimit()
{
	const FVector2D ViewportPx = UWidgetLayoutLibrary::GetViewportSize(this);
	const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));

	UCanvasPanel* RootCanvasPanel = ResolveCanvasPanelRoot();
	UScaleBox* DesignRootScaleBox = ResolveScaleBoxDesignRoot();
	USizeBox* DesignSurfaceSizeBox = ResolveSizeBoxDesignSurface();

	if (!RootCanvasPanel && !DesignRootScaleBox && !DesignSurfaceSizeBox)
	{
		LastAppliedViewportPx = ViewportPx;
		LastAppliedViewportScale = ViewportScale;
		return;
	}

	if (!RootCanvasPanel || !DesignRootScaleBox || !DesignSurfaceSizeBox)
	{
		UE_LOG(LogTemp, Error, TEXT("%s design surface scaling failed: expected CanvasPanel_Root, ScaleBox_DesignRoot, and SizeBox_DesignSurface."),
			*GetNameSafe(this));
		LastAppliedViewportPx = ViewportPx;
		LastAppliedViewportScale = ViewportScale;
		return;
	}

	const float ClampedDesignWidth = FMath::Max(1.0f, DesignWidth);
	const float ClampedDesignHeight = FMath::Max(1.0f, DesignHeight);
	const float ClampedSafeMarginPx = FMath::Max(0.0f, SafeMarginPx);

	const FVector2D AvailablePx(FMath::Max(1.0f, ViewportPx.X - ClampedSafeMarginPx * 2.0f), FMath::Max(1.0f, ViewportPx.Y - ClampedSafeMarginPx * 2.0f));

	const float FitScaleX = AvailablePx.X / ClampedDesignWidth;
	const float FitScaleY = AvailablePx.Y / ClampedDesignHeight;
	const float MaxFitScale = bLimitToDesignSize ? 1.0f : TNumericLimits<float>::Max();
	const float PhysicalFitScale = FMath::Min(MaxFitScale, FMath::Min(FitScaleX, FitScaleY));

	DesignSurfaceSizeBox->SetWidthOverride(ClampedDesignWidth);
	DesignSurfaceSizeBox->SetHeightOverride(ClampedDesignHeight);

	const FVector2D FinalPhysicalSize(ClampedDesignWidth * PhysicalFitScale, ClampedDesignHeight * PhysicalFitScale);
	const FVector2D FinalSlateSlotSize = FinalPhysicalSize / ViewportScale;

	DesignRootScaleBox->SetStretch(EStretch::ScaleToFit);
	DesignRootScaleBox->SetStretchDirection(EStretchDirection::DownOnly);
	DesignRootScaleBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	if (DesignRootScaleBox->GetParent() != RootCanvasPanel)
	{
		UE_LOG(LogTemp, Error, TEXT("%s design surface scaling failed: %s is not directly under CanvasPanel_Root."), *GetNameSafe(this),
			*GetNameSafe(DesignRootScaleBox));
		LastAppliedViewportPx = ViewportPx;
		LastAppliedViewportScale = ViewportScale;
		return;
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DesignRootScaleBox->Slot);
	if (!CanvasSlot)
	{
		UE_LOG(LogTemp, Error, TEXT("%s design surface scaling failed: %s has no CanvasPanelSlot."), *GetNameSafe(this), *GetNameSafe(DesignRootScaleBox));
		LastAppliedViewportPx = ViewportPx;
		LastAppliedViewportScale = ViewportScale;
		return;
	}

	CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasSlot->SetPosition(FVector2D::ZeroVector);
	CanvasSlot->SetSize(FinalSlateSlotSize);
	CanvasSlot->SetAutoSize(false);

	LastAppliedViewportPx = ViewportPx;
	LastAppliedViewportScale = ViewportScale;

	UE_LOG(LogTemp, Log, TEXT("%s DesignSurface ViewportPx=%.0fx%.0f Dpi=%.2f Fit=%.3f Design=%.0fx%.0f SlotSlate=%.1fx%.1f FinalPhysical=%.0fx%.0f"),
		*GetNameSafe(this), ViewportPx.X, ViewportPx.Y, ViewportScale, PhysicalFitScale, ClampedDesignWidth, ClampedDesignHeight, FinalSlateSlotSize.X,
		FinalSlateSlotSize.Y, FinalPhysicalSize.X, FinalPhysicalSize.Y);
}

UCanvasPanel* UGrimrockDesignSurfaceWidget::ResolveCanvasPanelRoot() const
{
	if (CanvasPanel_Root)
	{
		return CanvasPanel_Root;
	}

	return WidgetTree ? WidgetTree->FindWidget<UCanvasPanel>(TEXT("CanvasPanel_Root")) : nullptr;
}

UScaleBox* UGrimrockDesignSurfaceWidget::ResolveScaleBoxDesignRoot() const
{
	if (ScaleBox_DesignRoot)
	{
		return ScaleBox_DesignRoot;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	if (UScaleBox* DesignRoot = WidgetTree->FindWidget<UScaleBox>(TEXT("ScaleBox_DesignRoot")))
	{
		return DesignRoot;
	}

	return WidgetTree->FindWidget<UScaleBox>(TEXT("ScaleBox_MenuRoot"));
}

USizeBox* UGrimrockDesignSurfaceWidget::ResolveSizeBoxDesignSurface() const
{
	if (SizeBox_DesignSurface)
	{
		return SizeBox_DesignSurface;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	if (USizeBox* DesignSurface = WidgetTree->FindWidget<USizeBox>(TEXT("SizeBox_DesignSurface")))
	{
		return DesignSurface;
	}

	return WidgetTree->FindWidget<USizeBox>(TEXT("SizeBox_MenuDesign"));
}
