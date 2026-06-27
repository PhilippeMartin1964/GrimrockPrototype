#include "UI/GrimrockMainMenuModalWidget.h"

#include "Components/Button.h"

void UGrimrockMainMenuModalWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindButtons();
}

void UGrimrockMainMenuModalWidget::CloseModal()
{
    RemoveFromParent();
}

void UGrimrockMainMenuModalWidget::BindButtons()
{
    if (!Button_Back)
    {
        return;
    }

    Button_Back->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuModalWidget::HandleBackClicked);
    Button_Back->OnClicked.AddDynamic(this, &UGrimrockMainMenuModalWidget::HandleBackClicked);
}

void UGrimrockMainMenuModalWidget::HandleBackClicked()
{
    CloseModal();
}
