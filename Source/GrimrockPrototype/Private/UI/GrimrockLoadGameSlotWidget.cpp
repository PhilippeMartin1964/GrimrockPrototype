#include "UI/GrimrockLoadGameSlotWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UGrimrockLoadGameSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindSlotButton();
	RefreshSlotText();
}

void UGrimrockLoadGameSlotWidget::InitializeSaveSlot(const FGrimrockSaveSlotInfo& InSaveSlotInfo)
{
	SaveSlotInfo = InSaveSlotInfo;
	RefreshSlotText();
}

FGrimrockSaveSlotInfo UGrimrockLoadGameSlotWidget::GetSaveSlotInfo() const
{
	return SaveSlotInfo;
}

void UGrimrockLoadGameSlotWidget::BindSlotButton()
{
	if (!Button_LoadSlot)
	{
		return;
	}

	Button_LoadSlot->OnClicked.RemoveDynamic(this, &UGrimrockLoadGameSlotWidget::HandleLoadSlotClicked);
	Button_LoadSlot->OnClicked.AddDynamic(this, &UGrimrockLoadGameSlotWidget::HandleLoadSlotClicked);
}

void UGrimrockLoadGameSlotWidget::RefreshSlotText()
{
	if (Text_DisplayName)
	{
		Text_DisplayName->SetText(SaveSlotInfo.DisplayName);
	}

	if (Text_SlotName)
	{
		Text_SlotName->SetText(FText::FromString(SaveSlotInfo.SlotName));
	}

	if (Text_Status)
	{
		Text_Status->SetText(SaveSlotInfo.bExists ? FText::FromString(TEXT("Disponible")) : FText::FromString(TEXT("Indisponible")));
	}

	if (Button_LoadSlot)
	{
		Button_LoadSlot->SetIsEnabled(SaveSlotInfo.bExists);
	}
}

void UGrimrockLoadGameSlotWidget::HandleLoadSlotClicked()
{
	if (!SaveSlotInfo.bExists)
	{
		return;
	}

	OnSaveSlotSelected.Broadcast(SaveSlotInfo.SlotName, SaveSlotInfo.UserIndex);
}
