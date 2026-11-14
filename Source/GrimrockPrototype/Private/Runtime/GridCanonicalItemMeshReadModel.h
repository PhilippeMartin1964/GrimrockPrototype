#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Soft references for an actual canonical item instance, not a synthetic N1. */
struct FGridCanonicalItemMeshReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	FName VisualId = NAME_None;
	TSoftObjectPtr<UStaticMesh> WorldMesh;
	TSoftObjectPtr<UStaticMesh> EquippedMesh;
};

/** Never consults legacy copies; noncanonical callers keep their original path. */
struct FGridCanonicalItemMeshReadModel
{
	static FGridCanonicalItemMeshReadView Resolve(const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance)
	{
		FGridCanonicalItemMeshReadView Result;
		if (!Instance.IsValid() || !Registry ||
			!Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			return Result;
		}
		Result.bCanonicalIdentity = true;
		FGridResolvedItem Item;
		if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Item))
		{
			return Result;
		}
		Result.bResolved = true;
		Result.VisualId = Item.VisualId;
		Result.WorldMesh = Item.WorldMesh;
		// UGridItemSubsystem already authoritatively maps absent EquippedMesh
		// onto WorldMesh. Never copy an old DA_Weapon's mesh here.
		Result.EquippedMesh = Item.EquippedMesh;
		return Result;
	}
};
