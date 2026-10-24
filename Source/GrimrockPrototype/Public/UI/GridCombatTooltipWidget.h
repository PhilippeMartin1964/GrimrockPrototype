#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridEventFeedTypes.h"
#include "GridCombatTooltipWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UWidget;
class UGridCombatArmorRowWidget;

/**
 * Read-only, historical combat data. The native class formats this data into
 * localized FText, then sets existing UMG TextBlocks without altering their style.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCombatTooltipViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FText Attacker;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FText Defender;

	/** Raw attack, defense, damage and health snapshot for C++ formatting. */
	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridCombatResolutionTrace Trace;

	/** Entire ABSORPTION section is hidden when this is false. */
	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	bool bShowAbsorption = false;

	/** Only sources whose Absorbed > 0, in the authoritative consumption order. */
	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	TArray<FGridArmorAbsorptionSourceTrace> VisibleArmorRows;

	/** Transition to zero during this impact (may include multiple items). */
	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	TArray<FGridArmorAbsorptionSourceTrace> NewlyExhaustedSources;

	/** Equipped sources already at zero when this impact occurred. */
	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	TArray<FGridArmorAbsorptionSourceTrace> AlreadyDepletedSources;
};

/** Fully localized display values: C++ owns content; Designer owns style. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCombatTooltipDisplayTexts
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FText Attacker;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FText Defender;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FText Damage;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FText Health;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FText Status;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	bool bShowStatus = false;
};

/**
 * ARMEMENT01.10.3.3.8: independent, localized lines for designer-authored
 * TextBlocks. Empty lines are collapsed by the native presenter.
 * No layout, styling or widget construction is represented here.
 */
struct GRIMROCKPROTOTYPE_API FGridCombatTooltipDetailTexts
{
	FText AttackDice;
	FText AttackSourceAccuracy;
	FText AttackActionAccuracy;
	FText AttackTotal;
	FText DefenseBase;
	FText DefenseEvasion;
	FText DefenseTotal;
	FText AttackOutcome;
	FText AttackCritical;
};

/**
 * ARMEMENT01.10.3.3: native behavior and text assignment, designer-authored UMG.
 * C++ supplies all values, localized formats, visibility decisions and row instances.
 * WBP_GridCombatTooltip and WBP_GridCombatArmorRow own all layout and visual styling.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridCombatTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Return false to use the Event Feed's generic text tooltip if a WBP is incomplete. */
	UFUNCTION(BlueprintCallable, Category="Combat|Tooltip")
	bool InitializeCombatTooltip(const FGridHudEventEntry& Entry);

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridHudEventEntry CombatEntry;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridCombatTooltipViewData ViewData;

	static FGridCombatTooltipViewData BuildViewData(const FGridHudEventEntry& Entry);
	static FGridCombatTooltipDisplayTexts BuildDisplayTexts(const FGridCombatTooltipViewData& Data);
	static FGridCombatTooltipDetailTexts BuildDetailTexts(const FGridCombatTooltipViewData& Data);
	static int32 CountAbsorbingSources(const FGridCombatDamageTrace& Damage);

protected:
	/** Set in WBP_GridCombatTooltip > Class Defaults, after reparenting the row WBP. */
	UPROPERTY(EditDefaultsOnly, Category="Combat|Tooltip")
	TSubclassOf<UGridCombatArmorRowWidget> ArmorRowWidgetClass;

	/** Main content is authored in UMG; C++ supplies localized values only. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Attacker;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Defender;

	/** Nine required, independently editable Designer fields. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttackDice;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttackSourceAccuracy;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttackActionAccuracy;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttackTotal;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DefenseBase;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DefenseEvasion;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DefenseTotal;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttackOutcome;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttackCritical;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Damage;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Health;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	/** Visible/Collapsed applies to the whole authored section, including spacers. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> VerticalBox_AbsorptionSection;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_ArmorRows;

private:
	bool PopulateDesignerWidgets(const FGridCombatTooltipViewData& Data);
};
