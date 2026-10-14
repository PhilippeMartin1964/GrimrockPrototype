#include "UI/GridTalentVariantIconWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UGridTalentVariantIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Variant)
	{
		Button_Variant->OnClicked.RemoveDynamic(this, &UGridTalentVariantIconWidget::HandleVariantClicked);
		Button_Variant->OnClicked.AddUniqueDynamic(this, &UGridTalentVariantIconWidget::HandleVariantClicked);
	}
}

void UGridTalentVariantIconWidget::NativeDestruct()
{
	if (Button_Variant)
	{
		Button_Variant->OnClicked.RemoveDynamic(this, &UGridTalentVariantIconWidget::HandleVariantClicked);
	}
	Super::NativeDestruct();
}

bool UGridTalentVariantIconWidget::InitializeVariantIcon(FName InChoiceId, UTexture2D* IconTexture)
{
	if (InChoiceId.IsNone())
	{
		return false;
	}

	ChoiceId = InChoiceId;
	if (Image_VariantIcon)
	{
		Image_VariantIcon->SetBrushFromTexture(IconTexture, false);
		Image_VariantIcon->SetVisibility(
			IconTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	SetToolTipText(FText::GetEmpty());
	return true;
}

void UGridTalentVariantIconWidget::HandleVariantClicked()
{
	if (!ChoiceId.IsNone())
	{
		OnVariantIconClicked.Broadcast(ChoiceId);
	}
}
