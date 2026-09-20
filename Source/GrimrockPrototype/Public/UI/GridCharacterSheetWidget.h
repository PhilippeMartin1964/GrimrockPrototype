#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryWidget.h"
#include "GridCharacterSheetWidget.generated.h"

/**
 * UI-SPLIT01 semantic left-side inventory window.
 *
 * It deliberately reuses UGridInventoryWidget's optional projections and
 * interaction routing so no second character/inventory authority is created.
 * WBP_CharacterSheet should contain only party/character/paper-doll bindings.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridCharacterSheetWidget : public UGridInventoryWidget
{
	GENERATED_BODY()
};
