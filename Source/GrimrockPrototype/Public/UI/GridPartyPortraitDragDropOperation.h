#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "GridPartyPortraitDragDropOperation.generated.h"

/**
 * UI-PARTYHUD01.4 drag payload for party portrait presentation order.
 * Carries only stable character identity; never inventory data or runtime indices.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridPartyPortraitDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Persistent|Party|Drag")
	FGuid SourceCharacterId;

	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent|Party|Drag")
	void InitializeFromPortrait(FGuid InSourceCharacterId);

	UFUNCTION(BlueprintPure, Category = "HUD|Persistent|Party|Drag")
	bool HasValidSource() const;
};
