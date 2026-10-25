#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryUiTypes.h"

class UGridItemDefinitionAsset;
struct FGridItemInstance;

/** INDUS04.1: pure read model; only the fixed-level runtime catalogue is authoritative. */
class GRIMROCKPROTOTYPE_API FGridWeaponTooltipProjection
{
public:
    /** False for legacy items, malformed instances and missing catalogue profiles. */
    static bool Build(
        const UGridItemDefinitionAsset* Definition,
        const FGridItemInstance& Instance,
        FGridWeaponTooltipPresentation& Out);

    /** Only compares like-for-like catalogued roles; never invents bare-hand damage. */
    static bool BuildComparison(
        const UGridItemDefinitionAsset* CandidateDefinition,
        const FGridItemInstance& Candidate,
        const UGridItemDefinitionAsset* EquippedDefinition,
        const FGridItemInstance& Equipped,
        TArray<FGridWeaponTooltipComparisonFact>& OutFacts);
};
