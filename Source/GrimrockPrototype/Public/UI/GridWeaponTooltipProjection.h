#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryUiTypes.h"

class UGridItemSubsystem;
struct FGridItemInstance;
struct FGridResolvedItem;

/** Read-only presentation for the registered physical levels N1–N20. */
class GRIMROCKPROTOTYPE_API FGridWeaponTooltipProjection
{
public:
    /** False for malformed or unresolved instances; no fallback for known canonical IDs. */
    static bool Build(
        const FGridItemInstance& Instance,
        FGridWeaponTooltipPresentation& Out,
        const UGridItemSubsystem* ItemRegistry = nullptr);

    /** Only compares like-for-like catalogued roles; never invents bare-hand damage. */
    static bool BuildComparison(
        const FGridItemInstance& Candidate,
        const FGridItemInstance& Equipped,
        TArray<FGridWeaponTooltipComparisonFact>& OutFacts,
        const UGridItemSubsystem* ItemRegistry = nullptr);
    static bool Project(const FGridItemInstance& Instance, const FGridResolvedItem& Item,
        FGridWeaponTooltipPresentation& Out);
    static bool Compare(const FGridResolvedItem& Candidate, const FGridResolvedItem& Equipped,
        TArray<FGridWeaponTooltipComparisonFact>& OutFacts);
};
