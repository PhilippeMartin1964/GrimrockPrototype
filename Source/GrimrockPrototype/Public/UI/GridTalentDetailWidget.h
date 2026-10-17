#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "GridTalentDetailWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UGridTalentDetailLineWidget;
class URPGTalentDetailGlyphSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentAcquireConfirmedSignature, FName, ChoiceId);

/** Canonical Talent detail + confirmation presenter. Gameplay authority remains outside this widget. */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridTalentDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FGridTalentDetailView DetailView;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedTypeText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedStatusText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FText ResolvedPrincipleText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	FLinearColor BranchAccentColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail")
	bool bInitialized = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "RPG|Talents|Acquire")
	bool bAcquireConfirmationPending = false;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Acquire|Events")
	FGridTalentAcquireConfirmedSignature OnAcquireConfirmed;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Detail")
	bool InitializeTalentDetail(
		const FGridTalentDetailView& InDetailView,
		const FRPGTalentBranchPresentationDefinition& InBranchPresentation);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Detail")
	void ClearTalentDetail();

	/** Presentation-only icon resolved from URPGClassVisualAsset by canonical TalentNodeId. */
	void SetTalentIcon(UTexture2D* IconTexture);

		/** Branch-specific illustration supplied by URPGClassVisualAsset. */
	void SetTalentTypeImage(UTexture2D* ImageTexture);

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Acquire")
	bool CanRequestSimpleAcquisition() const;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	bool BeginAcquireConfirmation();

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Acquire")
	bool ConfirmAcquire();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleAcquireClicked();

	UFUNCTION()
	void HandleConfirmAcquireClicked();

	void BindAcquireButtons();
	void UnbindAcquireButtons();
	void RebuildDetailLineRows();
	void PopulateDetailLineRows(UVerticalBox* Container, const TArray<FGridTalentDetailLineView>& Lines);
	void BuildAcquisitionLines(TArray<FGridTalentDetailLineView>& OutLines) const;
	void RefreshCanonicalSections();
	void ApplyDetailPresentation();
	void ApplyAcquisitionPresentation();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_TalentType;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_DetailAccent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_DetailTalentIconFrame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_DetailTalentIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_DetailType;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailType;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_DetailStatus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailStatus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_DetailPrinciple;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetailPrinciple;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_DetailEffects;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_DetailEffectsEntries;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_DetailUsage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_DetailUsageEntries;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailAcquisition;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailAcquisitionEntries;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RPG|Talents|Detail|Lines", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGridTalentDetailLineWidget> DetailLineWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RPG|Talents|Detail|Lines", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URPGTalentDetailGlyphSet> DetailGlyphSet;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_AcquireTalent;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_ConfirmAcquire;


};
