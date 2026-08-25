#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridInventoryDragDropOperation.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API UGridInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Drag")
	EGridInventoryUiSlotType SourceSlotType = EGridInventoryUiSlotType::Inventory;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Drag")
	int32 SourceSlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Drag")
	FName SourceItemDefinitionId = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Drag")
	FGuid SourceRuntimeObjectId;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Drag")
	FGridItemInstance SourceItem;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Drag")
	bool bHasItem = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Drag")
	int32 RequestedQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Drag")
	bool bSplitStack = false;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag")
	void InitializeFromSlot(EGridInventoryUiSlotType InSlotType, int32 InSlotIndex, const FGridItemInstance& InItem);
};
