#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FLinearColor BranchAccentColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	bool bInitialized = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Talents|Acquire")
	bool bAcquireConfirmationPending = false;

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

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	bool BeginAcquireConfirmation();

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
	void HandleConfirmAcquireClicked();

	UFUNCTION()
	void HandleCancelAcquireClicked();

	void BindAcquireButtons();
	void UnbindAcquireButtons();
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

	/** UI-RPG04.3A: optional until the confirmation controls are materialized in 04.3B. */
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
};
