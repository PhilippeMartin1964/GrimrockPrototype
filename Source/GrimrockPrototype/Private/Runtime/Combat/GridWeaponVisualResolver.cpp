#include "Runtime/Combat/GridWeaponVisualResolver.h"

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	bool HasPresentableResource(const FGridResolvedWeaponVisual& Visual)
	{
		return !Visual.Icon.IsNull() || !Visual.WorldMesh.IsNull() || !Visual.EquippedMesh.IsNull();
	}

	void ApplyLegacyVisual(const UGridItemDefinitionAsset& Definition, FGridResolvedWeaponVisual& OutVisual)
	{
		OutVisual.bLegacyFallback = true;
		OutVisual.VisualId = NAME_None;
		OutVisual.Icon = Definition.Icon;
		OutVisual.WorldMesh = Definition.WorldMesh;
		// Preserve the exact Legacy definition: its existing held-mesh caller
		// is responsible for choosing WorldMesh when EquippedMesh is absent.
		OutVisual.EquippedMesh = Definition.EquippedMesh;
		OutVisual.OrnamentMesh.Reset();
		OutVisual.WorldVisualTransform = FTransform::Identity;
		OutVisual.OrnamentRelativeTransform = FTransform::Identity;
	}

	bool Finish(FGridResolvedWeaponVisual& OutVisual,
		EGridWeaponVisualResolutionError& OutError,
		EGridWeaponVisualResolutionError Error)
	{
		OutVisual.bResolved = HasPresentableResource(OutVisual);
		// Even a Legacy object can legitimately lack artwork. Report that
		// absence instead of returning false with a misleading None error.
		OutError = !OutVisual.bResolved && Error == EGridWeaponVisualResolutionError::None
			? EGridWeaponVisualResolutionError::MissingVisualResource : Error;
		OutVisual.Error = OutError;
		return OutVisual.bResolved;
	}

	bool FinishWithLegacy(const UGridItemDefinitionAsset& Definition,
		FGridResolvedWeaponVisual& OutVisual,
		EGridWeaponVisualResolutionError& OutError,
		EGridWeaponVisualResolutionError Error)
	{
		ApplyLegacyVisual(Definition, OutVisual);
		return Finish(OutVisual, OutError, Error);
	}
}

bool FGridWeaponVisualResolver::TryResolve(
	const UGridItemDefinitionAsset& Definition,
	const FGridItemInstance* OptionalInstance,
	FGridResolvedWeaponVisual& OutVisual,
	EGridWeaponVisualResolutionError& OutError)
{
	OutVisual = FGridResolvedWeaponVisual();
	OutError = EGridWeaponVisualResolutionError::None;

	if (OptionalInstance)
	{
		// A mismatched/invalid instance may still display the definition's
		// neutral Legacy visual, but must never expose a fabricated level.
		if (!OptionalInstance->IsValid() ||
			OptionalInstance->ItemDefinitionId != Definition.ItemDefinitionId)
		{
			return FinishWithLegacy(Definition, OutVisual, OutError,
				EGridWeaponVisualResolutionError::InstanceContextUnavailable);
		}

		if (OptionalInstance->ItemLevel >= 1 && OptionalInstance->ItemLevel <= 20)
		{
			OutVisual.bHasItemLevel = true;
			OutVisual.ItemLevel = OptionalInstance->ItemLevel;
		}
	}

	if (!Definition.bUseWeaponVisualCatalogue)
	{
		return FinishWithLegacy(Definition, OutVisual, OutError,
			EGridWeaponVisualResolutionError::None);
	}

	// Get() is intentionally non-loading. An unassigned or not-yet-loaded
	// catalogue is a presentation error, not a gameplay failure.
	const UGridWeaponVisualCatalogueAsset* Catalogue = Definition.WeaponVisualCatalogue.Get();
	if (!IsValid(Catalogue))
	{
		return FinishWithLegacy(Definition, OutVisual, OutError,
			EGridWeaponVisualResolutionError::CatalogueUnavailable);
	}

	const FName VisualId = Catalogue->FindVisualId(Definition.WeaponArchetypeId);
	if (VisualId.IsNone())
	{
		return FinishWithLegacy(Definition, OutVisual, OutError,
			EGridWeaponVisualResolutionError::MissingBinding);
	}

	const FGridWeaponVisualProfile* Profile = Catalogue->FindVisualProfile(VisualId);
	if (!Profile)
	{
		return FinishWithLegacy(Definition, OutVisual, OutError,
			EGridWeaponVisualResolutionError::UnknownVisualId);
	}

	// This also rejects duplicate IDs elsewhere in the authored catalogue.
	// Invalid visuals never leak a partly assembled configuration.
	if (!Profile->IsValidProfile() || !Catalogue->IsValidCatalogue())
	{
		return FinishWithLegacy(Definition, OutVisual, OutError,
			EGridWeaponVisualResolutionError::MissingVisualResource);
	}

	OutVisual.VisualId = VisualId;
	OutVisual.Icon = Profile->BaseIcon;
	OutVisual.WorldMesh = Profile->WorldMesh;
	OutVisual.EquippedMesh = Profile->EquippedMesh;
	OutVisual.OrnamentMesh = Profile->OrnamentMesh;
	OutVisual.WorldVisualTransform = Profile->WorldVisualTransform;
	OutVisual.OrnamentRelativeTransform = Profile->OrnamentRelativeTransform;

	// Missing channels are allowed by the authoring schema. Reuse their
	// exact Legacy sources rather than inventing or loading substitutes.
	if (OutVisual.Icon.IsNull() && !Definition.Icon.IsNull())
	{
		OutVisual.Icon = Definition.Icon;
		OutVisual.bLegacyFallback = true;
	}
	if (OutVisual.WorldMesh.IsNull() && !Definition.WorldMesh.IsNull())
	{
		OutVisual.WorldMesh = Definition.WorldMesh;
		OutVisual.bLegacyFallback = true;
	}
	if (OutVisual.EquippedMesh.IsNull())
	{
		// EquippedMesh is optional: prefer the current resolved world mesh.
		// A dedicated Legacy equipped mesh is used only if no visual mesh is
		// available, so a new silhouette is not paired with an unrelated one.
		if (!OutVisual.WorldMesh.IsNull())
		{
			OutVisual.EquippedMesh = OutVisual.WorldMesh;
		}
		else if (!Definition.EquippedMesh.IsNull())
		{
			OutVisual.EquippedMesh = Definition.EquippedMesh;
			OutVisual.bLegacyFallback = true;
		}
	}

	if (!HasPresentableResource(OutVisual))
	{
		return FinishWithLegacy(Definition, OutVisual, OutError,
			EGridWeaponVisualResolutionError::MissingVisualResource);
	}
	return Finish(OutVisual, OutError, EGridWeaponVisualResolutionError::None);
}
