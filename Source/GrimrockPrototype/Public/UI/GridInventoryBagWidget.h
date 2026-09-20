#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryWidget.h"
#include "GridInventoryBagWidget.generated.h"

class UButton;

/**
 * Independent right-side inventory bag window.
 *
 * Gameplay authority remains UGridPartyInventoryComponent through the shared
 * UGridInventoryWidget projection/interaction layer.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridInventoryBagWidget : public UGridInventoryWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|UI")
	TObjectPtr<UButton> Button_CloseInventoryBag;
};
