#include "UI/GridPartyPortraitHudWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

void UGridPartyPortraitHudWidget::SetPortraitView(const FGridPartyPortraitView& InView)
{
	View = InView;
	RefreshBoundVisuals();
	RefreshPortraitVisual();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

FGuid UGridPartyPortraitHudWidget::GetCharacterId() const
{
	return View.CharacterId;
}

bool UGridPartyPortraitHudWidget::IsSelected() const
{
	return View.bIsSelected;
}

void UGridPartyPortraitHudWidget::RefreshPortraitVisual_Implementation()
{
}

void UGridPartyPortraitHudWidget::RefreshBoundVisuals()
{
	if (Image_Portrait)
	{
		if (View.Portrait.IsNull())
		{
			Image_Portrait->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_Portrait->SetBrushFromSoftTexture(View.Portrait, false);
			Image_Portrait->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (ProgressBar_Health)
	{
		ProgressBar_Health->SetPercent(FMath::Clamp(View.HealthRatio, 0.0f, 1.0f));
		ProgressBar_Health->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ProgressBar_Mana)
	{
		ProgressBar_Mana->SetPercent(View.bHasMana ? FMath::Clamp(View.ManaRatio, 0.0f, 1.0f) : 0.0f);
		ProgressBar_Mana->SetVisibility(View.bHasMana ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ProgressBar_PhysicalArmor)
	{
		ProgressBar_PhysicalArmor->SetPercent(FMath::Clamp(View.PhysicalArmorRatio, 0.0f, 1.0f));
		ProgressBar_PhysicalArmor->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ProgressBar_MagicalArmor)
	{
		ProgressBar_MagicalArmor->SetPercent(FMath::Clamp(View.MagicalArmorRatio, 0.0f, 1.0f));
		ProgressBar_MagicalArmor->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (Border_Selected)
	{
		Border_Selected->SetVisibility(View.bIsSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
