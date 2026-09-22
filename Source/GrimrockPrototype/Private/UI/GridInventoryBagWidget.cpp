#include "UI/GridInventoryBagWidget.h"

#include "Components/Button.h"

namespace
{
	void SetFilterButtonSelected(UButton* Button, bool bSelected)
	{
		if (Button)
		{
			// The selected filter is intentionally not clickable. Its visual comes
			// from the Button Disabled style authored in UMG, not from C++ colors.
			Button->SetIsEnabled(!bSelected);
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

	RefreshInventoryFilterButtonState();
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

void UGridInventoryBagWidget::HandleInventoryFilterCategoryChanged()
{
	RefreshInventoryFilterButtonState();
}

void UGridInventoryBagWidget::RefreshInventoryFilterButtonState()
{
	SetFilterButtonSelected(Button_FilterAll, InventoryFilterCategory == EGridInventoryFilterCategory::All);
	SetFilterButtonSelected(Button_FilterEquipment, InventoryFilterCategory == EGridInventoryFilterCategory::Equipment);
	SetFilterButtonSelected(Button_FilterConsumables, InventoryFilterCategory == EGridInventoryFilterCategory::Consumables);
	SetFilterButtonSelected(Button_FilterMagic, InventoryFilterCategory == EGridInventoryFilterCategory::Magic);
	SetFilterButtonSelected(Button_FilterIngredients, InventoryFilterCategory == EGridInventoryFilterCategory::Ingredients);
	SetFilterButtonSelected(Button_FilterBooksAndKeys, InventoryFilterCategory == EGridInventoryFilterCategory::BooksAndKeys);
	SetFilterButtonSelected(Button_FilterMisc, InventoryFilterCategory == EGridInventoryFilterCategory::Misc);
}
