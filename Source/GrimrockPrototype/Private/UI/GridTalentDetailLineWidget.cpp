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
	UTexture2D* InIcon,
	const FText& InResolvedLabelText,
	const FText& InResolvedValueText)
{
	ClearLine();

	if (InResolvedValueText.IsEmpty() && !InLine.bHasNumericValue)
	{
		return false;
	}

	LineView = InLine;
	ResolvedLabelText = InResolvedLabelText;
	ResolvedNumericValue = InLine.NumericValue;
	ResolvedValueStyle = InLine.ValueStyle;
	bShowPositiveSign =
		InLine.bHasNumericValue &&
		InLine.NumericValue > 0 &&
		(InLine.ValueStyle == EGridTalentDetailValueStyle::SignedInteger ||
			InLine.ValueStyle == EGridTalentDetailValueStyle::SignedPercent);
	bShowPercentUnit =
		InLine.bHasNumericValue &&
		(InLine.ValueStyle == EGridTalentDetailValueStyle::Percent ||
			InLine.ValueStyle == EGridTalentDetailValueStyle::SignedPercent);
	ResolvedValueText =
		InLine.bHasNumericValue
			? FText::AsNumber(InLine.NumericValue)
			: InResolvedValueText;
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
	ResolvedNumericValue = 0;
	ResolvedValueStyle = EGridTalentDetailValueStyle::Text;
	bShowPositiveSign = false;
	bShowPercentUnit = false;
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
