#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "GridTalentVariantBlockWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentVariantChooseRequestedSignature, FName, ChoiceId);

/**
 * Presentation-only block for one concrete Talent variant.
 * All player-facing content is supplied by FGridTalentVariantView.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridTalentVariantBlockWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	FGridTalentVariantView VariantView;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	FText ResolvedDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	FText ResolvedTypeText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	FText ResolvedStatusText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	FText ResolvedPrincipleText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	FText ResolvedEffectsText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	FText ResolvedUsageText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	FText ResolvedChooseLabel;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	bool bChooseEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	bool bPendingChoice = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Variant")
	bool bInitialized = false;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Variant|Events")
	FGridTalentVariantChooseRequestedSignature OnChooseRequested;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Variant")
	bool InitializeVariant(
		const FGridTalentVariantView& InVariant,
		const FText& InDisplayName,
		bool bInPendingChoice);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Variant")
	void ClearVariant();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleChooseClicked();

	void BindChooseButton();
	void UnbindChooseButton();
	void ApplyPresentation();
	static FText FormatDetailLines(const TArray<FGridTalentDetailLineView>& Lines);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_VariantName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_VariantType;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_VariantStatus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_VariantPrinciple;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_VariantEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_VariantEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_VariantUsage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_VariantUsage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ChooseVariant;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ChooseVariant;
};
