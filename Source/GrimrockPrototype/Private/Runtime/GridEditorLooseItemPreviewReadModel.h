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
		if (!Registry || !IsValid(Definition) ||
			!Registry->HasCanonicalItem(Definition->ItemDefinitionId))
		{
			return View; // Only unregistered items may use the legacy visual reader.
		}
		View.bCanonicalIdentity = true;
		// Use the EXACT authored-placement policy shared with runtime.
		// The old bUseWeaponCatalogue flag must not override the canonical
		// ARM family, otherwise an editor preview can disagree with PIE.
		const FGridReceptacleItemMetadataReadView Level =
			FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
				Registry, Definition, Definition->ItemDefinitionId, Placement.ItemLevel);
		if (!Level.bResolved)
		{
			return View; // Known invalid N0/default: no historical preview fallback.
		}
		View.EffectiveItemLevel = Level.FixedItemLevel;
		FGridResolvedItem Canonical;
		if (!Registry->ResolveItem(Definition->ItemDefinitionId, View.EffectiveItemLevel, Canonical))
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
