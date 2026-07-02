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
    virtual bool SubmitCharacterCreation () override;

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
    void BindWizardSubmitButton ();
    void RefreshWizardShell ();
    void ApplyWizardStepToSwitcher ();
    UWidget* GetPanelForWizardStep (ERPGCharacterCreationWizardStep Step) const;
    void EnsureAttributeAllocationInitialized ();
    TSoftObjectPtr<UTexture2D> ResolveWizardSelectedClassIcon () const;

    UFUNCTION ()
    void HandlePreviousClicked ();

    UFUNCTION ()
    void HandleNextClicked ();

    UFUNCTION ()
    void HandleCancelClicked ();

    UFUNCTION ()
    void HandleWizardCreateCharacterClicked ();

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

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<URPGCharacterCreationAttributesStepWidget> Widget_StepAttributes;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_StepIdentity;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_StepSummary;

    UPROPERTY (Transient)
    FRPGAttributes AllocatedClassAttributes;

    UPROPERTY (Transient)
    FName AllocatedClassId = NAME_None;

    UPROPERTY (Transient)
    bool bHasInitializedAllocatedClassAttributes = false;
};