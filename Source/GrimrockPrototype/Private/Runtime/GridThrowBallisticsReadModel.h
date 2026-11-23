#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
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
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance)
	{
		FGridThrowBallisticsReadView Out;
		// Membership must be recognized BEFORE IsValid(): an invalid
		// registered N21 is not an unknown legacy identity.
		Out.bRegisteredIdentity = Registry && Registry->ContainsItem(Instance.ItemDefinitionId);
		if (!Instance.IsValid())
		{
			return Out;
		}
		if (Out.bRegisteredIdentity)
		{
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) ||
				!Canonical.IsValid() || Canonical.ItemId != Instance.ItemDefinitionId ||
				Canonical.ItemLevel != Instance.ItemLevel || Instance.Quantity > FMath::Max(1, Canonical.MaxStack))
			{
				return Out; // No synthetic N1 or old projectile for a known invalid ARM.
			}
			return ReadCanonicalFlight(Canonical);
		}
		// Only active, registry-less Automation fixtures may use old physics.
#if !WITH_DEV_AUTOMATION_TESTS
		return Out;
#else
		if (!GIsAutomationTesting || Registry) return Out;
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
#endif
	}
};
