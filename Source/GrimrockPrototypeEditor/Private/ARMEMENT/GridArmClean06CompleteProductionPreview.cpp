#include "ARMEMENT/GridArmClean06CompleteProductionPreview.h"

#include "ARMEMENT/GridArmClean06ProjectilePolicyProjection.h"
#include "ARMEMENT/GridArmClean06ProjectileVisualProjection.h"
#include "Runtime/GridItemDefinitionAsset.h"

bool GridArmClean06CompleteProductionPreview::Build(
	const TArray<FGridWeaponPipelineRow>& Manifest,
	const UGridWeaponCatalogueAsset& NumericSource,
	const UGridWeaponVisualCatalogueAsset& VisualSource,
	const UGridEquippedWeaponActionIconPalette& IconSource,
	UGridItemSystemAsset& TransientRoot, FSummary& OutSummary)
{
	OutSummary = FSummary();
	if (Manifest.Num() != 22 ||
		!GridArmClean06FullProductionCandidate::Build(Manifest,
			NumericSource, VisualSource, IconSource, TransientRoot,
			OutSummary.Core))
	{
		return false;
	}
	// This is the already-authored Item_Arrows_01; no new ammunition identity.
	if (!GridArmClean06ProductionAmmoCandidate::AddExistingArrows(
			Manifest, TransientRoot, OutSummary.Core, OutSummary.Ammo))
	{
		return false;
	}

	TSet<FName> SeenArchetypes;
	TSet<FName> SeenVisualIds;
	const int32 InitiallyUnconverted = OutSummary.Core.UnconvertedThrowPolicies;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ArmId(*Row.ArchetypeId);
		const FName VisualId(*Row.VisualId);
		if (Row.ProductionState != TEXT("Active") ||
			SeenArchetypes.Contains(ArmId) || SeenVisualIds.Contains(VisualId))
		{
			return false;
		}
		SeenArchetypes.Add(ArmId);
		SeenVisualIds.Add(VisualId);
		const UGridItemDefinitionAsset* Source =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		FGridCanonicalWeaponArchetype* Arm =
			TransientRoot.WeaponCatalogue->Archetypes.FindByPredicate(
				[ArmId](const FGridCanonicalWeaponArchetype& Entry)
				{
					return Entry.ArchetypeId == ArmId;
				});
		FGridCanonicalWeaponVisual* Visual =
			TransientRoot.WeaponVisualCatalogue->VisualProfiles.FindByPredicate(
				[VisualId](const FGridCanonicalWeaponVisual& Entry)
				{
					return Entry.VisualId == VisualId;
				});
		if (!IsValid(Source) || !Arm || !Visual ||
			Source->ItemDefinitionId != FName(*Row.ItemDefinitionId))
		{
			return false;
		}
		const bool bWasDirty = Source->GetOutermost()->IsDirty();
		if (!GridArmClean06ProjectilePolicyProjection::TryProject(
				Row, *Source, *Arm) ||
			!GridArmClean06ProjectileVisualProjection::TryProject(
				Row, *Source, *Visual) ||
			!GridArmClean06ProjectilePolicyProjection::Matches(
				*Source, Arm->Projectile) ||
			!GridArmClean06ProjectileVisualProjection::Matches(
				*Source, *Visual))
		{
			return false;
		}
		if (Source->bCombatThrowWeapon)
		{
			++OutSummary.PhysicalThrowProfiles;
			++OutSummary.VisualThrowProfiles;
		}
		OutSummary.UnresolvedSourceActions += FMath::Max(
			0, Source->CombatActions.Num() - (Row.IsPassive() ? 0 : 1));
		if (Source->GetOutermost()->IsDirty() != bWasDirty)
		{
			return false;
		}
	}
	if (SeenArchetypes.Num() != 22 || SeenVisualIds.Num() != 22 ||
		InitiallyUnconverted != OutSummary.PhysicalThrowProfiles ||
		OutSummary.PhysicalThrowProfiles != OutSummary.VisualThrowProfiles ||
		OutSummary.PhysicalThrowProfiles != 3 ||
		OutSummary.UnresolvedSourceActions !=
			OutSummary.Core.Actions.OtherLocalActionsUnconverted ||
		OutSummary.Ammo.CanonicalAmmoDefinitions != 1 ||
		TransientRoot.ItemDefinitions.Num() != 23)
	{
		return false;
	}
	TArray<FString> VisualErrors;
	TArray<FString> WeaponErrors;
	TArray<FString> RegistryErrors;
	OutSummary.bVisualCatalogueValid =
		TransientRoot.WeaponVisualCatalogue->Validate(VisualErrors);
	OutSummary.bWeaponCatalogueValid =
		TransientRoot.WeaponCatalogue->Validate(WeaponErrors);
	if (!OutSummary.bVisualCatalogueValid)
	{
		return false;
	}
	// An offensive ARM with a proven hand but NO authored ActionId/PresentationId
	// is correctly rejected by the canonical weapon validator. That expected
	// blocker must be visible in the preview, not misreported as numeric loss.
	// Every other validation error remains a fatal mismatch.
	if (!OutSummary.bWeaponCatalogueValid)
	{
		if (OutSummary.Core.UnboundOffensiveArmaments <= 0 ||
			WeaponErrors.Num() != OutSummary.Core.UnboundOffensiveArmaments)
		{
			return false;
		}
		for (const FString& WeaponError : WeaponErrors)
		{
			if (WeaponError != TEXT("Weapon catalogue: attack source without action binding"))
			{
				return false;
			}
		}
	}

	// All physical/visual policies were proven; the original action/presentation
	// blockers remain. Do not declare overall readiness on the basis of art.
	OutSummary.Core.UnconvertedThrowPolicies = 0;
	OutSummary.bRegistryValidAfterProjection =
		TransientRoot.Validate(RegistryErrors);
	OutSummary.Core.bRegistryValid =
		OutSummary.bRegistryValidAfterProjection;
	return true;
}
