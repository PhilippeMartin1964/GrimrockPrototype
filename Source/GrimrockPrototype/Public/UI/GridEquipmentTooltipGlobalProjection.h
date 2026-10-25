#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryUiTypes.h"

class UGridPartyInventoryComponent;
class UGridItemDefinitionAsset;

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
        const UGridItemDefinitionAsset* CandidateDefinition,
        const FGridItemInstance& Candidate,
        FGridItemTooltipEquipmentComparison& OutScenario);

    /** True for hand slots allowed by gameplay, not just authored array entries. */
    static void GatherAllowedSlots(
        const UGridItemDefinitionAsset* Candidate,
        TArray<EGridEquipmentSlot>& OutSlots);
};
