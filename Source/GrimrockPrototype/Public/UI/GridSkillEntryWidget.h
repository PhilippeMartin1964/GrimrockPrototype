#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "GridSkillEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridSkillIncreaseRequestedSignature, FName, SkillId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridSkillDecreaseRequestedSignature, FName, SkillId);

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

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|Entry|Events")
	FGridSkillIncreaseRequestedSignature OnIncreaseSkillRequested;

	UPROPERTY(BlueprintAssignable, Category = "RPG|Skills|Entry|Events")
	FGridSkillDecreaseRequestedSignature OnDecreaseSkillRequested;

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

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleIncreaseSkillClicked();

	UFUNCTION()
	void HandleDecreaseSkillClicked();

	static bool IsValidEntry(const FGridSkillEntryView& InEntry);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillAttribute;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillRank;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillTrainingPolicy;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_IncreaseSkill;

	/** RPG-SKILL01.3B Designer materialization; optional until WBP_RPGSkillEntry is updated manually. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_DecreaseSkill;
};
