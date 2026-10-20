#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"

bool UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(FName ArchetypeId)
{
	if (ArchetypeId.IsNone())
	{
		return false;
	}

	const FString Name = ArchetypeId.ToString();
	if (Name.Len() != 6 || !Name.StartsWith(TEXT("ARM"), ESearchCase::CaseSensitive))
	{
		return false;
	}

	int32 Ordinal = 0;
	for (int32 Index = 3; Index < 6; ++Index)
	{
		if (Name[Index] < TEXT('0') || Name[Index] > TEXT('9'))
		{
			return false;
		}
		Ordinal = Ordinal * 10 + static_cast<int32>(Name[Index] - TEXT('0'));
	}
	return Ordinal >= 1 && Ordinal <= 82;
}

bool FGridWeaponVisualArchetypeBinding::IsValidBinding() const
{
	return UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(ArchetypeId) && !VisualId.IsNone();
}

bool FGridWeaponVisualProfile::IsValidProfile() const
{
	// An icon-only profile is valid, as is a mesh-only profile: UI and world
	// presentation have independent requirements and explicit fallbacks.
	// No asset is loaded to verify file existence at this authoring stage.
	if (VisualId.IsNone() || FamilyTag.IsNone() || (BaseIcon.IsNull() && WorldMesh.IsNull()) ||
		( !OrnamentMesh.IsNull() && WorldMesh.IsNull() ))
	{
		return false;
	}
	// A child ornament must not introduce invalid or degenerate transforms.
	// Both transforms are presentation-only, never gameplay placement.
	return !WorldVisualTransform.ContainsNaN() && !OrnamentRelativeTransform.ContainsNaN() &&
		WorldVisualTransform.GetScale3D().GetMin() > KINDA_SMALL_NUMBER &&
		OrnamentRelativeTransform.GetScale3D().GetMin() > KINDA_SMALL_NUMBER;
}

bool UGridWeaponVisualCatalogueAsset::IsValidCatalogue() const
{
	if (ArchetypeBindings.IsEmpty() || VisualProfiles.IsEmpty() || ArchetypeBindings.Num() > 82)
	{
		return false;
	}

	TSet<FName> UniqueVisualIds;
	for (const FGridWeaponVisualProfile& Profile : VisualProfiles)
	{
		if (!Profile.IsValidProfile() || UniqueVisualIds.Contains(Profile.VisualId))
		{
			return false;
		}
		UniqueVisualIds.Add(Profile.VisualId);
	}

	TSet<FName> UniqueArchetypes;
	for (const FGridWeaponVisualArchetypeBinding& Binding : ArchetypeBindings)
	{
		if (!Binding.IsValidBinding() || UniqueArchetypes.Contains(Binding.ArchetypeId) ||
			!UniqueVisualIds.Contains(Binding.VisualId))
		{
			return false;
		}
		UniqueArchetypes.Add(Binding.ArchetypeId);
	}

	// Deliberately no requirement to map all 82 archetypes at once:
	// three pilot silhouettes are authored before full art coverage.
	return true;
}

FName UGridWeaponVisualCatalogueAsset::FindVisualId(FName ArchetypeId) const
{
	if (!IsCanonicalArchetypeId(ArchetypeId))
	{
		return NAME_None;
	}

	for (const FGridWeaponVisualArchetypeBinding& Binding : ArchetypeBindings)
	{
		if (Binding.ArchetypeId == ArchetypeId && Binding.IsValidBinding())
		{
			return Binding.VisualId;
		}
	}
	return NAME_None;
}

const FGridWeaponVisualProfile* UGridWeaponVisualCatalogueAsset::FindVisualProfile(FName VisualId) const
{
	if (VisualId.IsNone())
	{
		return nullptr;
	}
	for (const FGridWeaponVisualProfile& Profile : VisualProfiles)
	{
		if (Profile.VisualId == VisualId)
		{
			return &Profile;
		}
	}
	return nullptr;
}

const FGridWeaponVisualProfile* UGridWeaponVisualCatalogueAsset::FindProfileForArchetype(FName ArchetypeId) const
{
	return FindVisualProfile(FindVisualId(ArchetypeId));
}
