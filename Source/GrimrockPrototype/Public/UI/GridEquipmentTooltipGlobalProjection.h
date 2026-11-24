#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryUiTypes.h"

class UGridPartyInventoryComponent;
class UGridItemSubsystem;

/**
 * ARMEMENT-INDUS04.3B: bridges 04.3A's non-mutating equipment transaction
 * to the existing comparison widget, without adding Graph loops or UMG.
 */
struct GRIMROCKPROTOTYPE_API FGridEquipmentTooltipGlobalProjection
{
    static bool BuildScenario(
        const UGridPartyInventoryComponent* Inventory,
        int32 CharacterIndex,
        int32 InventorySlotIndex,
        EGridEquipmentSlot RequestedSlot,
        const FGridItemInstance& Candidate,
        FGridItemTooltipEquipmentComparison& OutScenario);

    /** Real ItemInstance level and canonical registry decide all shown slots. */
    static void GatherAllowedSlots(
        const UGridItemSubsystem* Registry,
        const FGridItemInstance& Candidate,
        TArray<EGridEquipmentSlot>& OutSlots);
};
