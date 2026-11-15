#pragma once

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"

/**
 * ARM-CLEAN06.4M — opt-in non-armament read-only policy views. No legacy
 * definition, weapon catalogue, UObject mutation or SaveGame conversion.
 * Unregistered old items continue using their established runtime readers
 * until the guarded binary migration.
 */
struct FGridCanonicalNonArmamentReadPolicies
{
    static bool CanRead(const UGridNonArmamentDefinitionAsset* Definition,
        const FGridItemInstance& Instance)
    {
        if (!IsValid(Definition) || !Instance.IsValid() ||
            Definition->ItemId != Instance.ItemDefinitionId)
        {
            return false;
        }
        // Instance state takes precedence; an explicit readable capability
        // preserves Book / Scroll with empty default ReadText.
        return !Instance.ReadTextOverride.IsEmpty() ||
            (IsValid(Instance.ReadableContentAsset) &&
                !Instance.ReadableContentAsset->BodyText.IsEmpty()) ||
            Definition->bReadable || !Definition->ReadText.IsEmpty();
    }

    static bool TryResolveWorldPickupUnitMass(
        const UGridNonArmamentDefinitionAsset* Definition,
        const FGridItemInstance& Instance, float& OutMassKg)
    {
        OutMassKg = 0.0f;
        if (!IsValid(Definition) || !Instance.IsValid() ||
            Definition->ItemId != Instance.ItemDefinitionId ||
            !Definition->bUseItemWeightAsWorldPhysicsMass ||
            !FMath::IsFinite(Definition->WeightKg) ||
            Definition->WeightKg <= KINDA_SMALL_NUMBER)
        {
            return false;
        }
        // Quantity does not affect the mass of a single physical pickup.
        OutMassKg = Definition->WeightKg;
        return true;
    }

    static float InitialPlacementTiltDegrees(
        const UGridNonArmamentDefinitionAsset* Definition)
    {
        if (!IsValid(Definition) ||
            !FMath::IsFinite(Definition->WorldPhysicsInitialTiltDegrees))
        {
            return 0.0f;
        }
        const float Degrees = FMath::Clamp(
            Definition->WorldPhysicsInitialTiltDegrees, 0.0f, 45.0f);
        return Degrees > KINDA_SMALL_NUMBER ? Degrees : 0.0f;
    }
};
