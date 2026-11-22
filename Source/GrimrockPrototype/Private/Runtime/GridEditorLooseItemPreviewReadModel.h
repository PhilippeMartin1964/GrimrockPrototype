#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** A preview is an authored placement, not yet a persisted FGridItemInstance. */
struct FGridEditorLooseItemPreviewReadView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	int32 EffectiveItemLevel = 0;
	FName VisualId = NAME_None;
	TSoftObjectPtr<UStaticMesh> WorldMesh;
	FTransform WorldTransform = FTransform::Identity;
};

/** The preview uses the same strictly physical identity as the native ItemSystem.
 * Invalid placement data cannot display any historical mesh.
 */
struct FGridEditorLooseItemPreviewReadModel
{
	static FGridEditorLooseItemPreviewReadView Resolve(
		const UGridItemSubsystem* Registry, const FGridLooseItemInstance& Placement)
	{
		FGridEditorLooseItemPreviewReadView View;
		View.bRegisteredIdentity = !Placement.CanonicalItemId.IsNone();
		if (!View.bRegisteredIdentity || !IsValid(Registry) || !Registry->IsConfigured() ||
			!Placement.InstanceId.IsValid() || Placement.Quantity < 1)
			return View;

		FGridResolvedItem Canonical;
		if (!Registry->ResolveItem(Placement.CanonicalItemId, Placement.ItemLevel, Canonical) ||
			!Canonical.IsValid() || Canonical.ItemId != Placement.CanonicalItemId ||
			Canonical.ItemLevel != Placement.ItemLevel)
			return View;

		View.bResolved = true;
		View.EffectiveItemLevel = Canonical.ItemLevel;
		View.VisualId = Canonical.VisualId;
		View.WorldMesh = Canonical.WorldMesh;
		View.WorldTransform = Canonical.WorldVisualTransform;
		return View;
	}
};
