#include "ARMEMENT/GridArmClean06ClassAffinityProjection.h"

namespace
{
	FGridCanonicalClassPreference CopyPreference(const FGridWeaponClassPreference& Source)
	{
		FGridCanonicalClassPreference Out;
		Out.ClassId = Source.ClassId;
		Out.CanonicalBranches = Source.CanonicalBranches;
		Out.PriorityWeapons = Source.PriorityWeapons;
		Out.EmblematicExamples = Source.EmblematicExamples;
		Out.Alternatives = Source.Alternatives;
		Out.DominantAttributes = Source.DominantAttributes;
		Out.Synergies = Source.Synergies;
		return Out;
	}

	bool EqualPreference(const FGridWeaponClassPreference& Source,
		const FGridCanonicalClassPreference& Destination)
	{
		return Source.ClassId == Destination.ClassId &&
			Source.CanonicalBranches == Destination.CanonicalBranches &&
			Source.PriorityWeapons == Destination.PriorityWeapons &&
			Source.EmblematicExamples == Destination.EmblematicExamples &&
			Source.Alternatives == Destination.Alternatives &&
			Source.DominantAttributes == Destination.DominantAttributes &&
			Source.Synergies == Destination.Synergies;
	}
}

bool GridArmClean06ClassAffinityProjection::EqualsSource(
	const UGridWeaponCatalogueAsset& Source,
	const UGridCanonicalWeaponCatalogueAsset& Destination)
{
	if (Source.ClassPreferences.Num() != Destination.ClassPreferences.Num() ||
		Source.ClassAffinities.Num() != Destination.Archetypes.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Source.ClassPreferences.Num(); ++Index)
	{
		if (!EqualPreference(Source.ClassPreferences[Index], Destination.ClassPreferences[Index]))
		{
			return false;
		}
	}
	for (const FGridWeaponArchetypeClassAffinity& Row : Source.ClassAffinities)
	{
		const FGridCanonicalWeaponArchetype* Candidate = Destination.Find(Row.ArchetypeId);
		if (!Candidate || Candidate->PrimaryClassId != Row.PrimaryClassId ||
			Candidate->SecondaryClassIds != Row.SecondaryClassIds ||
			Candidate->SecondarySourceLabel != Row.SecondarySourceLabel)
		{
			return false;
		}
	}
	return true;
}

bool GridArmClean06ClassAffinityProjection::TryProject(
	const UGridWeaponCatalogueAsset& Source,
	UGridCanonicalWeaponCatalogueAsset& Destination)
{
	// Refuse partial input, a pre-populated target, or a count mismatch.
	if (!Source.IsValidClassAffinityData() ||
		Source.ClassAffinities.Num() != 82 || Source.ClassPreferences.Num() != 6 ||
		Destination.Archetypes.Num() != 82 ||
		!Destination.ClassPreferences.IsEmpty())
	{
		return false;
	}
	TMap<FName, const FGridWeaponArchetypeClassAffinity*> SourceRows;
	for (const FGridWeaponArchetypeClassAffinity& Row : Source.ClassAffinities)
	{
		if (Row.ArchetypeId.IsNone() || SourceRows.Contains(Row.ArchetypeId))
		{
			return false;
		}
		SourceRows.Add(Row.ArchetypeId, &Row);
	}

	TSet<FName> DestinationIds;
	TArray<FGridCanonicalWeaponArchetype> Projected = Destination.Archetypes;
	for (FGridCanonicalWeaponArchetype& Arm : Projected)
	{
		if (Arm.ArchetypeId.IsNone() || DestinationIds.Contains(Arm.ArchetypeId) ||
			!Arm.PrimaryClassId.IsNone() || !Arm.SecondaryClassIds.IsEmpty() ||
			!Arm.SecondarySourceLabel.IsEmpty())
		{
			return false;
		}
		DestinationIds.Add(Arm.ArchetypeId);
		const FGridWeaponArchetypeClassAffinity* const* Found = SourceRows.Find(Arm.ArchetypeId);
		if (!Found || !*Found)
		{
			return false;
		}
		Arm.PrimaryClassId = (*Found)->PrimaryClassId;
		Arm.SecondaryClassIds = (*Found)->SecondaryClassIds;
		Arm.SecondarySourceLabel = (*Found)->SecondarySourceLabel;
	}

	TArray<FGridCanonicalClassPreference> Preferences;
	Preferences.Reserve(6);
	for (const FGridWeaponClassPreference& Preference : Source.ClassPreferences)
	{
		Preferences.Add(CopyPreference(Preference));
	}

	// Destination is a fresh transient object: no binary package save or
	// changes to the game's active weapon catalogue.
	Destination.Archetypes = MoveTemp(Projected);
	Destination.ClassPreferences = MoveTemp(Preferences);
	return EqualsSource(Source, Destination);
}
