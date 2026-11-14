#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

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
		// Mirrors AGridLevelRuntimeActor::AddPlacedItemActor exactly:
		// authored N0 inherits default ONCE for an ARM; ammo N0 is meaningful.
		View.EffectiveItemLevel =
			Definition->bUseWeaponCatalogue && Placement.ItemLevel == 0
			? Definition->DefaultWeaponItemLevel : Placement.ItemLevel;
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
