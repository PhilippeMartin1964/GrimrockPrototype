#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.3AG: transitional write of cached instance metadata.
 * ItemSystem owns the identity and mass of all registered objects, while
 * snapshots remain serialized only until the verified SaveGame conversion.
 */
struct FGridItemInstanceMaterializationReadModel
{
    static bool TryApply(const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* LegacyDefinition,
        FGridItemInstance& InOutInstance)
    {
        const bool bCanonicalIdentity =
            Registry && Registry->HasCanonicalItem(InOutInstance.ItemDefinitionId);
        if (InOutInstance.ItemDefinitionId.IsNone() ||
            InOutInstance.ItemLevel < 0 || InOutInstance.ItemLevel > 20 ||
            (IsValid(LegacyDefinition) &&
                LegacyDefinition->ItemDefinitionId != InOutInstance.ItemDefinitionId) ||
            (!bCanonicalIdentity && !IsValid(LegacyDefinition)))
        {
            return false;
        }

        // All mutations are staged; failures never alter a saved physical
        // identity, ownership, light state, armor debt or cached metadata.
        FGridItemInstance Candidate = InOutInstance;
        if (bCanonicalIdentity)
        {
            // Probe N1 solely to discover the canonical family. Never assign
            // N1 to an item: non-armaments such as arrows persist at N0.
            FGridResolvedItem Family;
            if (!Registry->ResolveItem(Candidate.ItemDefinitionId, 1, Family))
            {
                return false;
            }
            if (Family.bIsArmament && Candidate.ItemLevel == 0)
            {
                // An authored N0 ARM can inherit the pre-cutover default
                // only when its *matching* historical authoring asset exists.
                // Without it N0 is invalid, never silently converted to N1.
                if (!IsValid(LegacyDefinition))
                {
                    return false;
                }
                Candidate.ItemLevel = LegacyDefinition->DefaultWeaponItemLevel;
            }
            FGridResolvedItem Resolved;
            if (!Registry->ResolveItem(Candidate.ItemDefinitionId, Candidate.ItemLevel, Resolved) ||
                !FMath::IsFinite(Resolved.WeightKg) || Resolved.WeightKg < 0.0f)
            {
                return false; // Known invalid identity never borrows old N1.
            }
            Candidate.Weight = Resolved.WeightKg;
            if (Candidate.DisplayName.IsEmpty())
            {
                Candidate.DisplayName = Resolved.DisplayName;
            }
        }
        else
        {
            // Nonregistered items preserve their temporary legacy path,
            // including prior per-definition weapon profile validation.
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
            Candidate.Weight = UnitWeight;
            if (Candidate.DisplayName.IsEmpty())
            {
                Candidate.DisplayName = LegacyDefinition->DisplayName;
            }
        }
        InOutInstance = MoveTemp(Candidate);
        return true;
    }
};
