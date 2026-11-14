#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Transient flight numbers; the strength-to-weight gate remains a separate policy. */
struct FGridCanonicalThrowBallisticsReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	bool bSpecializedProjectile = false;
	float InitialSpeed = 0.0f;
	float Arc = 0.0f;
};

struct FGridCanonicalThrowBallisticsReadModel
{
	static FGridCanonicalThrowBallisticsReadView Resolve(
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance)
	{
		FGridCanonicalThrowBallisticsReadView Out;
		if (!IsValid(LegacyDefinition) || !Instance.IsValid() ||
			Instance.ItemDefinitionId != LegacyDefinition->ItemDefinitionId)
		{
			return Out;
		}
		if (LegacyDefinition->bUseWeaponCatalogue && Registry &&
			Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			Out.bCanonicalIdentity = true;
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) ||
				!Canonical.bIsArmament)
			{
				return Out; // No synthetic N1 or old projectile for a known invalid ARM.
			}
			if (Canonical.Weapon.Projectile.bEnabled)
			{
				if (!FMath::IsFinite(Canonical.Weapon.Projectile.InitialSpeed) ||
					Canonical.Weapon.Projectile.InitialSpeed <= KINDA_SMALL_NUMBER ||
					!FMath::IsFinite(Canonical.Weapon.Projectile.Arc))
				{
					return Out;
				}
				Out.bResolved = true;
				Out.bSpecializedProjectile = true;
				Out.InitialSpeed = Canonical.Weapon.Projectile.InitialSpeed;
				Out.Arc = Canonical.Weapon.Projectile.Arc;
				return Out;
			}
			// An ordinary one-handed ARM can still be physically tossed; the
			// N1-N20 catalogue intentionally does not author a specialized projectile.
		}
		Out.bResolved = true;
		Out.InitialSpeed = LegacyDefinition->ThrowSpeed;
		Out.Arc = LegacyDefinition->ThrowArc;
		return Out;
	}
};
