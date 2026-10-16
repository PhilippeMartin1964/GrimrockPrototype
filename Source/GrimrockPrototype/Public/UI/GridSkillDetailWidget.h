#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "GridSkillDetailWidget.generated.h"

class UImage;
class UTextBlock;
class UVerticalBox;

/**
 * Presentation-only detail panel for one selected Skill.
 *
 * Gameplay authority remains in the existing Skill services/read models.
 * This widget only formats and mirrors FGridSkillDetailView for Designer UMG.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridSkillDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FGridSkillDetailView DetailView;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	bool bInitialized = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedAttributeText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedRankText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedTrainingText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedCheckRankText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedCheckAttributeModifierText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedCheckBaseBonusText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedRecommendedClassesText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedUsefulClassesText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedSynergiesText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedCurrentRankCapText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedRemainingSkillPointsText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedNextRankText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Detail")
	FText ResolvedNextRankCostText;

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Detail")
	bool InitializeSkillDetail(const FGridSkillDetailView& InDetailView);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Detail")
	void ClearSkillDetail();

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Detail")
	void RefreshDetailVisual();

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Detail")
	static FText ResolveAttributeLabel(ERPGSkillGoverningAttribute Attribute);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Detail")
	static FText ResolveRankLabel(int32 Rank, int32 MaxRank);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Detail")
	static FText ResolveTrainingLabel(EGridSkillTrainingState TrainingState);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Detail")
	static FText ResolveSignedInteger(int32 Value);

protected:
	virtual void NativeConstruct() override;

private:
	void RefreshResolvedPresentation();

	static FText JoinClassNames(const TArray<FGridSkillClassAffinityView>& Classes);
	static FText JoinSynergyNames(const TArray<FGridSkillSynergyView>& Synergies);
	static void ApplyOptionalSection(UVerticalBox* Section, UTextBlock* ValueWidget, const FText& Value);


	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_SkillIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailAttribute;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailRank;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailDescription;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailDescription;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailTraining;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailTraining;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailUsage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailUsage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailBuildAdvice;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailBuildAdvice;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailCheck;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailCheckRank;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailCheckAttributeModifier;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailCheckBaseBonus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailAffinities;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailRecommendedClasses;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailRecommendedClasses;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailUsefulClasses;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailUsefulClasses;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailSynergies;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailSynergies;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailProgression;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailCurrentRankCap;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailRemainingSkillPoints;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailNextRank;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailNextRank;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_DetailNextRankCost;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DetailNextRankCost;
};
