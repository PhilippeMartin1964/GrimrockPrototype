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
    // Eligibility for binding is independent of the character's current Strength.
    // Call only with a physical definition resolved by the active registry.
    static bool CanBind(const FGridResolvedItem& Item, const FGridItemInstance& Instance)
    {
        return Instance.IsValid() && Item.IsValid() &&
            Item.ItemId == Instance.ItemDefinitionId && Item.ItemLevel == Instance.ItemLevel &&
            (Item.bIsArmament ? Item.ItemLevel > 0 && Item.Weapon.HandPolicy != EGridHandPolicy::TwoHanded
                              : Item.ItemLevel == 0) &&
            Instance.Quantity <= FMath::Max(1, Item.MaxStack) &&
            FMath::IsFinite(Item.WeightKg) && Item.WeightKg >= 0.0f &&
            FGridPhysicalThrowStrengthReadModel::IsCanonicalOneHanded(Item) &&
            FGridThrowBallisticsReadModel::ReadCanonicalFlight(Item).bResolved;
    }

    static FGridPhysicalThrowPreflightReadView Resolve(
        const UGridItemSubsystem* Registry,
        const FGridItemInstance& Instance, int32 Strength)
    {
        FGridResolvedItem Item;
        const bool bResolved = Registry && Registry->ResolvePhysicalItem(Instance, Item);
        const bool bRegistered = bResolved || (Registry && Registry->ContainsItem(Instance.ItemDefinitionId));
        return Project(Instance, bResolved ? &Item : nullptr, bRegistered, Strength);
    }

    static FGridPhysicalThrowPreflightReadView Project(const FGridItemInstance& Instance,
        const FGridResolvedItem* Item, bool bRegisteredIdentity, int32 Strength)
    {
        FGridPhysicalThrowPreflightReadView Out;
        Out.Ballistics = FGridThrowBallisticsReadModel::Project(Instance, Item, bRegisteredIdentity);
        Out.Strength = FGridPhysicalThrowStrengthReadModel::Project(Instance, Item, bRegisteredIdentity, Strength);
        Out.bRegisteredIdentity = bRegisteredIdentity;
        Out.bResolved = Out.Strength.bResolved && Out.Ballistics.bResolved;
        Out.bCanThrow = Out.bResolved && Out.Strength.bCanThrow &&
            FMath::IsFinite(Out.Ballistics.InitialSpeed) &&
            Out.Ballistics.InitialSpeed > KINDA_SMALL_NUMBER &&
            FMath::IsFinite(Out.Ballistics.Arc);
        return Out;
    }
};
