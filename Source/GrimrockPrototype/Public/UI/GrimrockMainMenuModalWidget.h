#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrimrockMainMenuModalWidget.generated.h"

class UButton;

/**
 * Simple modal parent for secondary main-menu screens.
 *
 * WBP_OptionsMenu, WBP_CreditsMenu and WBP_LicenseMenu can derive from this
 * class and only need a Button_Back widget to close themselves.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockMainMenuModalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Main Menu|Modal")
	void CloseModal();

protected:
	virtual void NativeConstruct() override;

private:
	void BindButtons();

	UFUNCTION()
	void HandleBackClicked();

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Back;
};
