#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "GridTalentDetailWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentAcquireConfirmedSignature, FName, ChoiceId);

/** Talent detail + confirmation presenter. Gameplay authority remains outside this widget. */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridTalentDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FGridTalentNodeView NodeView;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedDescription;

	/** Concrete variant preview; populated only for multi-variant conceptual nodes. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedVariantDisplayName;

	/** Actual FGridTalentVariantView::Description of the previewed variant. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedVariantDescription;

	/** Derived only from authoritative CombatActions unlocked by the previewed concrete choice. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedActionSummary;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FLinearColor BranchAccentColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	bool bInitialized = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Talents|Acquire")
	bool bAcquireConfirmationPending = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Talents|Acquire")
	bool bVariantSelectionPending = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Talents|Acquire")
	FName SelectedVariantChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Acquire")
	FText AcquisitionFeedback;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Acquire|Events")
	FGridTalentAcquireConfirmedSignature OnAcquireConfirmed;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Detail")
	bool InitializeTalentDetail(
		const FGridTalentNodeView& InNodeView,
		const FRPGTalentBranchPresentationDefinition& InBranchPresentation);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Detail")
	void ClearTalentDetail();

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Acquire")
	bool CanRequestSimpleAcquisition() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Acquire")
	bool CanRequestVariantAcquisition() const;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	bool BeginAcquireConfirmation();

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	bool BeginVariantSelection();

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	bool SelectVariantChoice(FName ChoiceId);

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Acquire")
	FName GetSelectedVariantChoiceId() const { return SelectedVariantChoiceId; }

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Acquire")
	bool GetVariantDisplayLabel(FName ChoiceId, FText& OutLabel) const;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	void CancelAcquireConfirmation();

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	bool ConfirmAcquire();

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	void SetAcquisitionFeedback(const FText& InFeedback);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleAcquireClicked();

	UFUNCTION()
	void HandleChooseVariantClicked();

	UFUNCTION()
	void HandleVariantSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleConfirmAcquireClicked();

	UFUNCTION()
	void HandleCancelAcquireClicked();

	void BindAcquireButtons();
	void UnbindAcquireButtons();
	void RebuildVariantOptions();
	FText MakeVariantDisplayLabel(const FGridTalentVariantView& Variant) const;
	void RefreshVariantDetailPreview();
	FText BuildActionSummary(const FGridTalentVariantView& Variant) const;
	void ApplyDetailPresentation();
	void ApplyAcquisitionPresentation();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_DetailAccent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailLevel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailCost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailState;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailVariants;

	/** UI-RPG-DESC01.2 Designer materialization points. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailVariantName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailVariantDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailActionSummary;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_AcquireTalent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ConfirmAcquire;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_CancelAcquire;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AcquirePrompt;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AcquireFeedback;

	/** UI-RPG04.4: generic N-variant selector; optional until the WBP migration is saved. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ChooseVariant;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> Combo_VariantChoice;

	TArray<FString> VariantOptionLabels;
	TArray<FName> VariantOptionChoiceIds;
};
