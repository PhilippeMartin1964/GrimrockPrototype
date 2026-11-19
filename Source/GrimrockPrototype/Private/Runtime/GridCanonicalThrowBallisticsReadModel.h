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
		// Membership must be recognized BEFORE IsValid(): an invalid
		// registered N21 is not an unknown legacy identity.
		Out.bCanonicalIdentity = Registry && Registry->HasCanonicalItem(Instance.ItemDefinitionId);
		if (!Instance.IsValid())
		{
			return Out;
		}
		if (Out.bCanonicalIdentity)
		{
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical))
			{
				return Out; // No synthetic N1 or old projectile for a known invalid ARM.
			}
			if (!Canonical.bIsArmament)
			{
				if (Instance.ItemLevel != 0 || !FMath::IsFinite(Canonical.GenericThrow.InitialSpeed) ||
					Canonical.GenericThrow.InitialSpeed <= KINDA_SMALL_NUMBER || !FMath::IsFinite(Canonical.GenericThrow.Arc))
					return Out;
				Out.bResolved = true;
				Out.InitialSpeed = Canonical.GenericThrow.InitialSpeed;
				Out.Arc = Canonical.GenericThrow.Arc;
				return Out;
			}
			if (Canonical.Weapon.Projectile.bEnabled || Canonical.Weapon.Projectile.bGenericThrowAuthored)
			{
				if (!FMath::IsFinite(Canonical.Weapon.Projectile.InitialSpeed) ||
					Canonical.Weapon.Projectile.InitialSpeed <= KINDA_SMALL_NUMBER ||
					!FMath::IsFinite(Canonical.Weapon.Projectile.Arc))
				{
					return Out;
				}
				Out.bResolved = true;
				Out.bSpecializedProjectile = Canonical.Weapon.Projectile.bEnabled;
				Out.InitialSpeed = Canonical.Weapon.Projectile.InitialSpeed;
				Out.Arc = Canonical.Weapon.Projectile.Arc;
				return Out;
			}
			// 5D.3 materialized the ordinary policies. Missing canonical
			// flight is a refusal, never a historical fallback.
			return Out;
		}
		if (!IsValid(LegacyDefinition) ||
			LegacyDefinition->ItemDefinitionId != Instance.ItemDefinitionId)
		{
			return Out;
		}
		// Historical flight values remain authoritative only if physically
		// usable. Keep signed finite arcs: the launcher already clamps them.
		if (!FMath::IsFinite(LegacyDefinition->ThrowSpeed) ||
			LegacyDefinition->ThrowSpeed <= KINDA_SMALL_NUMBER ||
			!FMath::IsFinite(LegacyDefinition->ThrowArc))
		{
			return Out;
		}
		Out.bResolved = true;
		Out.InitialSpeed = LegacyDefinition->ThrowSpeed;
		Out.Arc = LegacyDefinition->ThrowArc;
		return Out;
	}
};
