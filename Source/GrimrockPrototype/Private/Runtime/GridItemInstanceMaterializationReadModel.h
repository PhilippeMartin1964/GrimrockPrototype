#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.5G.94: validate physical identity against ItemSystem.
 * Canonical instances do not carry cached name or mass. Historical
 * Automation fixtures may validate old definitions but never write metadata
 * into the physical instance.
 */
struct FGridItemInstanceMaterializationReadModel
{
    static bool TryApply(const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* LegacyDefinition,
        FGridItemInstance& InOutInstance)
    {
        const bool bRegisteredIdentity =
            Registry && Registry->ContainsItem(InOutInstance.ItemDefinitionId);
        if (InOutInstance.ItemDefinitionId.IsNone() ||
            InOutInstance.ItemLevel < 0 || InOutInstance.ItemLevel > 20 ||
            (!bRegisteredIdentity && (!IsValid(LegacyDefinition) ||
                LegacyDefinition->ItemDefinitionId != InOutInstance.ItemDefinitionId)))
        {
            return false;
        }

        // All mutations are staged; failures never alter a saved physical
        // identity, ownership, light state or armor debt.
        FGridItemInstance Candidate = InOutInstance;
        if (bRegisteredIdentity)
        {
            FGridResolvedItem Resolved;
            if (!Registry->ResolveItem(Candidate.ItemDefinitionId, Candidate.ItemLevel, Resolved) ||
                (!Resolved.bIsArmament && Candidate.ItemLevel != 0) ||
                !FMath::IsFinite(Resolved.WeightKg) || Resolved.WeightKg < 0.0f)
            {
                return false; // Physical instances never inherit an obsolete default or family alias.
            }
        }
        else
        {
            // Only historical inventory characterization fixtures may
            // materialize a test-only definition outside ItemSystem.
#if WITH_DEV_AUTOMATION_TESTS
            if (Registry || !GIsAutomationTesting) return false;
            float UnitWeight = LegacyDefinition->Weight;
            if (LegacyDefinition->bUseWeaponCatalogue)
            {
                const int32 Level = Candidate.ItemLevel == 0
                    ? LegacyDefinition->DefaultWeaponItemLevel : Candidate.ItemLevel;
                const FGridWeaponLevelProfile* Profile = nullptr;
                if (!LegacyDefinition->TryGetWeaponCatalogueProfile(Level, Profile))
                {
                    return false;
                }
                Candidate.ItemLevel = Level;
                UnitWeight = Profile->WeightKg;
            }
            if (!FMath::IsFinite(UnitWeight) || UnitWeight < 0.0f) return false;
#else
            return false;
#endif
        }
        InOutInstance = MoveTemp(Candidate);
        return true;
    }
};
