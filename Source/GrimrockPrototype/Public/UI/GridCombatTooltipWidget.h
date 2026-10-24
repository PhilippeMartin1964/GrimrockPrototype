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
 * Numeric dice projection retained for compatibility and tests.
 * The native presenter now formats dice and signs in C++, not Blueprint.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCombatTooltipDiceNumbers
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip|Dice")
	bool bHasDice = false;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip|Dice")
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip|Dice")
	int32 Sides = 0;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip|Dice")
	int32 Rolled = 0;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip|Dice")
	int32 Offset = 0;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip|Dice")
	int32 Total = 0;
};

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

	/** Mechanical dice projection also used by regression tests. */
	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridCombatTooltipDiceNumbers AttackDice;

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
	FText Attack;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FText Defense;

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
 * ARMEMENT01.10.3.3: native behavior and text assignment, designer-authored UMG.
 * C++ supplies all values, localized formats, visibility decisions and row instances.
 * WBP_GridCombatTooltip and WBP_GridCombatArmorRow own all layout and visual styling.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridCombatTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Return false to keep the legacy text tooltip when WBP setup is incomplete. */
	UFUNCTION(BlueprintCallable, Category="Combat|Tooltip")
	bool InitializeCombatTooltip(const FGridHudEventEntry& Entry);

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridHudEventEntry CombatEntry;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridCombatTooltipViewData ViewData;

	static FGridCombatTooltipViewData BuildViewData(const FGridHudEventEntry& Entry);
	static FGridCombatTooltipDisplayTexts BuildDisplayTexts(const FGridCombatTooltipViewData& Data);
	static int32 CountAbsorbingSources(const FGridCombatDamageTrace& Damage);

protected:
	/** Set in WBP_GridCombatTooltip > Class Defaults, after reparenting the row WBP. */
	UPROPERTY(EditDefaultsOnly, Category="Combat|Tooltip")
	TSubclassOf<UGridCombatArmorRowWidget> ArmorRowWidgetClass;

	/** The eight designer-authored variable widgets; C++ only assigns text. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Attacker;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Defender;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Attack;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Defense;

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

	/**
	 * Legacy Blueprint override kept ONLY for Blueprint asset compatibility
	 * while old Event Graph nodes are manually removed in Unreal Editor.
	 * InitializeCombatTooltip no longer invokes this method.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Combat|Tooltip")
	bool PresentCombatTooltip(const FGridCombatTooltipViewData& Data);

	virtual bool PresentCombatTooltip_Implementation(const FGridCombatTooltipViewData& Data);

private:
	bool PopulateDesignerWidgets(const FGridCombatTooltipViewData& Data);
};
