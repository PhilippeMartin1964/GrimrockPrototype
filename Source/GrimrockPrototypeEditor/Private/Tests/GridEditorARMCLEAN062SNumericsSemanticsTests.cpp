#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ProductionNumericsCandidate.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062SProductionNumbers,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.Numeric82WithProductionSemantics22",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062SProductionNumbers::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Exact manifest of production ARM"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	const UGridWeaponCatalogueAsset* Source =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	if (!TestNotNull(TEXT("Compact source catalogue"), Source)) { return false; }
	const bool bSourceDirty = Source->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridCanonicalWeaponCatalogueAsset> Candidate(
		NewObject<UGridCanonicalWeaponCatalogueAsset>(GetTransientPackage()));
	GridArmClean06ProductionNumericsCandidate::FSummary Stats;
	if (!TestTrue(TEXT("Build lossless 82x20 with only 22 proven hand/kind"),
		GridArmClean06ProductionNumericsCandidate::Build(
			*Source, Manifest, *Candidate, Stats)))
	{
		return false;
	}
	TestEqual(TEXT("82 ARM native entries"), Stats.Archetypes, 82);
	TestEqual(TEXT("1640 numeric profiles"), Stats.ComparedProfiles, 1640);
	TestEqual(TEXT("260 passive profiles"), Stats.PassiveProfiles, 260);
	TestEqual(TEXT("22 concrete entries have real hand and kind"), Stats.ProvenConcreteArchetypes, 22);
	TestEqual(TEXT("60 numeric-only entries have no claimed hand/kind"), Stats.UnprovenUninstantiatedArchetypes, 60);
	TestEqual(TEXT("19 mainhand attacks"), Stats.MainHandAttacks, 19);
	TestEqual(TEXT("2 offhand attacks"), Stats.OffHandAttacks, 2);
	TestEqual(TEXT("Six source class preferences"), Candidate->ClassPreferences.Num(), 6);

	TSet<FName> ProductionIds;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		ProductionIds.Add(FName(*Row.ArchetypeId));
	}
	int32 Proven = 0, Unproven = 0;
	for (const FGridCanonicalWeaponArchetype& Arm : Candidate->Archetypes)
	{
		if (ProductionIds.Contains(Arm.ArchetypeId))
		{
			TestTrue(TEXT("Attack-source slots remain unauthored until real action bindings"),
				Arm.AttackSourceSlots.IsEmpty());
			if (!Arm.bHandAndKindProven) { AddError(TEXT("Active ARM semantics not qualified")); }
			else { ++Proven; }
		}
		else
		{
			if (Arm.bHandAndKindProven) { AddError(TEXT("Uninstantiated ARM falsely marked proven")); }
			else { ++Unproven; }
		}
	}
	TestEqual(TEXT("Exactly 22 active proved"), Proven, 22);
	TestEqual(TEXT("Exactly 60 inactive unproven"), Unproven, 60);
	TestTrue(TEXT("Source numeric catalogue was never saved/modified"),
		Source->GetOutermost()->IsDirty() == bSourceDirty);
	GridArmClean06ProductionNumericsCandidate::FSummary SecondPass;
	TestFalse(TEXT("A real numeric catalogue candidate is never silently overwritten"),
		GridArmClean06ProductionNumericsCandidate::Build(
			*Source, Manifest, *Candidate, SecondPass));
	return Proven == 22 && Unproven == 60 &&
		Source->GetOutermost()->IsDirty() == bSourceDirty;
}
#endif
