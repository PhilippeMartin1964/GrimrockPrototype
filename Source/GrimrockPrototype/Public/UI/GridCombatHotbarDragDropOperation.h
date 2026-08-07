#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridCombatHotbarDragDropOperation.generated.h"

/** Payload used only to move or exchange persistent combat shortcuts. */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridCombatHotbarDragDropOperation
    : public UDragDropOperation
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
    int32 SourceSlotIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
    FGridCombatHotbarBinding Binding;

    UFUNCTION (BlueprintCallable, Category = "Combat|Hotbar|Drag")
    void InitializeFromHotbarSlot (
        int32 InCharacterIndex,
        int32 InSourceSlotIndex,
        const FGridCombatHotbarBinding& InBinding);
};
