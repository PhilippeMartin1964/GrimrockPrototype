#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.4X.16.4 — Non-mutating preflight for the REAL, separately
 * persisted receptacle acceptance lists. Never writes an asset/package.
 * This planner does not infer physical ItemLevel or change puzzle behavior.
 */
struct FGridReceptacleAcceptancePlanStats
{
    int32 Rules = 0;
    int32 AlreadyCanonical = 0;
    int32 EligibleLegacy = 0;
    int32 MissingSource = 0;
    int32 UnknownLegacy = 0;
    int32 UnknownCanonical = 0;
    int32 ContradictoryCanonical = 0;
    bool bRegistryUnavailable = false;

    bool HasBlockers() const
    {
        return bRegistryUnavailable || MissingSource > 0 ||
            UnknownLegacy > 0 || UnknownCanonical > 0 ||
            ContradictoryCanonical > 0;
    }

    void Accumulate(const FGridReceptacleAcceptancePlanStats& Other)
    {
        Rules += Other.Rules;
        AlreadyCanonical += Other.AlreadyCanonical;
        EligibleLegacy += Other.EligibleLegacy;
        MissingSource += Other.MissingSource;
        UnknownLegacy += Other.UnknownLegacy;
        UnknownCanonical += Other.UnknownCanonical;
        ContradictoryCanonical += Other.ContradictoryCanonical;
        bRegistryUnavailable = bRegistryUnavailable || Other.bRegistryUnavailable;
    }

    FString Describe() const
    {
        return FString::Printf(TEXT(
            "rules=%d already=%d eligible=%d missing=%d unknownLegacy=%d "
            "unknownCanonical=%d contradictory=%d registryUnavailable=%d"),
            Rules, AlreadyCanonical, EligibleLegacy, MissingSource,
            UnknownLegacy, UnknownCanonical, ContradictoryCanonical,
            bRegistryUnavailable ? 1 : 0);
    }
};

struct FGridReceptacleAcceptanceMigrationPreview
{
    static bool TryPreview(const UGridItemSubsystem* Registry,
        const TArray<FGridReceptacleAcceptedItemConfig>& Source,
        TArray<FGridReceptacleAcceptedItemConfig>& OutProposed,
        FGridReceptacleAcceptancePlanStats& OutStats)
    {
        OutProposed.Reset();
        OutStats = FGridReceptacleAcceptancePlanStats();
        OutStats.Rules = Source.Num();
        if (!IsValid(Registry) || !Registry->IsConfigured())
        {
            OutStats.bRegistryUnavailable = true;
            return false;
        }

        TArray<FGridReceptacleAcceptedItemConfig> Proposed;
        Proposed.Reserve(Source.Num());
        for (const FGridReceptacleAcceptedItemConfig& Rule : Source)
        {
            FGridReceptacleAcceptedItemConfig Copy = Rule;
            const UGridItemDefinitionAsset* Legacy = Rule.ItemDefinition.Get();
            if (!Rule.CanonicalItemId.IsNone())
            {
                if (!Registry->HasCanonicalItem(Rule.CanonicalItemId))
                {
                    ++OutStats.UnknownCanonical;
                    continue;
                }
                if (IsValid(Legacy) &&
                    Legacy->ItemDefinitionId != Rule.CanonicalItemId)
                {
                    ++OutStats.ContradictoryCanonical;
                    continue;
                }
                ++OutStats.AlreadyCanonical;
            }
            else if (!IsValid(Legacy) || Legacy->ItemDefinitionId.IsNone())
            {
                ++OutStats.MissingSource;
                continue;
            }
            else if (!Registry->HasCanonicalItem(Legacy->ItemDefinitionId))
            {
                ++OutStats.UnknownLegacy;
                continue;
            }
            else
            {
                Copy.CanonicalItemId = Legacy->ItemDefinitionId;
                ++OutStats.EligibleLegacy;
            }
            Proposed.Add(Copy);
        }

        if (OutStats.HasBlockers() || Proposed.Num() != Source.Num())
        {
            // Atomic proposal: no partial list is ever exposed to a caller.
            return false;
        }
        OutProposed = MoveTemp(Proposed);
        return true;
    }
};
