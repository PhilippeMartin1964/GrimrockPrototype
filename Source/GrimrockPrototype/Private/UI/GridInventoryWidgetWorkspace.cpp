#include "UI/GridInventoryWidget.h"

#include "Components/Button.h"
#include "Components/Widget.h"

void UGridInventoryWidget::ResetInventoryWorkspace()
{
	bCharacterSheetPanelVisible = true;
	bInventoryBagPanelVisible = true;
	ApplyWorkspacePanelVisibility();
}

void UGridInventoryWidget::SetCharacterSheetPanelVisible(bool bVisible)
{
	bCharacterSheetPanelVisible = bVisible;
	ApplyWorkspacePanelVisibility();
}

void UGridInventoryWidget::SetInventoryBagPanelVisible(bool bVisible)
{
	bInventoryBagPanelVisible = bVisible;
	ApplyWorkspacePanelVisibility();
}

void UGridInventoryWidget::BindWorkspaceButtons()
{
	if (Button_CloseCharacterSheet)
	{
		Button_CloseCharacterSheet->OnClicked.RemoveDynamic(this, &UGridInventoryWidget::HandleCloseCharacterSheetClicked);
		Button_CloseCharacterSheet->OnClicked.AddDynamic(this, &UGridInventoryWidget::HandleCloseCharacterSheetClicked);
	}

	if (Button_CloseInventoryBag)
	{
		Button_CloseInventoryBag->OnClicked.RemoveDynamic(this, &UGridInventoryWidget::HandleCloseInventoryBagClicked);
		Button_CloseInventoryBag->OnClicked.AddDynamic(this, &UGridInventoryWidget::HandleCloseInventoryBagClicked);
	}
}

void UGridInventoryWidget::ApplyWorkspacePanelVisibility()
{
	if (Panel_CharacterSheet)
	{
		Panel_CharacterSheet->SetVisibility(bCharacterSheetPanelVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (Panel_InventoryBag)
	{
		Panel_InventoryBag->SetVisibility(bInventoryBagPanelVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UGridInventoryWidget::HandleCloseCharacterSheetClicked()
{
	SetCharacterSheetPanelVisible(false);
}

void UGridInventoryWidget::HandleCloseInventoryBagClicked()
{
	SetInventoryBagPanelVisible(false);
}
