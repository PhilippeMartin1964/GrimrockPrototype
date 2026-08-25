#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RPG/RPGCharacterTypes.h"
#include "RPGCharacterCreationAttributesStepWidget.generated.h"

class UButton;
class UTextBlock;
class URPGCharacterCreationWizardWidget;

/**
 * Dedicated widget for the Attributes step of the character creation wizard.
 *
 * The wizard remains the owner of the creation state and gameplay rules.
 * This widget owns only the UMG controls of the Attributes screen.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API URPGCharacterCreationAttributesStepWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RPG|Character Creation|Attributes")
	void InitializeAttributesStep(URPGCharacterCreationWizardWidget* InOwningWizard);

	UFUNCTION(BlueprintCallable, Category = "RPG|Character Creation|Attributes")
	void RefreshFromWizardState();

protected:
	virtual void NativeConstruct() override;

private:
	void BindAttributeButtons();
	URPGCharacterCreationWizardWidget* ResolveOwningWizardFromOuterChain() const;
	URPGCharacterCreationWizardWidget* GetResolvedWizard();
	void RefreshAttributeAllocationPreview();
	void RefreshAttributeAllocationControls();

	UFUNCTION()
	void HandleResetRecommendedAttributesClicked();

	UFUNCTION()
	void HandleStrengthMinusClicked();

	UFUNCTION()
	void HandleStrengthPlusClicked();

	UFUNCTION()
	void HandleDexterityMinusClicked();

	UFUNCTION()
	void HandleDexterityPlusClicked();

	UFUNCTION()
	void HandleConstitutionMinusClicked();

	UFUNCTION()
	void HandleConstitutionPlusClicked();

	UFUNCTION()
	void HandleIntelligenceMinusClicked();

	UFUNCTION()
	void HandleIntelligencePlusClicked();

	UFUNCTION()
	void HandleWisdomMinusClicked();

	UFUNCTION()
	void HandleWisdomPlusClicked();

	UFUNCTION()
	void HandleCharismaMinusClicked();

	UFUNCTION()
	void HandleCharismaPlusClicked();

private:
	UPROPERTY(Transient)
	TObjectPtr<URPGCharacterCreationWizardWidget> OwningWizard;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttributePointsRemaining;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttributeHelp;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_HealthValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ManaValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CarryWeightValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ResetRecommendedAttributes;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StrengthClassValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StrengthRaceBonus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StrengthTotalValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StrengthModifier;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StrengthEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_StrengthMinus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_StrengthPlus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DexterityClassValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DexterityRaceBonus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DexterityTotalValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DexterityModifier;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DexterityEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_DexterityMinus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_DexterityPlus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ConstitutionClassValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ConstitutionRaceBonus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ConstitutionTotalValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ConstitutionModifier;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ConstitutionEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ConstitutionMinus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ConstitutionPlus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_IntelligenceClassValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_IntelligenceRaceBonus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_IntelligenceTotalValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_IntelligenceModifier;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_IntelligenceEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_IntelligenceMinus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_IntelligencePlus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_WisdomClassValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_WisdomRaceBonus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_WisdomTotalValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_WisdomModifier;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_WisdomEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_WisdomMinus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_WisdomPlus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CharismaClassValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CharismaRaceBonus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CharismaTotalValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CharismaModifier;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CharismaEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_CharismaMinus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_CharismaPlus;
};
