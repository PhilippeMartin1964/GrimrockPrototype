#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrimrockMainMenuWidget.generated.h"

class UButton;

/**
 * Main title-screen menu widget.
 *
 * The main menu stays a pure menu. It can open secondary modal widgets and
 * request map transitions, but it does not load saves or create gameplay actors
 * directly.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Main Menu")
    void SetHasValidSaveGame(bool bInHasValidSaveGame);

    UFUNCTION(BlueprintCallable, Category = "Main Menu")
    bool HasValidSaveGame() const;

    UFUNCTION(BlueprintCallable, Category = "Main Menu")
    void RefreshButtonStates();

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Modal")
    bool OpenOptionsMenu();

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Modal")
    bool OpenCreditsMenu();

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Modal")
    bool OpenLicenseMenu();

    UFUNCTION(BlueprintCallable, Category = "Main Menu")
    void QuitMainMenu();

protected:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Actions")
    void OnContinueRequested();

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Actions")
    void OnNewGameRequested();

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Actions")
    void OnLoadGameRequested();

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Actions")
    void OnOptionsRequested();

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Actions")
    void OnCreditsRequested();

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Actions")
    void OnLicenseRequested();

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu|Actions")
    void OnQuitRequested();

private:
    void BindMainMenuButtons();
    bool OpenMainMenuModal(TSubclassOf<UUserWidget> WidgetClass, const TCHAR* MissingClassReason);

    UFUNCTION()
    void HandleContinueClicked();

    UFUNCTION()
    void HandleNewGameClicked();

    UFUNCTION()
    void HandleLoadGameClicked();

    UFUNCTION()
    void HandleOptionsClicked();

    UFUNCTION()
    void HandleCreditsClicked();

    UFUNCTION()
    void HandleLicenseClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu", meta = (AllowPrivateAccess = "true"))
    bool bHasValidSaveGame = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu|Modal", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> OptionsMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu|Modal", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> CreditsMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu|Modal", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UUserWidget> LicenseMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu|Modal", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
    int32 ModalZOrder = 200;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu", meta = (AllowPrivateAccess = "true"))
    bool bQuitDirectlyFromMainMenu = true;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Continue;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_NewGame;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_LoadGame;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Options;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Credits;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_License;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Quit;
};