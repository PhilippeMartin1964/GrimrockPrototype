#include "UI/GrimrockMainMenuWidget.h"

#include "Components/Button.h"

void UGrimrockMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindMainMenuButtons();
    RefreshButtonStates();
}

void UGrimrockMainMenuWidget::SetHasValidSaveGame(bool bInHasValidSaveGame)
{
    bHasValidSaveGame = bInHasValidSaveGame;
    RefreshButtonStates();
}

bool UGrimrockMainMenuWidget::HasValidSaveGame() const
{
    return bHasValidSaveGame;
}

void UGrimrockMainMenuWidget::RefreshButtonStates()
{
    if (Button_Continue)
    {
        Button_Continue->SetIsEnabled(bHasValidSaveGame);
    }

    if (Button_LoadGame)
    {
        Button_LoadGame->SetIsEnabled(bHasValidSaveGame);
    }
}

void UGrimrockMainMenuWidget::BindMainMenuButtons()
{
    if (Button_Continue)
    {
        Button_Continue->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleContinueClicked);
        Button_Continue->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleContinueClicked);
    }

    if (Button_NewGame)
    {
        Button_NewGame->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleNewGameClicked);
        Button_NewGame->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleNewGameClicked);
    }

    if (Button_LoadGame)
    {
        Button_LoadGame->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleLoadGameClicked);
        Button_LoadGame->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleLoadGameClicked);
    }

    if (Button_Options)
    {
        Button_Options->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleOptionsClicked);
        Button_Options->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleOptionsClicked);
    }

    if (Button_Credits)
    {
        Button_Credits->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleCreditsClicked);
        Button_Credits->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleCreditsClicked);
    }

    if (Button_License)
    {
        Button_License->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleLicenseClicked);
        Button_License->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleLicenseClicked);
    }

    if (Button_Quit)
    {
        Button_Quit->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleQuitClicked);
        Button_Quit->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleQuitClicked);
    }
}

void UGrimrockMainMenuWidget::HandleContinueClicked()
{
    if (!bHasValidSaveGame)
    {
        return;
    }

    OnContinueRequested();
}

void UGrimrockMainMenuWidget::HandleNewGameClicked()
{
    OnNewGameRequested();
}

void UGrimrockMainMenuWidget::HandleLoadGameClicked()
{
    if (!bHasValidSaveGame)
    {
        return;
    }

    OnLoadGameRequested();
}

void UGrimrockMainMenuWidget::HandleOptionsClicked()
{
    OnOptionsRequested();
}

void UGrimrockMainMenuWidget::HandleCreditsClicked()
{
    OnCreditsRequested();
}

void UGrimrockMainMenuWidget::HandleLicenseClicked()
{
    OnLicenseRequested();
}

void UGrimrockMainMenuWidget::HandleQuitClicked()
{
    OnQuitRequested();
}
