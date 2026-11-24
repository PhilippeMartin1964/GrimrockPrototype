#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Transient flight numbers; the strength-to-weight gate remains a separate policy. */
struct FGridThrowBallisticsReadView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	bool bSpecializedProjectile = false;
	float InitialSpeed = 0.0f;
	float Arc = 0.0f;
};

struct FGridThrowBallisticsReadModel
{
	static FGridThrowBallisticsReadView ReadCanonicalFlight(const FGridResolvedItem& Canonical)
	{
		FGridThrowBallisticsReadView Out;
		Out.bRegisteredIdentity = true;
		if (!Canonical.bIsArmament)
		{
			if (Canonical.ItemLevel != 0 || !FMath::IsFinite(Canonical.GenericThrow.InitialSpeed) ||
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
		return Out;
	}

	static FGridThrowBallisticsReadView Resolve(
		const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance)
	{
		FGridResolvedItem Item;
		const bool bResolved = Registry && Registry->ResolvePhysicalItem(Instance, Item);
		const bool bRegistered = bResolved || (Registry && Registry->ContainsItem(Instance.ItemDefinitionId));
		return Project(Instance, bResolved ? &Item : nullptr, bRegistered);
	}

	static FGridThrowBallisticsReadView Project(const FGridItemInstance& Instance,
		const FGridResolvedItem* Item, bool bRegisteredIdentity)
	{
		if (Item && Instance.Quantity <= FMath::Max(1, Item->MaxStack))
		{
			return ReadCanonicalFlight(*Item);
		}
		FGridThrowBallisticsReadView Out;
		Out.bRegisteredIdentity = bRegisteredIdentity;
		return Out;
	}
};
