#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridEventFeedTypes.h"
#include "GridCombatTooltipWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/**
 * ARMEMENT01.10.3.3 — view-only structured combat tooltip.
 * Layout/style lives in the authored WBP_GridCombatTooltip, while combat
 * authority is the immutable FGridHudEventEntry snapshot.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridCombatTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Combat|Tooltip")
	void InitializeCombatTooltip(const FGridHudEventEntry& Entry);

	UPROPERTY(BlueprintReadOnly, Category="Combat|Tooltip")
	FGridHudEventEntry CombatEntry;

	/** Count sources that absorbed strictly positive damage in this historical hit. */
	static int32 CountAbsorbingSources(const FGridCombatDamageTrace& Damage);

	/** Dynamic armor rows copy their font from the WBP's Text_HeaderSource. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tooltip|Style")
	FLinearColor NormalTextColor = FLinearColor(0.88f, 0.83f, 0.72f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tooltip|Style")
	FLinearColor ArmorTextColor = FLinearColor(0.53f, 0.79f, 0.69f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tooltip|Style")
	FLinearColor WarningTextColor = FLinearColor(0.97f, 0.68f, 0.33f, 1.0f);

protected:
	virtual void NativeConstruct() override;

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
	TObjectPtr<UVerticalBox> VerticalBox_ArmorRows;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Health;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

private:
	void RefreshTooltip();
};
