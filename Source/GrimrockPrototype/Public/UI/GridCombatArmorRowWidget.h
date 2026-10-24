#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/Combat/GridCombatTraceTypes.h"
#include "GridCombatArmorRowWidget.generated.h"

class UTextBlock;

/**
 * Native data presenter for one designer-authored WBP_GridCombatArmorRow.
 * Never creates TextBlocks, changes fonts, positions columns or edits margins.
 * The Widget Blueprint owns every visual property of the four TextBlocks.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridCombatArmorRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by the native parent AFTER adding the row to its VerticalBox. */
	bool SetArmorData(const FGridArmorAbsorptionSourceTrace& Source);

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Source;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Before;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Absorbed;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_After;
};
