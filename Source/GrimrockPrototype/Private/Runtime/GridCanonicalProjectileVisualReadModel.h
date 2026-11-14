#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** One read-only flight appearance resolved for the true physical item instance. */
struct FGridCanonicalProjectileVisualReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	bool bHasProjectileVisual = false;
	bool bIsAmmunition = false;
	FName VisualId = NAME_None;
	TSoftObjectPtr<UStaticMesh> WorldMesh;
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;
	FGridCanonicalProjectileVisual Appearance;
	FGridCanonicalProjectilePolicy FlightPolicy;
};

/** No synthetic N1: armaments require N1-N20, ammunition retains canonical N0. */
struct FGridCanonicalProjectileVisualReadModel
{
	static FGridCanonicalProjectileVisualReadView Resolve(
		const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
	{
		FGridCanonicalProjectileVisualReadView Result;
		// Membership is determined by ItemId before inspecting the persisted
		// instance: an invalid N-1, GUID or quantity must never cause a legacy
		// fallback for an already registered canonical identity.
		if (!Registry || !Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			return Result;
		}
		Result.bCanonicalIdentity = true;
		if (!Instance.IsValid())
		{
			return Result;
		}
		FGridResolvedItem Resolved;
		if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Resolved))
		{
			return Result;
		}
		Result.bResolved = true;
		Result.VisualId = Resolved.VisualId;
		Result.bIsAmmunition = !Resolved.AmmoKindId.IsNone() && !Resolved.bIsArmament;
		if (Result.bIsAmmunition)
		{
			// Ammo flight renders its single authored world mesh; only armaments
			// have a dedicated visual flight profile in WeaponVisualCatalogue.
			Result.WorldMesh = Resolved.WorldMesh;
		}
		Result.bHasProjectileVisual = Resolved.bIsArmament && Resolved.ProjectileAppearance.bEnabled;
		if (Result.bHasProjectileVisual)
		{
			Result.ProjectileMesh = Resolved.ProjectileMesh;
			Result.Appearance = Resolved.ProjectileAppearance;
			Result.FlightPolicy = Resolved.Weapon.Projectile;
		}
		return Result;
	}
};
