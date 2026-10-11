#include "UI/GridTalentDetailLineWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UGridTalentDetailLineWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyPresentation();
}

bool UGridTalentDetailLineWidget::InitializeLine(
	const FGridTalentDetailLineView& InLine,
	UTexture2D* InIcon)
{
	ClearLine();

	if (InLine.Value.IsEmpty())
	{
		return false;
	}

	LineView = InLine;
	ResolvedLabelText = InLine.Label.IsEmpty()
		? FText::GetEmpty()
		: FText::Format(
			NSLOCTEXT("GridTalentDetailLine", "LabelWithSeparator", "{0} :"),
			InLine.Label);
	ResolvedValueText = InLine.Value;
	ResolvedIcon = InIcon;
	bInitialized = true;
	ApplyPresentation();
	return true;
}

void UGridTalentDetailLineWidget::ClearLine()
{
	LineView = FGridTalentDetailLineView();
	ResolvedLabelText = FText::GetEmpty();
	ResolvedValueText = FText::GetEmpty();
	ResolvedIcon = nullptr;
	bInitialized = false;
	ApplyPresentation();
}

void UGridTalentDetailLineWidget::ApplyPresentation()
{
	if (Image_LineIcon)
	{
		Image_LineIcon->SetBrushFromTexture(ResolvedIcon, false);
		Image_LineIcon->SetVisibility(
			ResolvedIcon
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (Text_LineLabel)
	{
		Text_LineLabel->SetText(ResolvedLabelText);
		Text_LineLabel->SetVisibility(
			ResolvedLabelText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
	}

	if (Text_LineValue)
	{
		Text_LineValue->SetText(ResolvedValueText);
		Text_LineValue->SetVisibility(
			ResolvedValueText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
	}
}
