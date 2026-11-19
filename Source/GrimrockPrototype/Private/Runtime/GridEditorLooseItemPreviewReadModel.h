#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridWorldItemLevelReadModel.h"

/** A preview is an authored placement, not yet a persisted FGridItemInstance. */
struct FGridEditorLooseItemPreviewReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	int32 EffectiveItemLevel = 0;
	FName VisualId = NAME_None;
	TSoftObjectPtr<UStaticMesh> WorldMesh;
	FTransform WorldTransform = FTransform::Identity;
};

/** Read only: same ItemSystem as runtime, no synthetic weapon level on ammo. */
struct FGridEditorLooseItemPreviewReadModel
{
	static FGridEditorLooseItemPreviewReadView Resolve(
		const UGridItemSubsystem* Registry, const FGridLooseItemInstance& Placement)
	{
		FGridEditorLooseItemPreviewReadView View;
		const UGridItemDefinitionAsset* Definition = Placement.ItemDefinition.Get();
		const bool bPersistedCanonical = !Placement.CanonicalItemId.IsNone();
		const FName ItemId = bPersistedCanonical ? Placement.CanonicalItemId :
			(IsValid(Definition) ? Definition->ItemDefinitionId : NAME_None);
		// A persisted canonical identity may NEVER fall back to the old mesh,
		// even when the ItemSystem is unavailable or the ItemId is wrong.
		View.bCanonicalIdentity = bPersistedCanonical ||
			(Registry && Registry->HasCanonicalItem(ItemId));
		if (!View.bCanonicalIdentity || !Registry || !Registry->HasCanonicalItem(ItemId))
		{
			return View; // Only unregistered, non-canonical source uses legacy art.
		}
		if (bPersistedCanonical &&
			(!Placement.InstanceId.IsValid() || Placement.Quantity < 1 ||
			 (IsValid(Definition) && Definition->ItemDefinitionId != ItemId)))
		{
			return View; // Contradictory source or invalid physical placement.
		}
		// All registered identities require physical ItemLevel N1..20 / N0,
		// regardless of whether the source still carries a legacy pointer.
		const FGridReceptacleItemMetadataReadView Level = bPersistedCanonical
			? FGridWorldItemLevelReadModel::ResolvePersistedInstance(
				Registry, nullptr, ItemId, Placement.ItemLevel)
			: FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
				Registry, Definition, ItemId, Placement.ItemLevel);
		if (!Level.bResolved)
		{
			return View; // Known invalid physical level: no historical preview fallback.
		}
		View.EffectiveItemLevel = Level.FixedItemLevel;
		FGridResolvedItem Canonical;
		if (!Registry->ResolveItem(ItemId, View.EffectiveItemLevel, Canonical))
		{
			return View;
		}
		View.bResolved = true;
		View.VisualId = Canonical.VisualId;
		View.WorldMesh = Canonical.WorldMesh;
		View.WorldTransform = Canonical.WorldVisualTransform;
		return View;
	}
};
