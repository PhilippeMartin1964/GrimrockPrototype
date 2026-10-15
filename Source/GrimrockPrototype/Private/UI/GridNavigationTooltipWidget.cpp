#include "UI/GridNavigationTooltipWidget.h"

#include "Components/TextBlock.h"

void UGridNavigationTooltipWidget::InitializeTooltip(const FText& InText)
{
	TooltipText = InText;
	ApplyTooltipPresentation();
}

void UGridNavigationTooltipWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyTooltipPresentation();
}

void UGridNavigationTooltipWidget::ApplyTooltipPresentation()
{
	if (Text_Tooltip)
	{
		Text_Tooltip->SetText(TooltipText);
	}
}
