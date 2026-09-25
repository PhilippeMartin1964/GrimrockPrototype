#include "UI/GridPartyMemberWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGClassVisualAsset.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UI/GridInventoryWidget.h"

void UGridPartyMemberWidget::InitializePartyMember(int32 InCharacterIndex)
{
	CharacterIndex = InCharacterIndex;
}

void UGridPartyMemberWidget::SetCharacterSummary(const FGridInventoryCharacterSummary& InSummary)
{
	CachedSummary = InSummary;
	CharacterIndex = InSummary.CharacterIndex;
	RefreshBoundMemberVisuals();
	RefreshMemberVisual();
}

void UGridPartyMemberWidget::SetStatusEffects(const TArray<FGridStatusEffectPresentationView>& InStatusEffects)
{
	CachedStatusEffects = InStatusEffects;
	RefreshBoundStatusEffects();
}

void UGridPartyMemberWidget::SetAvailableClassVisuals(const TArray<URPGClassVisualAsset*>& InAvailableClassVisuals)
{
	AvailableClassVisuals.Reset();
	for (URPGClassVisualAsset* ClassVisual : InAvailableClassVisuals)
	{
		if (ClassVisual)
		{
			AvailableClassVisuals.Add(ClassVisual);
		}
	}
	RefreshBoundMemberVisuals();
}

bool UGridPartyMemberWidget::IsSelected() const
{
	return CachedSummary.bIsSelected;
}

void UGridPartyMemberWidget::HandleClicked()
{
	OnPartyMemberClicked.Broadcast(CharacterIndex);
}

void UGridPartyMemberWidget::SetOwnerInventoryWidget(UGridInventoryWidget* InOwnerInventoryWidget)
{
	OwningInventoryWidget = InOwnerInventoryWidget;
}

bool UGridPartyMemberWidget::NativeOnDrop(
	const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UGridInventoryDragDropOperation* Operation = Cast<UGridInventoryDragDropOperation>(InOperation);
	if (!Operation || !OwningInventoryWidget)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	return OwningInventoryWidget->HandlePartyMemberItemDrop(Operation, CharacterIndex);
}

void UGridPartyMemberWidget::RefreshMemberVisual_Implementation()
{
}

const URPGClassVisualAsset* UGridPartyMemberWidget::FindClassVisualForCachedClass() const
{
	if (CachedSummary.ClassId.IsNone())
	{
		return nullptr;
	}

	for (const URPGClassVisualAsset* ClassVisual : AvailableClassVisuals)
	{
		if (ClassVisual && ClassVisual->IsValidForClass(CachedSummary.ClassId))
		{
			return ClassVisual;
		}
	}

	return nullptr;
}

