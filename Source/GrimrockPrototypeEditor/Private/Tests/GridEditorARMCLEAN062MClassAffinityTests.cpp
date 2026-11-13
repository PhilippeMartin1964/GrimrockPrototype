#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ClassAffinityProjection.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062MClassAffinityProjection,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ClassAffinityProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062MClassAffinityProjection::RunTest(const FString&)
{
	const UGridWeaponCatalogueAsset* Source = LoadObject<UGridWeaponCatalogueAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	if (!TestNotNull(TEXT("Source ARM catalogue"), Source) ||
		!TestTrue(TEXT("82 real class affinity rows valid"), Source->IsValidClassAffinityData()) ||
		!TestEqual(TEXT("Six class preference rows"), Source->ClassPreferences.Num(), 6))
	{
		return false;
	}
	const bool bSourceWasDirty = Source->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridCanonicalWeaponCatalogueAsset> Candidate(
		NewObject<UGridCanonicalWeaponCatalogueAsset>(GetTransientPackage()));
	for (const FGridWeaponArchetypeProgression& SourceRow : Source->ArchetypeProgressions)
	{
		FGridCanonicalWeaponArchetype& Arm = Candidate->Archetypes.AddDefaulted_GetRef();
		Arm.ArchetypeId = SourceRow.BaseProfile.ArchetypeId;
	}
	if (!TestTrue(TEXT("82 class affinities project without any rule inference"),
		GridArmClean06ClassAffinityProjection::TryProject(*Source, *Candidate)))
	{
		return false;
	}
	TestTrue(TEXT("Every class ID, order and source qualifier are identical"),
		GridArmClean06ClassAffinityProjection::EqualsSource(*Source, *Candidate));
	TestEqual(TEXT("82 canonical archetype class rows"), Candidate->Archetypes.Num(), 82);
	TestEqual(TEXT("Six canonical class preferences"), Candidate->ClassPreferences.Num(), 6);
	TestFalse(TEXT("Projection refuses a prepopulated canonical target"),
		GridArmClean06ClassAffinityProjection::TryProject(*Source, *Candidate));
	TestTrue(TEXT("Rejected duplicate keeps original metadata unchanged"),
		GridArmClean06ClassAffinityProjection::EqualsSource(*Source, *Candidate));
	TestTrue(TEXT("Source catalogue stays unchanged"),
		Source->GetOutermost()->IsDirty() == bSourceWasDirty);
	return Source->GetOutermost()->IsDirty() == bSourceWasDirty;
}
#endif
