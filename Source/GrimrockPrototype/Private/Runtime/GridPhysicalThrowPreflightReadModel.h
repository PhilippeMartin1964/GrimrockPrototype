#pragma once

#include "GridCanonicalThrowBallisticsReadModel.h"
#include "GridPhysicalThrowStrengthReadModel.h"

/** Same authored throw preflight for context menu and Pawn execution.
 * Registered items exclusively use materialized flight policies.
 * Historical definitions supply flight only in registry-less Automation fixtures.
 */
struct FGridPhysicalThrowPreflightReadView
{
    bool bCanonicalIdentity = false;
    bool bResolved = false;
    bool bCanThrow = false;
    FGridPhysicalThrowStrengthReadView Strength;
    FGridCanonicalThrowBallisticsReadView Ballistics;
};

struct FGridPhysicalThrowPreflightReadModel
{
    static FGridPhysicalThrowPreflightReadView Resolve(
        const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* Legacy,
        const FGridItemInstance& Instance, int32 Strength)
    {
        FGridPhysicalThrowPreflightReadView Out;
        Out.Ballistics =
            FGridCanonicalThrowBallisticsReadModel::Resolve(Registry, Legacy, Instance);
        Out.Strength =
            FGridPhysicalThrowStrengthReadModel::Resolve(Registry, Legacy, Instance, Strength);
        Out.bCanonicalIdentity =
            Out.Ballistics.bCanonicalIdentity || Out.Strength.bCanonicalIdentity;
        Out.bResolved = Out.Strength.bResolved && Out.Ballistics.bResolved;
        Out.bCanThrow = Out.bResolved && Out.Strength.bCanThrow &&
            FMath::IsFinite(Out.Ballistics.InitialSpeed) &&
            Out.Ballistics.InitialSpeed > KINDA_SMALL_NUMBER &&
            FMath::IsFinite(Out.Ballistics.Arc);
        return Out;
    }
};