void UGridPartyMemberWidget::RefreshBoundMemberVisuals()
{
	const URPGClassVisualAsset* ClassVisual = FindClassVisualForCachedClass();
	const TSoftObjectPtr<UTexture2D> ClassIcon = ClassVisual && !ClassVisual->ClassIcon.IsNull() ? ClassVisual->ClassIcon : CachedSummary.ClassIcon;

	if (Image_Portrait)
	{
		if (CachedSummary.Portrait.IsNull())
		{
			Image_Portrait->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_Portrait->SetBrushFromSoftTexture(CachedSummary.Portrait, false);
			Image_Portrait->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (Image_ClassIcon)
	{
		if (ClassIcon.IsNull())
		{
			Image_ClassIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_ClassIcon->SetBrushFromSoftTexture(ClassIcon, false);
			Image_ClassIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (Border_ClassAccent)
	{
		if (!ClassVisual)
		{
			Border_ClassAccent->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Border_ClassAccent->SetBrushColor(ClassVisual->AccentColor);
			Border_ClassAccent->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (Border_Selected)
	{
		Border_Selected->SetVisibility(CachedSummary.bIsSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Image_WeightAlert)
	{
		Image_WeightAlert->SetVisibility(
			CachedSummary.WeightState == EGridInventoryWeightState::Overloaded ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}


void UGridPartyMemberWidget::RefreshBoundStatusEffects()
{
	if (!HorizontalBox_StatusEffects)
	{
		return;
	}

	HorizontalBox_StatusEffects->ClearChildren();
	if (CachedStatusEffects.IsEmpty())
	{
		HorizontalBox_StatusEffects->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const int32 SafeMaxIndicators = FMath::Clamp(MaxStatusEffectIndicators, 2, 8);
	const bool bHasOverflow = CachedStatusEffects.Num() > SafeMaxIndicators;
	const int32 DirectIndicatorCount =
		bHasOverflow ? SafeMaxIndicators - 1 : FMath::Min(CachedStatusEffects.Num(), SafeMaxIndicators);

	auto CreateSizeBox = [this]() -> USizeBox*
	{
		USizeBox* Box = WidgetTree ? WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()) : NewObject<USizeBox>(this);
		if (Box)
		{
			Box->SetWidthOverride(StatusEffectIndicatorSize);
			Box->SetHeightOverride(StatusEffectIndicatorSize);
		}
		return Box;
	};

	for (int32 Index = 0; Index < DirectIndicatorCount; ++Index)
	{
		const FGridStatusEffectPresentationView& Status = CachedStatusEffects[Index];
		USizeBox* IndicatorBox = CreateSizeBox();
		if (!IndicatorBox)
		{
			continue;
		}

		IndicatorBox->SetToolTipText(Status.ToolTipText);
		if (!Status.Icon.IsNull())
		{
			UImage* Icon = WidgetTree ? WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()) : NewObject<UImage>(this);
			if (Icon)
			{
				Icon->SetBrushFromSoftTexture(Status.Icon, false);
				Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
				IndicatorBox->AddChild(Icon);
			}
		}
		else
		{
			UTextBlock* Fallback = WidgetTree ? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()) : NewObject<UTextBlock>(this);
			if (Fallback)
			{
				Fallback->SetText(FText::FromString(TEXT("•")));
				Fallback->SetVisibility(ESlateVisibility::HitTestInvisible);
				IndicatorBox->AddChild(Fallback);
			}
		}

		if (UHorizontalBoxSlot* IndicatorSlot = HorizontalBox_StatusEffects->AddChildToHorizontalBox(IndicatorBox))
		{
			IndicatorSlot->SetPadding(FMargin(1.0f, 0.0f));
		}
	}

	if (bHasOverflow)
	{
		USizeBox* OverflowBox = CreateSizeBox();
		if (OverflowBox)
		{
			UTextBlock* OverflowText =
				WidgetTree ? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()) : NewObject<UTextBlock>(this);
			if (OverflowText)
			{
				OverflowText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), CachedStatusEffects.Num() - DirectIndicatorCount)));
				OverflowText->SetVisibility(ESlateVisibility::HitTestInvisible);
				OverflowBox->AddChild(OverflowText);
			}

			TArray<FText> OverflowToolTips;
			for (int32 Index = DirectIndicatorCount; Index < CachedStatusEffects.Num(); ++Index)
			{
				if (!CachedStatusEffects[Index].ToolTipText.IsEmpty())
				{
					OverflowToolTips.Add(CachedStatusEffects[Index].ToolTipText);
				}
			}
			OverflowBox->SetToolTipText(FText::Join(FText::FromString(TEXT("\n\n")), OverflowToolTips));
			if (UHorizontalBoxSlot* OverflowSlot = HorizontalBox_StatusEffects->AddChildToHorizontalBox(OverflowBox))
			{
				OverflowSlot->SetPadding(FMargin(1.0f, 0.0f));
			}
		}
	}

	// The row itself never blocks portrait interaction; individual size boxes
	// remain available for standard UMG tooltip hit testing.
	HorizontalBox_StatusEffects->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}
