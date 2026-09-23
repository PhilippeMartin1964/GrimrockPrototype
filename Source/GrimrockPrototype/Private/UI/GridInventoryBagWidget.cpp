#include "UI/GridInventoryBagWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
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

	FilterSelectionFrameAll = CreateFilterSelectionFrame(Button_FilterAll, TEXT("Overlay_FilterAllSelection"), TEXT("Image_FilterAllSelectionFrame"));
	FilterSelectionFrameEquipment =
		CreateFilterSelectionFrame(Button_FilterEquipment, TEXT("Overlay_FilterEquipmentSelection"), TEXT("Image_FilterEquipmentSelectionFrame"));
	FilterSelectionFrameConsumables =
		CreateFilterSelectionFrame(Button_FilterConsumables, TEXT("Overlay_FilterConsumablesSelection"), TEXT("Image_FilterConsumablesSelectionFrame"));
	FilterSelectionFrameMagic =
		CreateFilterSelectionFrame(Button_FilterMagic, TEXT("Overlay_FilterMagicSelection"), TEXT("Image_FilterMagicSelectionFrame"));
	FilterSelectionFrameIngredients =
		CreateFilterSelectionFrame(Button_FilterIngredients, TEXT("Overlay_FilterIngredientsSelection"), TEXT("Image_FilterIngredientsSelectionFrame"));
	FilterSelectionFrameBooksAndKeys =
		CreateFilterSelectionFrame(Button_FilterBooksAndKeys, TEXT("Overlay_FilterBooksAndKeysSelection"), TEXT("Image_FilterBooksAndKeysSelectionFrame"));
	FilterSelectionFrameMisc =
		CreateFilterSelectionFrame(Button_FilterMisc, TEXT("Overlay_FilterMiscSelection"), TEXT("Image_FilterMiscSelectionFrame"));

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

UImage* UGridInventoryBagWidget::CreateFilterSelectionFrame(UButton* Button, FName OverlayName, FName FrameName)
{
	if (!Button || !WidgetTree)
	{
		return nullptr;
	}

	if (UImage* ExistingFrame = Cast<UImage>(WidgetTree->FindWidget(FrameName)))
	{
		return ExistingFrame;
	}

	UPanelWidget* Parent = Button->GetParent();
	if (!Parent)
	{
		return nullptr;
	}

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), OverlayName);
	UImage* Frame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FrameName);
	if (!Overlay || !Frame || !Parent->ReplaceChild(Button, Overlay))
	{
		return nullptr;
	}

	if (UOverlaySlot* ButtonSlot = Overlay->AddChildToOverlay(Button))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetVerticalAlignment(VAlign_Fill);
	}

	const float SafeThickness = FMath::Max(0.0f, SelectedFilterFrameThickness);
	const FSlateRoundedBoxBrush FrameBrush(
		FLinearColor::Transparent,
		0.0f,
		SelectedFilterFrameColor,
		SafeThickness,
		FVector2D(1.0f, 1.0f));
	Frame->SetBrush(FrameBrush);
	Frame->SetVisibility(ESlateVisibility::Collapsed);

	if (UOverlaySlot* FrameSlot = Overlay->AddChildToOverlay(Frame))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	return Frame;
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

	SetFrameSelected(FilterSelectionFrameAll, InventoryFilterCategory == EGridInventoryFilterCategory::All);
	SetFrameSelected(FilterSelectionFrameEquipment, InventoryFilterCategory == EGridInventoryFilterCategory::Equipment);
	SetFrameSelected(FilterSelectionFrameConsumables, InventoryFilterCategory == EGridInventoryFilterCategory::Consumables);
	SetFrameSelected(FilterSelectionFrameMagic, InventoryFilterCategory == EGridInventoryFilterCategory::Magic);
	SetFrameSelected(FilterSelectionFrameIngredients, InventoryFilterCategory == EGridInventoryFilterCategory::Ingredients);
	SetFrameSelected(FilterSelectionFrameBooksAndKeys, InventoryFilterCategory == EGridInventoryFilterCategory::BooksAndKeys);
	SetFrameSelected(FilterSelectionFrameMisc, InventoryFilterCategory == EGridInventoryFilterCategory::Misc);
}

void UGridInventoryBagWidget::RefreshInventorySortPresentation()
{
	if (Text_SortInventory)
	{
		Text_SortInventory->SetText(FText::FromString(
			FString::Printf(TEXT("Tri : %s"), *GetGridInventorySortModeDisplayName(InventorySortMode).ToString())));
	}
}
