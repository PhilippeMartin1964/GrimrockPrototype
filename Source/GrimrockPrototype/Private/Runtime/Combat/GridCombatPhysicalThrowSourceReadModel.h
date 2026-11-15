#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.3BE — one preflight for the existing Attack executor's
 * physically consumed source. This does not add actions, choose targets,
 * or generate SourceItemQuantityCost: only the canonical ActionCatalogue
 * can authorize a registered weapon's one-unit combat throw.
 */
struct FGridCombatPhysicalThrowSourceReadView
{
    bool bCanonicalIdentity = false;
    bool bResolved = false;
    bool bLaunchPhysicalProjectile = false;
    bool bRejectAction = false;
};

struct FGridCombatPhysicalThrowSourceReadModel
{
    static FGridCombatPhysicalThrowSourceReadView Resolve(
        const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* Legacy,
        const FGridItemInstance& Instance,
        int32 AuthoredSourceItemQuantityCost,
        bool bRangedAttack)
    {
        FGridCombatPhysicalThrowSourceReadView Out;
        Out.bCanonicalIdentity = Registry &&
            Registry->HasCanonicalItem(Instance.ItemDefinitionId);

        if (Out.bCanonicalIdentity)
        {
            FGridResolvedItem Weapon;
            if (!Instance.IsValid() ||
                !Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Weapon) ||
                !Weapon.bIsArmament)
            {
                Out.bRejectAction = true; // Invalid known N0/N21, never legacy.
                return Out;
            }
            Out.bResolved = true;
            if (!Weapon.Weapon.Projectile.bEnabled)
            {
                return Out; // A stale bCombatThrowWeapon never enables a throw.
            }
            if (!FMath::IsFinite(Weapon.Weapon.Projectile.InitialSpeed) ||
                Weapon.Weapon.Projectile.InitialSpeed <= KINDA_SMALL_NUMBER ||
                !FMath::IsFinite(Weapon.Weapon.Projectile.Arc) ||
                Weapon.ProjectileMesh.IsNull())
            {
                Out.bRejectAction = true;
                return Out;
            }
            // Standard physical throws authored by the catalogue cost ONE
            // object. A ranged throw with a missing/inconsistent cost must
            // fail closed rather than apply damage without a projectile.
            if (AuthoredSourceItemQuantityCost != 1)
            {
                Out.bRejectAction = bRangedAttack ||
                    AuthoredSourceItemQuantityCost > 0;
                return Out;
            }
            Out.bLaunchPhysicalProjectile = true;
            return Out;
        }

        // Unknown items preserve their pre-migration combat behavior.
        // Keep the legacy quantity override but only for a matching ID.
        Out.bResolved = IsValid(Legacy) && Legacy->ItemDefinitionId == Instance.ItemDefinitionId;
        Out.bLaunchPhysicalProjectile = Out.bResolved && Legacy->IsCombatThrowable();
        return Out;
    }
};
