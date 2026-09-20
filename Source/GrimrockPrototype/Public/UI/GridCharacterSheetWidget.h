#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryWidget.h"
#include "GridCharacterSheetWidget.generated.h"

class UButton;

/**
 * Independent left-side character/equipment window.
 *
 * Gameplay authority remains UGridPartyInventoryComponent through the shared
 * UGridInventoryWidget projection/interaction layer.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridCharacterSheetWidget : public UGridInventoryWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|UI")
	TObjectPtr<UButton> Button_CloseCharacterSheet;
};
