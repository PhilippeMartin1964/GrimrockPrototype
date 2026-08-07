#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"

class UGridItemDefinitionAsset;

/** Immutable inputs used to evaluate contributions without mutating gameplay. */
struct GRIMROCKPROTOTYPE_API FGridCombatActionCatalogContext
{
    int32 CharacterIndex = INDEX_NONE;
    FGuid CharacterId;
    bool bCombatActive = false;
    bool bCharacterDefeated = false;
    bool bActiveCombatant = false;
    bool bPartyBusy = false;
    bool bEnableQuickItemExecutors = false;
    bool bEnableClassActionExecutors = false;
    int32 RemainingActionPoints = 0;
    int32 CurrentHealth = 0;
    int32 MaximumHealth = 0;
    int32 CurrentMana = 0;
    int32 MaximumMana = 0;
    TSet<FName> SatisfiedRequirements;
    TMap<FName, int32> RemainingCooldownRounds;
};

/**
 * Pure catalogue service. It evaluates definitions and current resources but
 * never resolves an effect and never consumes PA, mana or source items.
 */
class GRIMROCKPROTOTYPE_API FGridCombatActionCatalog
{
public:
    static void Build (
        const FGridCombatActionCatalogContext& Context,
        const TArray<FGridCombatActionContribution>& Contributions,
        TArray<FGridAvailableCombatAction>& OutActions);

    /** Compatibility adapter for MON11 item assets not migrated to Actions. */
    static FGridCombatActionDefinition MakeLegacyEquipmentAttackDefinition (
        const UGridItemDefinitionAsset& ItemDefinition,
        int32 ActionPointCost);

    static FGridCombatActionDefinition MakeUnarmedAttackDefinition (
        int32 ActionPointCost);

    static FText GetAvailabilityReasonText (
        EGridCombatActionAvailabilityReason Reason);
};
