#pragma once

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"

/** Read-only numerical projections of generic NON-arm throws. Not an action
 * source, not a weapon projectile, never a permission to attack. */
struct FGridNonArmamentThrowReadModel
{
    static bool TryBallistics(
        const UGridNonArmamentDefinitionAsset* Item,
        const FGridItemInstance& Instance, float& OutSpeed, float& OutArc)
    {
        OutSpeed = 0.0f;
        OutArc = 0.0f;
        if (!IsValid(Item) || !Instance.IsValid() ||
            Instance.ItemDefinitionId != Item->ItemId ||
            !FMath::IsFinite(Item->GenericThrow.InitialSpeed) ||
            Item->GenericThrow.InitialSpeed <= KINDA_SMALL_NUMBER ||
            !FMath::IsFinite(Item->GenericThrow.Arc))
        {
            return false;
        }
        OutSpeed = Item->GenericThrow.InitialSpeed;
        OutArc = Item->GenericThrow.Arc;
        return true;
    }

    static FVector ClampedVisualScale(const FGridNonArmamentThrowPolicy& Policy)
    {
        FVector Scale = Policy.VisualRelativeScale;
        if (Scale.ContainsNaN()) { Scale = FVector(1.5f); }
        Scale.X = FMath::Clamp(FMath::Abs(Scale.X), 0.01f, 100.0f);
        Scale.Y = FMath::Clamp(FMath::Abs(Scale.Y), 0.01f, 100.0f);
        Scale.Z = FMath::Clamp(FMath::Abs(Scale.Z), 0.01f, 100.0f);
        return Scale;
    }

    static FVector VisualAngularVelocity(const FGridNonArmamentThrowPolicy& Policy)
    {
        switch (Policy.VisualMode)
        {
        case EGridNonArmamentThrowVisualMode::Stable:
            return FVector::ZeroVector;
        case EGridNonArmamentThrowVisualMode::Spin:
            return FVector(0.0f, 0.0f, FMath::Max(0.0f, Policy.VisualSpinDegreesPerSecond));
        case EGridNonArmamentThrowVisualMode::Tumble:
        default:
        {
            FVector Axis = Policy.VisualTumbleAxis;
            if (Axis.ContainsNaN() || Axis.IsNearlyZero())
            {
                Axis = FVector(0.65f, 0.35f, 1.0f);
            }
            Axis.Normalize();
            return Axis * FMath::Max(0.0f, Policy.VisualTumbleDegreesPerSecond);
        }
        }
    }

    static float FlightLifeSeconds(const FGridNonArmamentThrowPolicy& Policy)
    {
        return FMath::Max(0.01f, Policy.LifeSeconds);
    }

    static float ImpactDropOffset(const FGridNonArmamentThrowPolicy& Policy)
    {
        return FMath::Max(0.0f, Policy.ImpactDropOffset);
    }
};
