#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GridNavigationTooltipWidget.generated.h"

class UTextBlock;

/** Presentation-only compact text tooltip shared by persistent HUD hover surfaces. */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridNavigationTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Persistent|Navigation")
	FText NavigationTooltipText;

	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent|Navigation")
	void InitializeTooltip(const FText& InText);

protected:
	virtual void NativeConstruct() override;

private:
	void ApplyTooltipPresentation();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Tooltip;
};
