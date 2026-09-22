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
	RefreshInventoryFilterButtonState();
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
