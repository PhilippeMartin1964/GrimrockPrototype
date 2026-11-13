#include "ARMEMENT/GridArmClean06ProductionCandidate.h"

#include "ARMEMENT/GridArmClean06ActionIconsProjection.h"
#include "ARMEMENT/GridArmClean06ItemIdentityProjection.h"
#include "ARMEMENT/GridArmClean06VisualProjection.h"
#include "Runtime/GridItemDefinitionAsset.h"

bool GridArmClean06ProductionCandidate::BuildDefinitionsAndVisuals(
	const TArray<FGridWeaponPipelineRow>& Manifest,
	const UGridWeaponVisualCatalogueAsset& LegacyVisuals,
	const UGridEquippedWeaponActionIconPalette& LegacyIcons,
	UGridItemSystemAsset& Destination, FSummary& OutSummary)
{
	OutSummary = FSummary();
	if (Manifest.Num() != 22 || !LegacyVisuals.IsValidCatalogue() ||
		!Destination.ItemDefinitions.IsEmpty() || Destination.WeaponVisualCatalogue ||
		Destination.WeaponCatalogue || Destination.ActionCatalogue ||
		Destination.ActionPresentationCatalogue)
	{
		return false;
	}

	UGridCanonicalWeaponVisualCatalogueAsset* Visuals =
		NewObject<UGridCanonicalWeaponVisualCatalogueAsset>(&Destination);
	TSet<FName> ItemIds;
	TSet<FName> ArchetypeIds;
	TSet<FName> VisualIds;
	auto Reject = [&Destination, &OutSummary]() -> bool
	{
		Destination.ItemDefinitions.Reset();
		Destination.WeaponVisualCatalogue = nullptr;
		OutSummary = FSummary();
		return false;
	};

	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ArmId(*Row.ArchetypeId);
		const FName ItemId(*Row.ItemDefinitionId);
		const FName VisualId(*Row.VisualId);
		if (Row.ProductionState != TEXT("Active") || ItemId.IsNone() ||
			ArmId.IsNone() || VisualId.IsNone() ||
			ItemIds.Contains(ItemId) || ArchetypeIds.Contains(ArmId))
		{
			return Reject();
		}
		ItemIds.Add(ItemId);
		ArchetypeIds.Add(ArmId);

		const UGridItemDefinitionAsset* Source =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		const FGridWeaponVisualProfile* LegacyVisual =
			LegacyVisuals.FindProfileForArchetype(ArmId);
		if (!IsValid(Source) || Source->ItemDefinitionId != ItemId ||
			Source->WeaponArchetypeId != ArmId ||
			!Source->bUseWeaponCatalogue || !Source->bUseWeaponVisualCatalogue ||
			!LegacyVisual || LegacyVisual->VisualId != VisualId)
		{
			return Reject();
		}
		const bool bSourceDirty = Source->GetOutermost()->IsDirty();
		UGridArmamentDefinitionAsset* Item =
			NewObject<UGridArmamentDefinitionAsset>(&Destination);
		if (!GridArmClean06ItemIdentityProjection::TryProject(*Source, *Item) ||
			!GridArmClean06ItemIdentityProjection::Matches(*Source, *Item))
		{
			return Reject();
		}
		Destination.ItemDefinitions.Add(Item);
		++OutSummary.ItemDefinitions;
		if (!Item->HasValidIdentity()) { ++OutSummary.NonCanonicalIdentities; }
		if (Row.IsPassive()) { ++OutSummary.PassiveDefinitions; }

		FGridCanonicalWeaponVisualBinding Binding;
		Binding.ArchetypeId = ArmId;
		Binding.VisualId = VisualId;
		Visuals->ArchetypeBindings.Add(Binding);
		if (!VisualIds.Contains(VisualId))
		{
			FGridCanonicalWeaponVisual Profile;
			if (!GridArmClean06VisualProjection::TryProject(*LegacyVisual, Profile))
			{
				return Reject();
			}
			VisualIds.Add(VisualId);
			Visuals->VisualProfiles.Add(Profile);
			++OutSummary.UniqueVisualProfiles;
		}
		if (Source->GetOutermost()->IsDirty() != bSourceDirty)
		{
			return Reject();
		}
	}

	if (!GridArmClean06ActionIconsProjection::TryProject(LegacyIcons, *Visuals) ||
		!GridArmClean06ActionIconsProjection::IsIdentical(LegacyIcons, *Visuals))
	{
		return Reject();
	}
	TArray<FString> Errors;
	if (OutSummary.ItemDefinitions != 22 || OutSummary.PassiveDefinitions != 3 ||
		Visuals->ArchetypeBindings.Num() != 22 || !Visuals->Validate(Errors))
	{
		return Reject();
	}
	Destination.WeaponVisualCatalogue = Visuals;
	return true;
}
