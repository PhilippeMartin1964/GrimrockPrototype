#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryUiTypes.h"

class UGridItemDefinitionAsset;
class UGridItemSubsystem;
struct FGridItemInstance;

/** Read-only presentation: ItemSystem owns registered N1-N20 items; old
 * definitions remain temporarily for unregistered identities until 06.4. */
class GRIMROCKPROTOTYPE_API FGridWeaponTooltipProjection
{
public:
    /** False for malformed or unresolved instances; no fallback for known canonical IDs. */
    static bool Build(
        const UGridItemDefinitionAsset* Definition,
        const FGridItemInstance& Instance,
        FGridWeaponTooltipPresentation& Out,
        const UGridItemSubsystem* CanonicalRegistry = nullptr);

    /** Only compares like-for-like catalogued roles; never invents bare-hand damage. */
    static bool BuildComparison(
        const UGridItemDefinitionAsset* CandidateDefinition,
        const FGridItemInstance& Candidate,
        const UGridItemDefinitionAsset* EquippedDefinition,
        const FGridItemInstance& Equipped,
        TArray<FGridWeaponTooltipComparisonFact>& OutFacts,
        const UGridItemSubsystem* CanonicalRegistry = nullptr);
};
