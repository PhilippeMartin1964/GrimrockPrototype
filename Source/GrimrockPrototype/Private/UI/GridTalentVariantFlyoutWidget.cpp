#include "UI/GridTalentVariantFlyoutWidget.h"

#include "Components/UniformGridPanel.h"
#include "Engine/Texture2D.h"
#include "UI/GridTalentVariantIconWidget.h"

int32 UGridTalentVariantFlyoutWidget::ResolveColumnCount(int32 VariantCount)
{
	if (VariantCount <= 2)
	{
		return FMath::Max(VariantCount, 1);
	}
	if (VariantCount <= 4)
	{
		return 2;
	}
	return 3;
}

bool UGridTalentVariantFlyoutWidget::InitializeFlyout(
	const FGridTalentNodeView& NodeView,
	const TMap<FName, TObjectPtr<UTexture2D>>& VariantIcons)
{
	if (!NodeView.bHasExclusiveVariants || NodeView.Variants.Num() < 2 ||
		!NodeView.SelectedChoiceId.IsNone() || !Grid_Variants || !VariantIconWidgetClass || !GetWorld())
	{
		return false;
	}

	TalentNodeId = NodeView.TalentNodeId;
	Grid_Variants->ClearChildren();

	const int32 ColumnCount = ResolveColumnCount(NodeView.Variants.Num());
	for (int32 Index = 0; Index < NodeView.Variants.Num(); ++Index)
	{
		const FGridTalentVariantView& Variant = NodeView.Variants[Index];
		if (Variant.ChoiceId.IsNone())
		{
			continue;
		}

		UGridTalentVariantIconWidget* Entry =
			CreateWidget<UGridTalentVariantIconWidget>(GetWorld(), VariantIconWidgetClass);
		if (!Entry)
		{
			continue;
		}

		UTexture2D* IconTexture = nullptr;
		if (const TObjectPtr<UTexture2D>* FoundIcon = VariantIcons.Find(Variant.ChoiceId))
		{
			IconTexture = FoundIcon->Get();
		}
		if (!Entry->InitializeVariantIcon(Variant.ChoiceId, IconTexture))
		{
			continue;
		}

		Entry->OnVariantIconClicked.AddUniqueDynamic(
			this, &UGridTalentVariantFlyoutWidget::HandleVariantIconClicked);
		Grid_Variants->AddChildToUniformGrid(Entry, Index / ColumnCount, Index % ColumnCount);
	}

	return Grid_Variants->GetChildrenCount() == NodeView.Variants.Num();
}

void UGridTalentVariantFlyoutWidget::HandleVariantIconClicked(FName ChoiceId)
{
	if (!ChoiceId.IsNone())
	{
		OnVariantChosen.Broadcast(ChoiceId);
	}
}
