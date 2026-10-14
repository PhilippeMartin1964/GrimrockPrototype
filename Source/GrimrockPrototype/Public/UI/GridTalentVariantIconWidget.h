#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GridTalentVariantIconWidget.generated.h"

class UButton;
class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentVariantIconClickedSignature, FName, ChoiceId);

UCLASS()
class GRIMROCKPROTOTYPE_API UGridTalentVariantIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant Flyout")
	FName ChoiceId = NAME_None;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Variant Flyout|Events")
	FGridTalentVariantIconClickedSignature OnVariantIconClicked;

	bool InitializeVariantIcon(FName InChoiceId, UTexture2D* IconTexture);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleVariantClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Variant;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_VariantIcon;
};
