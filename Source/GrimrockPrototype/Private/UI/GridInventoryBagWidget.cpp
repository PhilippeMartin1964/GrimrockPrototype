#include "UI/GridInventoryBagWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

namespace
{
	void SetFilterButtonSelected(UButton* Button, bool bSelected, const FLinearColor& UnselectedBackgroundColor,
		const FLinearColor& SelectedBackgroundColor)
	{
		if (Button)
		{
			// Selection is presentation state, not disabled interaction state.
			// Keeping the button enabled preserves Hovered/Pressed feedback and
			// avoids Slate's disabled greying pass.
			Button->SetIsEnabled(true);
			Button->SetBackgroundColor(bSelected ? SelectedBackgroundColor : UnselectedBackgroundColor);
		}
	}
}

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

	RefreshInventoryFilterButtonState();
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
	RefreshInventoryFilterButtonState();
}

void UGridInventoryBagWidget::HandleInventorySortModeChanged()
{
	RefreshInventorySortPresentation();
}

void UGridInventoryBagWidget::RefreshInventoryFilterButtonState()
{
	SetFilterButtonSelected(Button_FilterAll, InventoryFilterCategory == EGridInventoryFilterCategory::All,
		UnselectedFilterBackgroundColor, SelectedFilterBackgroundColor);
	SetFilterButtonSelected(Button_FilterEquipment, InventoryFilterCategory == EGridInventoryFilterCategory::Equipment,
		UnselectedFilterBackgroundColor, SelectedFilterBackgroundColor);
	SetFilterButtonSelected(Button_FilterConsumables, InventoryFilterCategory == EGridInventoryFilterCategory::Consumables,
		UnselectedFilterBackgroundColor, SelectedFilterBackgroundColor);
	SetFilterButtonSelected(Button_FilterMagic, InventoryFilterCategory == EGridInventoryFilterCategory::Magic,
		UnselectedFilterBackgroundColor, SelectedFilterBackgroundColor);
	SetFilterButtonSelected(Button_FilterIngredients, InventoryFilterCategory == EGridInventoryFilterCategory::Ingredients,
		UnselectedFilterBackgroundColor, SelectedFilterBackgroundColor);
	SetFilterButtonSelected(Button_FilterBooksAndKeys, InventoryFilterCategory == EGridInventoryFilterCategory::BooksAndKeys,
		UnselectedFilterBackgroundColor, SelectedFilterBackgroundColor);
	SetFilterButtonSelected(Button_FilterMisc, InventoryFilterCategory == EGridInventoryFilterCategory::Misc,
		UnselectedFilterBackgroundColor, SelectedFilterBackgroundColor);
}


void UGridInventoryBagWidget::RefreshInventorySortPresentation()
{
	if (Text_SortInventory)
	{
		Text_SortInventory->SetText(FText::FromString(
			FString::Printf(TEXT("Tri : %s"), *GetGridInventorySortModeDisplayName(InventorySortMode).ToString())));
	}
}
