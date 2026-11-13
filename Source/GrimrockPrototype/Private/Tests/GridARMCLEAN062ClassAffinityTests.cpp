#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ClassParityTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Semantics.ClassAffinityParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062ClassParityTest::RunTest(const FString&)
{
	const UGridWeaponCatalogueAsset* Source = LoadObject<UGridWeaponCatalogueAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	if (!TestNotNull(TEXT("Real UE source catalogue"), Source)) return false;
	if (!TestTrue(TEXT("Class source valid"), Source->IsValidClassAffinityData()) ||
		!TestEqual(TEXT("82 affinity rows"), Source->ClassAffinities.Num(), 82) ||
		!TestEqual(TEXT("Six class preferences"), Source->ClassPreferences.Num(), 6) ||
		!TestEqual(TEXT("82 compact archetypes"), Source->ArchetypeProgressions.Num(), 82))
	{
		return false;
	}

	TStrongObjectPtr<UGridCanonicalWeaponCatalogueAsset> Target(
		NewObject<UGridCanonicalWeaponCatalogueAsset>(GetTransientPackage()));
	for (const FGridWeaponArchetypeProgression& Entry : Source->ArchetypeProgressions)
	{
		const FGridWeaponLevelProfile& N1 = Entry.BaseProfile;
		FGridCanonicalWeaponArchetype& Dest = Target->Archetypes.AddDefaulted_GetRef();
		Dest.ArchetypeId = N1.ArchetypeId;
		Dest.Kind = N1.DamageDice.bEnabled ? EGridCanonicalArmamentKind::Weapon :
			EGridCanonicalArmamentKind::Shield;
		Dest.DamageType = N1.DamageType;
		Dest.PhysicalSubtype = N1.PhysicalSubtype;
		Dest.ScalingAttribute = N1.ScalingAttribute;
		Dest.AccuracyBonus = N1.AccuracyBonus;
		Dest.ActionPointCost = N1.ActionPointCost;
		Dest.RangeCells = N1.RangeCells;
		Dest.DamageDiceN1 = N1.DamageDice;
		Dest.PhysicalArmorN1 = N1.PhysicalArmor;
		Dest.MagicalArmorN1 = N1.MagicalArmor;
		Dest.DiceCountGrowth = Entry.DiceCountGrowth;
		Dest.DiceSidesGrowth = Entry.DiceSidesGrowth;
		Dest.FlatBonusGrowth = Entry.FlatBonusGrowth;
		Dest.PhysicalArmorGrowth = Entry.PhysicalArmorGrowth;
		Dest.MagicalArmorGrowth = Entry.MagicalArmorGrowth;
	}
	for (const FGridWeaponClassPreference& A : Source->ClassPreferences)
	{
		FGridCanonicalClassPreference& B = Target->ClassPreferences.AddDefaulted_GetRef();
		B.ClassId = A.ClassId;
		B.CanonicalBranches = A.CanonicalBranches;
		B.PriorityWeapons = A.PriorityWeapons;
		B.EmblematicExamples = A.EmblematicExamples;
		B.Alternatives = A.Alternatives;
		B.DominantAttributes = A.DominantAttributes;
		B.Synergies = A.Synergies;
	}
	TSet<FName> Seen;
	for (const FGridWeaponArchetypeClassAffinity& A : Source->ClassAffinities)
	{
		if (!TestFalse(TEXT("Unique affinity ARM id"), Seen.Contains(A.ArchetypeId))) return false;
		Seen.Add(A.ArchetypeId);
		FGridCanonicalWeaponArchetype* B = Target->Archetypes.FindByPredicate(
			[&A](const FGridCanonicalWeaponArchetype& Row) { return Row.ArchetypeId == A.ArchetypeId; });
		if (!TestNotNull(TEXT("ARM affinity destination exists"), B)) return false;
		B->PrimaryClassId = A.PrimaryClassId;
		B->SecondaryClassIds = A.SecondaryClassIds;
		B->SecondarySourceLabel = A.SecondarySourceLabel;
	}
	TestEqual(TEXT("All 82 affinities mapped"), Seen.Num(), 82);
	for (int32 Index = 0; Index < Source->ClassPreferences.Num(); ++Index)
	{
		const FGridWeaponClassPreference& A = Source->ClassPreferences[Index];
		const FGridCanonicalClassPreference& B = Target->ClassPreferences[Index];
		TestEqual(TEXT("ClassId"), B.ClassId, A.ClassId);
		TestEqual(TEXT("CanonicalBranches"), B.CanonicalBranches, A.CanonicalBranches);
		TestEqual(TEXT("PriorityWeapons"), B.PriorityWeapons, A.PriorityWeapons);
		TestEqual(TEXT("EmblematicExamples"), B.EmblematicExamples, A.EmblematicExamples);
		TestEqual(TEXT("Alternatives"), B.Alternatives, A.Alternatives);
		TestEqual(TEXT("DominantAttributes"), B.DominantAttributes, A.DominantAttributes);
		TestEqual(TEXT("Synergies"), B.Synergies, A.Synergies);
	}
	for (const FGridWeaponArchetypeClassAffinity& A : Source->ClassAffinities)
	{
		const FGridCanonicalWeaponArchetype* B = Target->Find(A.ArchetypeId);
		if (!TestNotNull(TEXT("Destination class row"), B)) return false;
		TestEqual(TEXT("Primary tier"), B->PrimaryClassId, A.PrimaryClassId);
		TestEqual(TEXT("Secondary ordering"), B->SecondaryClassIds, A.SecondaryClassIds);
		TestEqual(TEXT("Qualifier text"), B->SecondarySourceLabel, A.SecondarySourceLabel);
	}
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Complete canonical catalogue valid"), Target->Validate(Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	const FName OriginalPrimary = Target->Archetypes[0].PrimaryClassId;
	Target->Archetypes[0].PrimaryClassId = NAME_None;
	Errors.Reset();
	TestFalse(TEXT("Missing primary rejected"), Target->Validate(Errors));
	Target->Archetypes[0].PrimaryClassId = OriginalPrimary;
	const FName OriginalId = Target->ClassPreferences[1].ClassId;
	Target->ClassPreferences[1].ClassId = Target->ClassPreferences[0].ClassId;
	Errors.Reset();
	TestFalse(TEXT("Duplicate class identity rejected"), Target->Validate(Errors));
	Target->ClassPreferences[1].ClassId = OriginalId;
	Errors.Reset();
	TestTrue(TEXT("Restored catalogue valid"), Target->Validate(Errors));
	return true;
}
#endif
