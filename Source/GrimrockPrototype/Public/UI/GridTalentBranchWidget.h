#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "GridTalentBranchWidget.generated.h"

class UBorder;
class UGridTalentNodeWidget;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridTalentBranchNodeClickedSignature, FName, TalentNodeId);

/** Reusable visual column for one five-tier Talent branch. */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridTalentBranchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Branch")
	FGridTalentBranchView BranchView;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Branch")
	FRPGTalentBranchPresentationDefinition BranchPresentation;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Branch")
	bool bInitialized = false;

	/** Presentation-only routing. No acquisition or progression mutation happens here. */
	UPROPERTY(BlueprintAssignable, Category = "RPG|Talents|Branch|Events")
	FGridTalentBranchNodeClickedSignature OnTalentNodeClicked;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Branch")
	bool InitializeTalentBranch(
		const FGridTalentBranchView& InBranchView,
		const FRPGTalentBranchPresentationDefinition& InPresentation);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Branch")
	void ClearTalentBranch();

	/** Presentation-only background supplied by the owning class visual asset. */
	void SetBranchBackground(UTexture2D* BackgroundTexture);

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Branch")
	UGridTalentNodeWidget* GetTalentNodeWidgetForTier(int32 Tier) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleTalentNodeClicked(FName TalentNodeId);

	void BindNodeEvents();
	void UnbindNodeEvents();
	void ApplyBranchPresentation();

	/** Authored background layer for this Talent branch. Keep behind the node widgets in WBP_RPGTalentBranch. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_BranchBackground;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_BranchName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_BranchProgress;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_BranchAccent;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentNodeWidget> Node_Tier1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentNodeWidget> Node_Tier2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentNodeWidget> Node_Tier3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentNodeWidget> Node_Tier4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGridTalentNodeWidget> Node_Tier5;
};
