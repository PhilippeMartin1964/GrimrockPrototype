#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrimrockMainMenuWidget.generated.h"

class UButton;

/**
 * Main title-screen menu widget.
 *
 * MM1 scope: visual shell and safe button hooks only.
 * This widget does not load saves, create characters, or open the CC7 wizard directly.
 * Blueprint or later C++ flow code can react to the requested actions.
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
