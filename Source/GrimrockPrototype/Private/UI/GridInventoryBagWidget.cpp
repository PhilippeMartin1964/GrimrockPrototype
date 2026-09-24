#include "UI/GridInventoryBagWidget.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"

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
	InitializeInventorySortComboBox();
	if (ComboBox_SortInventory)
	{
		ComboBox_SortInventory->OnSelectionChanged.AddUniqueDynamic(this, &UGridInventoryBagWidget::HandleSortInventorySelectionChanged);
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
	if (ComboBox_SortInventory)
	{
		ComboBox_SortInventory->OnSelectionChanged.RemoveDynamic(this, &UGridInventoryBagWidget::HandleSortInventorySelectionChanged);
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

void UGridInventoryBagWidget::HandleSortInventorySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	(void)SelectionType;

	const EGridInventorySortMode SortModes[] = {
		EGridInventorySortMode::NameAscending,
		EGridInventorySortMode::NameDescending,
		EGridInventorySortMode::TypeAscending,
		EGridInventorySortMode::TypeDescending,
		EGridInventorySortMode::WeightAscending,
		EGridInventorySortMode::WeightDescending
	};

	for (const EGridInventorySortMode SortMode : SortModes)
	{
		if (SelectedItem == GetGridInventorySortModeDisplayName(SortMode).ToString())
		{
			SetInventorySortMode(SortMode);
			return;
		}
	}
}

void UGridInventoryBagWidget::InitializeInventorySortComboBox()
{
	if (!ComboBox_SortInventory)
	{
		return;
	}

	ComboBox_SortInventory->ClearOptions();
	ComboBox_SortInventory->AddOption(GetGridInventorySortModeDisplayName(EGridInventorySortMode::NameAscending).ToString());
	ComboBox_SortInventory->AddOption(GetGridInventorySortModeDisplayName(EGridInventorySortMode::NameDescending).ToString());
	ComboBox_SortInventory->AddOption(GetGridInventorySortModeDisplayName(EGridInventorySortMode::TypeAscending).ToString());
	ComboBox_SortInventory->AddOption(GetGridInventorySortModeDisplayName(EGridInventorySortMode::TypeDescending).ToString());
	ComboBox_SortInventory->AddOption(GetGridInventorySortModeDisplayName(EGridInventorySortMode::WeightAscending).ToString());
	ComboBox_SortInventory->AddOption(GetGridInventorySortModeDisplayName(EGridInventorySortMode::WeightDescending).ToString());
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
	if (ComboBox_SortInventory)
	{
		const FString SelectedOption = GetGridInventorySortModeDisplayName(InventorySortMode).ToString();
		if (ComboBox_SortInventory->GetSelectedOption() != SelectedOption)
		{
			ComboBox_SortInventory->SetSelectedOption(SelectedOption);
		}
	}
}
