#include "UI/GridEventFeedWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"

UGridEventFeedWidget::UGridEventFeedWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentPanelHeight = ClampPanelHeight(InitialPanelHeight, MinPanelHeight, MaxPanelHeight);
}

void UGridEventFeedWidget::SetPanelHeight(float InHeight)
{
	CurrentPanelHeight = ClampPanelHeight(InHeight, MinPanelHeight, MaxPanelHeight);
	SynchronizeShell();
}

float UGridEventFeedWidget::GetPanelHeight() const
{
	return CurrentPanelHeight;
}

float UGridEventFeedWidget::GetPanelWidth() const
{
	return FMath::Max(1.0f, PanelWidth);
}

float UGridEventFeedWidget::GetMinPanelHeight() const
{
	return FMath::Max(1.0f, FMath::Min(MinPanelHeight, MaxPanelHeight));
}

float UGridEventFeedWidget::GetMaxPanelHeight() const
{
	return FMath::Max(GetMinPanelHeight(), FMath::Max(MinPanelHeight, MaxPanelHeight));
}

float UGridEventFeedWidget::ClampPanelHeight(float InHeight, float InMinHeight, float InMaxHeight)
{
	const float SafeMin = FMath::Max(1.0f, FMath::Min(InMinHeight, InMaxHeight));
	const float SafeMax = FMath::Max(SafeMin, FMath::Max(InMinHeight, InMaxHeight));
	return FMath::Clamp(InHeight, SafeMin, SafeMax);
}

void UGridEventFeedWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	CurrentPanelHeight = ClampPanelHeight(CurrentPanelHeight, MinPanelHeight, MaxPanelHeight);
	SynchronizeShell();
}

TSharedRef<SWidget> UGridEventFeedWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	return SAssignNew(NativeSizeBox, SBox)
		.WidthOverride(GetPanelWidth())
		.HeightOverride(GetPanelHeight())
		[
			SNew(SBorder)
			.Padding(FMargin(6.0f))
			.BorderBackgroundColor(ShellBackgroundColor)
			[
				SAssignNew(NativeScrollBox, SScrollBox)
			]
		];
}

void UGridEventFeedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SynchronizeShell();
}

void UGridEventFeedWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	NativeSizeBox.Reset();
	NativeScrollBox.Reset();
}

void UGridEventFeedWidget::SynchronizeShell()
{
	PanelWidth = FMath::Max(1.0f, PanelWidth);
	CurrentPanelHeight = ClampPanelHeight(CurrentPanelHeight, MinPanelHeight, MaxPanelHeight);

	if (SizeBox_EventFeed)
	{
		SizeBox_EventFeed->SetWidthOverride(PanelWidth);
		SizeBox_EventFeed->SetHeightOverride(CurrentPanelHeight);
	}
	if (Border_EventFeed)
	{
		Border_EventFeed->SetBrushColor(ShellBackgroundColor);
		Border_EventFeed->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (ScrollBox_Entries)
	{
		ScrollBox_Entries->SetVisibility(ESlateVisibility::Visible);
	}
	if (NativeSizeBox.IsValid())
	{
		NativeSizeBox->SetWidthOverride(PanelWidth);
		NativeSizeBox->SetHeightOverride(CurrentPanelHeight);
	}
}
