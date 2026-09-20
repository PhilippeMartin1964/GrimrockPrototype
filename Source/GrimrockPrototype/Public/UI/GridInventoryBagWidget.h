#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryWidget.h"
#include "GridInventoryBagWidget.generated.h"

/**
 * UI-SPLIT01 semantic right-side inventory window.
 *
 * It deliberately reuses UGridInventoryWidget's optional projections and
 * interaction routing so drag/drop, cursor state and item actions remain on
 * the existing authoritative inventory implementation.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridInventoryBagWidget : public UGridInventoryWidget
{
	GENERATED_BODY()
};
