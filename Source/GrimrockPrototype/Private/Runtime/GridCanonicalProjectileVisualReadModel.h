#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** One read-only flight appearance resolved for the true physical item instance. */
struct FGridCanonicalProjectileVisualReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	bool bHasProjectileVisual = false;
	FName VisualId = NAME_None;
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;
	FGridCanonicalProjectileVisual Appearance;
};

/** A missing N1-N20 level must not read DA_Weapon_* visual fields. */
struct FGridCanonicalProjectileVisualReadModel
{
	static FGridCanonicalProjectileVisualReadView Resolve(
		const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
	{
		FGridCanonicalProjectileVisualReadView Result;
		if (!Registry || !Instance.IsValid() || !Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			return Result;
		}
		Result.bCanonicalIdentity = true;
		FGridResolvedItem Resolved;
		if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Resolved))
		{
			return Result;
		}
		Result.bResolved = true;
		Result.VisualId = Resolved.VisualId;
		Result.bHasProjectileVisual = Resolved.bIsArmament && Resolved.ProjectileAppearance.bEnabled;
		if (Result.bHasProjectileVisual)
		{
			Result.ProjectileMesh = Resolved.ProjectileMesh;
			Result.Appearance = Resolved.ProjectileAppearance;
		}
		return Result;
	}
};
