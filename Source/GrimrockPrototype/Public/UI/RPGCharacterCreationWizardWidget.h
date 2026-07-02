#pragma once

#include "CoreMinimal.h"
#include "UI/RPGCharacterCreationWidget.h"
#include "RPGCharacterCreationWizardWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;
class URPGCharacterCreationAttributesStepWidget;

UENUM (BlueprintType)
enum class ERPGCharacterCreationWizardStep : uint8
{
    Race UMETA (DisplayName = "Race"),
    Class UMETA (DisplayName = "Classe"),
    Attributes UMETA (DisplayName = "Caractéristiques"),
    Identity UMETA (DisplayName = "Identité"),
    Summary UMETA (DisplayName = "Résumé")
};

/**
 * Multi-step shell for the real character creation wizard.
 *
 * The class deliberately inherits from URPGCharacterCreationWidget so the
 * already validated creation request, preview and submit logic stay in one
 * place while the new Blueprint can be laid out as a proper wizard.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API URPGCharacterCreationWizardWidget : public URPGCharacterCreationWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Wizard")
    ERPGCharacterCreationWizardStep InitialWizardStep = ERPGCharacterCreationWizardStep::Race;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Wizard")
    ERPGCharacterCreationWizardStep CurrentWizardStep = ERPGCharacterCreationWizardStep::Race;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Wizard")
    bool bAllowCancel = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Wizard")
    bool bFocusNameInputOnIdentityStep = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Attributes", meta = (ClampMin = "1"))
    int32 AttributeAllocationMinimum = 8;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Attributes", meta = (ClampMin = "1"))
    int32 AttributeAllocationMaximum = 16;

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation|Wizard")
    void SetCurrentWizardStep (ERPGCharacterCreationWizardStep NewStep);

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation|Wizard")
    bool GoToNextWizardStep ();

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation|Wizard")
    bool GoToPreviousWizardStep ();

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation|Wizard")
    void CancelWizard ();

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Wizard")
    bool CanGoToNextWizardStep () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Wizard")
    bool CanGoToPreviousWizardStep () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Wizard")
    bool IsWizardOnLastStep () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Wizard")
    int32 GetCurrentWizardStepIndex () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Wizard")
    int32 GetCurrentWizardStepNumber () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Wizard")
    int32 GetWizardStepCount () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Wizard")
    FText GetCurrentWizardStepTitle () const;

    virtual void RefreshPreview () override;
    virtual bool CanSubmitCharacterCreation () const override;
    virtual bool GetPreviewAttributes (FRPGAttributes& OutAttributes) const override;
    virtual bool GetPreviewDerivedStats (FRPGDerivedStats& OutDerivedStats) const override;
    virtual float GetPreviewCarryWeight () const override;

    virtual bool SubmitCharacterCreation () override
    {
        return SubmitWizardCharacterCreation ();
    }

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation|Attributes")
    void ResetAttributeAllocationToClassDefinition ();

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation|Attributes")
    void AdjustAllocatedAttribute (FName AttributeId, int32 Delta);

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Attributes")
    bool CanIncreaseAllocatedAttribute (FName AttributeId) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Attributes")
    bool CanDecreaseAllocatedAttribute (FName AttributeId) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Attributes")
    int32 GetAttributePointBudget () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Attributes")
    int32 GetAllocatedAttributePointsSpent () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Attributes")
    int32 GetRemainingAttributePoints () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Attributes")
    FRPGAttributes GetAllocatedClassAttributesForPreview () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation|Attributes")
    bool IsUsingRecommendedAttributes () const;

    UFUNCTION (BlueprintImplementableEvent, Category = "RPG|Character Creation|Wizard")
    void OnWizardStepChanged (ERPGCharacterCreationWizardStep PreviousStep, ERPGCharacterCreationWizardStep NewStep);

protected:
    virtual void NativeConstruct () override;

private:
    void BindWizardButtons ();
    void BindAttributeAllocationButtons ();
    void BindWizardSubmitButton ();
    void RefreshWizardShell ();
    void ApplyWizardStepToSwitcher ();
    UWidget* GetPanelForWizardStep (ERPGCharacterCreationWizardStep Step) const;
    void RefreshAttributeAllocationPreview ();
    void RefreshAttributeAllocationControls ();
    void EnsureAttributeAllocationInitialized ();
    TSoftObjectPtr<UTexture2D> ResolveWizardSelectedClassIcon () const;
    bool SubmitWizardCharacterCreation ();

    UFUNCTION ()
    void HandlePreviousClicked ();

    UFUNCTION ()
    void HandleNextClicked ();

    UFUNCTION ()
    void HandleCancelClicked ();

    UFUNCTION ()
    void HandleWizardCreateCharacterClicked ();

    UFUNCTION ()
    void HandleResetRecommendedAttributesClicked ();

    UFUNCTION ()
    void HandleStrengthMinusClicked ();

    UFUNCTION ()
    void HandleStrengthPlusClicked ();

    UFUNCTION ()
    void HandleDexterityMinusClicked ();

    UFUNCTION ()
    void HandleDexterityPlusClicked ();

    UFUNCTION ()
    void HandleConstitutionMinusClicked ();

    UFUNCTION ()
    void HandleConstitutionPlusClicked ();

    UFUNCTION ()
    void HandleIntelligenceMinusClicked ();

    UFUNCTION ()
    void HandleIntelligencePlusClicked ();

    UFUNCTION ()
    void HandleWisdomMinusClicked ();

    UFUNCTION ()
    void HandleWisdomPlusClicked ();

    UFUNCTION ()
    void HandleCharismaMinusClicked ();

    UFUNCTION ()
    void HandleCharismaPlusClicked ();

private:
    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Previous;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Next;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Cancel;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_StepTitle;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_StepCounter;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidgetSwitcher> WidgetSwitcher_Steps;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_StepRace;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_StepClass;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_StepAttributes;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<URPGCharacterCreationAttributesStepWidget> Widget_StepAttributes;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_StepIdentity;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_StepSummary;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_AttributePointsRemaining;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_AttributeHelp;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_ResetRecommendedAttributes;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_StrengthClassValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_StrengthRaceBonus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_StrengthTotalValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_StrengthModifier;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_StrengthEffects;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_StrengthMinus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_StrengthPlus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DexterityClassValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DexterityRaceBonus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DexterityTotalValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DexterityModifier;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DexterityEffects;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_DexterityMinus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_DexterityPlus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ConstitutionClassValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ConstitutionRaceBonus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ConstitutionTotalValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ConstitutionModifier;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ConstitutionEffects;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_ConstitutionMinus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_ConstitutionPlus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_IntelligenceClassValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_IntelligenceRaceBonus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_IntelligenceTotalValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_IntelligenceModifier;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_IntelligenceEffects;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_IntelligenceMinus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_IntelligencePlus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_WisdomClassValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_WisdomRaceBonus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_WisdomTotalValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_WisdomModifier;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_WisdomEffects;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_WisdomMinus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_WisdomPlus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_CharismaClassValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_CharismaRaceBonus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_CharismaTotalValue;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_CharismaModifier;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_CharismaEffects;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_CharismaMinus;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_CharismaPlus;

    UPROPERTY (Transient)
    FRPGAttributes AllocatedClassAttributes;

    UPROPERTY (Transient)
    FName AllocatedClassId = NAME_None;

    UPROPERTY (Transient)
    bool bHasInitializedAllocatedClassAttributes = false;
};