#include "UI/GridDungeonBuildProgressWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GridDungeonBuildProgressWidget"

UGridDungeonBuildProgressWidget::UGridDungeonBuildProgressWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultTitle = LOCTEXT("DefaultDungeonBuildTitle", "Construction du donjon");
	DefaultStatusText = LOCTEXT("DefaultDungeonBuildStatus", "Préparation...");
	CurrentTitle = DefaultTitle;
	CurrentStatusText = DefaultStatusText;
}

void UGridDungeonBuildProgressWidget::SetBuildTitle(const FText& InTitle)
{
	CurrentTitle = InTitle.IsEmpty() ? DefaultTitle : InTitle;
	SynchronizeNativeAndBlueprintWidgets();
}

void UGridDungeonBuildProgressWidget::SetBuildProgress(float InProgress, const FText& InStatusText)
{
	CurrentProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	if (!InStatusText.IsEmpty())
	{
		CurrentStatusText = InStatusText;
	}
	SynchronizeNativeAndBlueprintWidgets();
}

float UGridDungeonBuildProgressWidget::GetBuildProgress() const
{
	return CurrentProgress;
}

FText UGridDungeonBuildProgressWidget::GetBuildStatusText() const
{
	return CurrentStatusText;
}

void UGridDungeonBuildProgressWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	SynchronizeNativeAndBlueprintWidgets();
}

TSharedRef<SWidget> UGridDungeonBuildProgressWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	return SNew(SBorder)
		.Padding(FMargin(48.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)[SNew(SBox).WidthOverride(640.0f)[SNew(SVerticalBox) +
			SVerticalBox::Slot().AutoHeight().Padding(
				0.0f, 0.0f, 0.0f, 16.0f)[SAssignNew(NativeTitleText, STextBlock).Justification(ETextJustify::Center).Text(CurrentTitle)] +
			SVerticalBox::Slot().AutoHeight().Padding(
				0.0f, 0.0f, 0.0f, 8.0f)[SAssignNew(NativeStatusText, STextBlock).Justification(ETextJustify::Center).Text(CurrentStatusText)] +
			SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)[SAssignNew(NativeProgressBar, SProgressBar).Percent(CurrentProgress)] +
			SVerticalBox::Slot().AutoHeight()[SAssignNew(NativePercentText, STextBlock).Justification(ETextJustify::Center).Text(BuildPercentText())]]];
}

void UGridDungeonBuildProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SynchronizeNativeAndBlueprintWidgets();
}

void UGridDungeonBuildProgressWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	NativeProgressBar.Reset();
	NativeTitleText.Reset();
	NativeStatusText.Reset();
	NativePercentText.Reset();
}

FText UGridDungeonBuildProgressWidget::BuildPercentText() const
{
	return FText::Format(LOCTEXT("DungeonBuildPercentFormat", "{0} %"), FText::AsNumber(FMath::RoundToInt(CurrentProgress * 100.0f)));
}

void UGridDungeonBuildProgressWidget::SynchronizeNativeAndBlueprintWidgets()
{
	if (CurrentTitle.IsEmpty())
	{
		CurrentTitle = DefaultTitle;
	}
	if (CurrentStatusText.IsEmpty())
	{
		CurrentStatusText = DefaultStatusText;
	}

	const FText PercentText = BuildPercentText();

	if (ProgressBar_BuildProgress)
		ProgressBar_BuildProgress->SetPercent(CurrentProgress);
	if (Text_Title)
		Text_Title->SetText(CurrentTitle);
	if (Text_Status)
		Text_Status->SetText(CurrentStatusText);
	if (Text_Percent)
		Text_Percent->SetText(PercentText);
	if (NativeProgressBar.IsValid())
		NativeProgressBar->SetPercent(CurrentProgress);
	if (NativeTitleText.IsValid())
		NativeTitleText->SetText(CurrentTitle);
	if (NativeStatusText.IsValid())
		NativeStatusText->SetText(CurrentStatusText);
	if (NativePercentText.IsValid())
		NativePercentText->SetText(PercentText);
}

#undef LOCTEXT_NAMESPACE
