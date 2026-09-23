#include "UI/GridInventoryBagWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UGridInventoryBagWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_FilterAll)
	{
		Button_FilterAll->OnClicked.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleFilterAllClicked);
	}
	if (Button_FilterEquipment)
	{
		Button_FilterEquipment->OnClicked.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleFilterEquipmentClicked);
	}
	if (Button_FilterConsumables)
	{
		Button_FilterConsumables->OnClicked.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleFilterConsumablesClicked);
	}
	if (Button_FilterMagic)
	{
		Button_FilterMagic->OnClicked.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleFilterMagicClicked);
	}
	if (Button_FilterIngredients)
	{
		Button_FilterIngredients->OnClicked.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleFilterIngredientsClicked);
	}
	if (Button_FilterBooksAndKeys)
	{
		Button_FilterBooksAndKeys->OnClicked.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleFilterBooksAndKeysClicked);
	}
	if (Button_FilterMisc)
	{
		Button_FilterMisc->OnClicked.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleFilterMiscClicked);
	}
	if (Button_SortInventory)
	{
		Button_SortInventory->OnClicked.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleSortInventoryClicked);
	}

	RefreshInventoryFilterSelectionFrames();
	RefreshInventorySortPresentation();
}

void UGridInventoryBagWidget::NativeDestruct()
{
	if (Button_FilterAll)
	{
		Button_FilterAll->OnClicked.RemoveDynamic(this, &UGridInventoryBagWidget::HandleFilterAllClicked);
	}
	if (Button_FilterEquipment)
	{
		Button_FilterEquipment->OnClicked.RemoveDynamic(this, &UGridInventoryBagWidget::HandleFilterEquipmentClicked);
	}
	if (Button_FilterConsumables)
	{
		Button_FilterConsumables->OnClicked.RemoveDynamic(this, &UGridInventoryBagWidget::HandleFilterConsumablesClicked);
	}
	if (Button_FilterMagic)
	{
		Button_FilterMagic->OnClicked.RemoveDynamic(this, &UGridInventoryBagWidget::HandleFilterMagicClicked);
	}
	if (Button_FilterIngredients)
	{
		Button_FilterIngredients->OnClicked.RemoveDynamic(this, &UGridInventoryBagWidget::HandleFilterIngredientsClicked);
	}
	if (Button_FilterBooksAndKeys)
	{
		Button_FilterBooksAndKeys->OnClicked.RemoveDynamic(this, &UGridInventoryBagWidget::HandleFilterBooksAndKeysClicked);
	}
	if (Button_FilterMisc)
	{
		Button_FilterMisc->OnClicked.RemoveDynamic(this, &UGridInventoryBagWidget::HandleFilterMiscClicked);
	}
	if (Button_SortInventory)
	{
		Button_SortInventory->OnClicked.RemoveDynamic(this, &UGridInventoryBagWidget::HandleSortInventoryClicked);
	}

	Super::NativeDestruct();
}

void UGridInventoryBagWidget::HandleFilterAllClicked()
{
	SetInventoryFilterCategory(EGridInventoryFilterCategory::All);
}

void UGridInventoryBagWidget::HandleFilterEquipmentClicked()
{
	SetInventoryFilterCategory(EGridInventoryFilterCategory::Equipment);
}

void UGridInventoryBagWidget::HandleFilterConsumablesClicked()
{
	SetInventoryFilterCategory(EGridInventoryFilterCategory::Consumables);
}

void UGridInventoryBagWidget::HandleFilterMagicClicked()
{
	SetInventoryFilterCategory(EGridInventoryFilterCategory::Magic);
}

void UGridInventoryBagWidget::HandleFilterIngredientsClicked()
{
	SetInventoryFilterCategory(EGridInventoryFilterCategory::Ingredients);
}

void UGridInventoryBagWidget::HandleFilterBooksAndKeysClicked()
{
	SetInventoryFilterCategory(EGridInventoryFilterCategory::BooksAndKeys);
}

void UGridInventoryBagWidget::HandleFilterMiscClicked()
{
	SetInventoryFilterCategory(EGridInventoryFilterCategory::Misc);
}

void UGridInventoryBagWidget::HandleSortInventoryClicked()
{
	CycleInventorySortMode();
}

void UGridInventoryBagWidget::HandleInventoryFilterCategoryChanged()
{
	RefreshInventoryFilterSelectionFrames();
}

void UGridInventoryBagWidget::HandleInventorySortModeChanged()
{
	RefreshInventorySortPresentation();
}

void UGridInventoryBagWidget::RefreshInventoryFilterSelectionFrames()
{
	auto SetFrameSelected = [](UImage* Frame, bool bSelected)
	{
		if (Frame)
		{
			Frame->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	};

	SetFrameSelected(Image_FilterAllSelectionFrame, InventoryFilterCategory == EGridInventoryFilterCategory::All);
	SetFrameSelected(Image_FilterEquipmentSelectionFrame, InventoryFilterCategory == EGridInventoryFilterCategory::Equipment);
	SetFrameSelected(Image_FilterConsumablesSelectionFrame, InventoryFilterCategory == EGridInventoryFilterCategory::Consumables);
	SetFrameSelected(Image_FilterMagicSelectionFrame, InventoryFilterCategory == EGridInventoryFilterCategory::Magic);
	SetFrameSelected(Image_FilterIngredientsSelectionFrame, InventoryFilterCategory == EGridInventoryFilterCategory::Ingredients);
	SetFrameSelected(Image_FilterBooksAndKeysSelectionFrame, InventoryFilterCategory == EGridInventoryFilterCategory::BooksAndKeys);
	SetFrameSelected(Image_FilterMiscSelectionFrame, InventoryFilterCategory == EGridInventoryFilterCategory::Misc);
}

void UGridInventoryBagWidget::RefreshInventorySortPresentation()
{
	if (Text_SortInventory)
	{
		Text_SortInventory->SetText(FText::FromString(
			FString::Printf(TEXT("Tri : %s"), *GetGridInventorySortModeDisplayName(InventorySortMode).ToString())));
	}
}
