#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "GridSkillEntryWidget.generated.h"

class UTextBlock;

/**
 * Presentation-only row for one canonical Skill.
 *
 * The widget owns no Skill progression state. It receives one immutable
 * FGridSkillEntryView produced by FGridSkillsPageService and mirrors only
 * fields that already exist in the read model.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridSkillEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Entry")
	FGridSkillEntryView Entry;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Entry")
	bool bInitialized = false;

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry")
	bool InitializeSkillEntry(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry")
	void ClearSkillEntry();

	UFUNCTION(BlueprintCallable, Category = "RPG|Skills|Entry")
	void RefreshEntryVisual();

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static FText ResolveDisplayName(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static FText ResolveAttributeLabel(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static FText ResolveRankLabel(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static FText ResolveTrainingLabel(const FGridSkillEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "RPG|Skills|Entry")
	static bool HasDescription(const FGridSkillEntryView& InEntry);

private:
	static bool IsValidEntry(const FGridSkillEntryView& InEntry);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillAttribute;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillRank;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillTrainingPolicy;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillDescription;
};
