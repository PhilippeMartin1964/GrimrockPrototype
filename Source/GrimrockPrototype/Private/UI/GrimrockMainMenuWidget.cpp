#include "UI/GrimrockMainMenuWidget.h"

#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

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

bool UGrimrockMainMenuWidget::OpenOptionsMenu()
{
    return OpenMainMenuModal(OptionsMenuWidgetClass, TEXT("NoOptionsMenuWidgetClass"));
}

bool UGrimrockMainMenuWidget::OpenCreditsMenu()
{
    return OpenMainMenuModal(CreditsMenuWidgetClass, TEXT("NoCreditsMenuWidgetClass"));
}

bool UGrimrockMainMenuWidget::OpenLicenseMenu()
{
    return OpenMainMenuModal(LicenseMenuWidgetClass, TEXT("NoLicenseMenuWidgetClass"));
}

void UGrimrockMainMenuWidget::QuitMainMenu()
{
    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController && GetWorld())
    {
        PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    }

    UE_LOG(LogTemp, Log, TEXT("MainMenu Quit Requested Widget=%s"), *GetName());
    UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
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

bool UGrimrockMainMenuWidget::OpenMainMenuModal(TSubclassOf<UUserWidget> WidgetClass, const TCHAR* MissingClassReason)
{
    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("MainMenu Modal Open Failed Widget=%s Reason=%s"), *GetName(), MissingClassReason);
        return false;
    }

    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController && GetWorld())
    {
        PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    }

    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("MainMenu Modal Open Failed Widget=%s Reason=NoPlayerController"), *GetName());
        return false;
    }

    UUserWidget* ModalWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
    if (!ModalWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("MainMenu Modal Open Failed Widget=%s Reason=CreateWidgetFailed Class=%s"), *GetName(), *GetNameSafe(WidgetClass));
        return false;
    }

    ModalWidget->AddToViewport(ModalZOrder);
    ModalWidget->SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Log, TEXT("MainMenu Modal Opened Widget=%s Class=%s ZOrder=%d"), *GetName(), *GetNameSafe(WidgetClass), ModalZOrder);
    return true;
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
    if (OpenOptionsMenu())
    {
        return;
    }

    OnOptionsRequested();
}

void UGrimrockMainMenuWidget::HandleCreditsClicked()
{
    if (OpenCreditsMenu())
    {
        return;
    }

    OnCreditsRequested();
}

void UGrimrockMainMenuWidget::HandleLicenseClicked()
{
    if (OpenLicenseMenu())
    {
        return;
    }

    OnLicenseRequested();
}

void UGrimrockMainMenuWidget::HandleQuitClicked()
{
    if (bQuitDirectlyFromMainMenu)
    {
        QuitMainMenu();
        return;
    }

    OnQuitRequested();
}