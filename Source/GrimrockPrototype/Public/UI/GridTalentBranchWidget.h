#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "GridTalentBranchWidget.generated.h"

class UBorder;
class UGridTalentNodeWidget;
class UTextBlock;

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

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Branch")
	bool InitializeTalentBranch(
		const FGridTalentBranchView& InBranchView,
		const FRPGTalentBranchPresentationDefinition& InPresentation);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Branch")
	void ClearTalentBranch();

	UFUNCTION(BlueprintPure, Category = "RPG|Talents|Branch")
	UGridTalentNodeWidget* GetTalentNodeWidgetForTier(int32 Tier) const;

private:
	void ApplyBranchPresentation();

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
