#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "GridTalentVariantFlyoutWidget.generated.h"

class UGridTalentVariantIconWidget;
class UTexture2D;
class UUniformGridPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentVariantFlyoutChosenSignature, FName, ChoiceId);

UCLASS()
class GRIMROCKPROTOTYPE_API UGridTalentVariantFlyoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant Flyout")
	FName TalentNodeId = NAME_None;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Variant Flyout|Events")
	FGridTalentVariantFlyoutChosenSignature OnVariantChosen;

	bool InitializeFlyout(
		const FGridTalentNodeView& NodeView,
		const TMap<FName, TObjectPtr<UTexture2D>>& VariantIcons);

private:
	UFUNCTION()
	void HandleVariantIconClicked(FName ChoiceId);

	static int32 ResolveColumnCount(int32 VariantCount);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> Grid_Variants;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RPG|Talents|Variant Flyout", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGridTalentVariantIconWidget> VariantIconWidgetClass;
};
