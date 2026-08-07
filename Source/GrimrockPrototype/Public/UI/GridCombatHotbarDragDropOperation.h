#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridCombatHotbarDragDropOperation.generated.h"

/** Payload used to move a shortcut or assign an action-palette entry. */
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

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
    bool bFromActionPalette = false;

    UFUNCTION (BlueprintCallable, Category = "Combat|Hotbar|Drag")
    void InitializeFromHotbarSlot (
        int32 InCharacterIndex,
        int32 InSourceSlotIndex,
        const FGridCombatHotbarBinding& InBinding);

    UFUNCTION (BlueprintCallable, Category = "Combat|Hotbar|Drag")
    void InitializeFromActionPalette (
        int32 InCharacterIndex,
        const FGridAvailableCombatAction& InAction);
};
