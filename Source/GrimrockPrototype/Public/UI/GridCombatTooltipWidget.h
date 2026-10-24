#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridEventFeedTypes.h"
#include "GridCombatTooltipWidget.generated.h"

/**
 * Numeric dice projection for the WBP. Formatting and signs belong to UMG,
 * but converting a Range roll into an equivalent virtual die is domain data.
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
 * Read-only data for WBP_GridCombatTooltip.
 * No Slate layout, fonts, widgets or localized presentation strings in C++.
 * Combat values remain the immutable snapshot of the historical event.
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

	/** Raw attack, defense, damage and health figures for WBP formatting. */
	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridCombatResolutionTrace Trace;

	/** Mechanical dice projection (no display text constructed in C++). */
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

/**
 * ARMEMENT01.10.3.3 — C++ read-model ONLY.
 * WBP_GridCombatTooltip owns all text formatting, ST_UI, visibility and layout.
 * A missing Blueprint renderer reports false to keep the existing text tooltip.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridCombatTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** False => presenter is not ready; the Event Feed must use its old tooltip. */
	UFUNCTION(BlueprintCallable, Category="Combat|Tooltip")
	bool InitializeCombatTooltip(const FGridHudEventEntry& Entry);

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridHudEventEntry CombatEntry;

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridCombatTooltipViewData ViewData;

	/** Pure read-model function, usable by automated tests without any UMG widget. */
	static FGridCombatTooltipViewData BuildViewData(const FGridHudEventEntry& Entry);

	/** Only positive, resolved physical absorption yields visible rows. */
	static int32 CountAbsorbingSources(const FGridCombatDamageTrace& Damage);

protected:
	/**
	 * Implement in WBP_GridCombatTooltip: set texts from ST_UI, style, visibility,
	 * and instantiate BP-authored WBP_GridCombatArmorRow for every VisibleArmorRows item.
	 * Return true after presentation succeeds. Default false preserves old tooltip.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Combat|Tooltip")
	bool PresentCombatTooltip(const FGridCombatTooltipViewData& Data);

	virtual bool PresentCombatTooltip_Implementation(const FGridCombatTooltipViewData& Data);
};
