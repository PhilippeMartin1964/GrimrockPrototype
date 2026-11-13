#include "ARMEMENT/GridArmClean06ItemIdentityProjection.h"

bool GridArmClean06ItemIdentityProjection::Matches(
	const UGridItemDefinitionAsset& Source,
	const UGridArmamentDefinitionAsset& Destination)
{
	return Source.ItemDefinitionId == Destination.ItemId &&
		Source.WeaponArchetypeId == Destination.ArchetypeId &&
		Source.DisplayName.EqualTo(Destination.DisplayName) &&
		Source.Description.EqualTo(Destination.Description) &&
		Source.Weight == Destination.WeightKg &&
		Source.ItemTags == Destination.Tags;
}

bool GridArmClean06ItemIdentityProjection::TryProject(
	const UGridItemDefinitionAsset& Source, UGridArmamentDefinitionAsset& Destination)
{
	// Only a fresh transient destination is allowed. Do not overwrite a
	// previously authored canonical item and never write the source asset.
	if (!Source.bUseWeaponCatalogue || Source.ItemDefinitionId.IsNone() ||
		Source.WeaponArchetypeId.IsNone() || !FMath::IsFinite(Source.Weight) ||
		Source.Weight < 0.0f || !Destination.ItemId.IsNone() ||
		!Destination.ArchetypeId.IsNone())
	{
		return false;
	}
	Destination.ItemId = Source.ItemDefinitionId;
	Destination.ArchetypeId = Source.WeaponArchetypeId;
	Destination.DisplayName = Source.DisplayName;
	Destination.Description = Source.Description;
	Destination.WeightKg = Source.Weight;
	Destination.Tags = Source.ItemTags;
	return Destination.HasValidIdentity() && Matches(Source, Destination);
}
