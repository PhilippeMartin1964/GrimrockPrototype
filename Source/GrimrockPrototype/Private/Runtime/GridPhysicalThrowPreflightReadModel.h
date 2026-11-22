#pragma once

#include "GridThrowBallisticsReadModel.h"
#include "GridPhysicalThrowStrengthReadModel.h"

/** Same authored throw preflight for context menu and Pawn execution.
 * Registered items exclusively use materialized flight policies.
 * Historical definitions supply flight only in registry-less Automation fixtures.
 */
struct FGridPhysicalThrowPreflightReadView
{
    bool bRegisteredIdentity = false;
    bool bResolved = false;
    bool bCanThrow = false;
    FGridPhysicalThrowStrengthReadView Strength;
    FGridThrowBallisticsReadView Ballistics;
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
            FGridThrowBallisticsReadModel::Resolve(Registry, Legacy, Instance);
        Out.Strength =
            FGridPhysicalThrowStrengthReadModel::Resolve(Registry, Legacy, Instance, Strength);
        Out.bRegisteredIdentity =
            Out.Ballistics.bRegisteredIdentity || Out.Strength.bRegisteredIdentity;
        Out.bResolved = Out.Strength.bResolved && Out.Ballistics.bResolved;
        Out.bCanThrow = Out.bResolved && Out.Strength.bCanThrow &&
            FMath::IsFinite(Out.Ballistics.InitialSpeed) &&
            Out.Ballistics.InitialSpeed > KINDA_SMALL_NUMBER &&
            FMath::IsFinite(Out.Ballistics.Arc);
        return Out;
    }
};
