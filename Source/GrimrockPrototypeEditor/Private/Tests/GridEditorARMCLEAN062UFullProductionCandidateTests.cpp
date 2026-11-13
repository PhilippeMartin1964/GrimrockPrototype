#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06FullProductionCandidate.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062UFullProductionCandidate,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.FullProductionCandidatePreflight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062UFullProductionCandidate::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Read 22 ARM production rows"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	const UGridWeaponCatalogueAsset* Numbers =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	const UGridWeaponVisualCatalogueAsset* Visuals =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* Icons =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	if (!TestNotNull(TEXT("Numeric source"), Numbers) ||
		!TestNotNull(TEXT("Visual source"), Visuals) ||
		!TestNotNull(TEXT("Action icon source"), Icons))
	{
		return false;
	}
	const bool bNumbersDirty = Numbers->GetOutermost()->IsDirty();
	const bool bVisualsDirty = Visuals->GetOutermost()->IsDirty();
	const bool bIconsDirty = Icons->GetOutermost()->IsDirty();

	TStrongObjectPtr<UGridItemSystemAsset> Root(NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06FullProductionCandidate::FSummary Summary;
	if (!TestTrue(TEXT("Assemble one transient registry with 4 real catalogue projections"),
		GridArmClean06FullProductionCandidate::Build(
			Manifest, *Numbers, *Visuals, *Icons, *Root, Summary)))
	{
		return false;
	}
	TestEqual(TEXT("22 concrete data-only item definitions"), Root->ItemDefinitions.Num(), 22);
	TestEqual(TEXT("82 catalogued archetypes"), Root->WeaponCatalogue->Archetypes.Num(), 82);
	TestEqual(TEXT("1640 numeric level comparisons"), Summary.Numerics.ComparedProfiles, 1640);
	TestEqual(TEXT("260 passives"), Summary.Numerics.PassiveProfiles, 260);
	TestEqual(TEXT("19 actual main attacks"), Summary.AttacksMainHand, 19);
	TestEqual(TEXT("2 actual offhand attacks"), Summary.AttacksOffHand, 2);
	TestEqual(TEXT("Every missing action-to-presentation link reported"),
		Summary.UnboundOffensiveArmaments, 19 - Summary.Actions.BoundActions);
	TestFalse(TEXT("A dry-run preflight does not authorize binary cutover"),
		Summary.ReadyForBinaryCutover());
	TestTrue(TEXT("Numeric source not modified"), Numbers->GetOutermost()->IsDirty() == bNumbersDirty);
	TestTrue(TEXT("Visual source not modified"), Visuals->GetOutermost()->IsDirty() == bVisualsDirty);
	TestTrue(TEXT("Icon source not modified"), Icons->GetOutermost()->IsDirty() == bIconsDirty);

	GridArmClean06FullProductionCandidate::FSummary Retry;
	TestFalse(TEXT("Populated root refuses overwriting on second pass"),
		GridArmClean06FullProductionCandidate::Build(
			Manifest, *Numbers, *Visuals, *Icons, *Root, Retry));
	AddInfo(FString::Printf(TEXT(
		"ARM-CLEAN06.2U: 22 concrete ARM, 82 numerics, 1640 levels; "
		"%d actions bound, %d missing bindings, %d ammo and %d throw policies "
		"unconverted, %d invalid canonical identities; full registry valid=%s; "
		"binary cutover REFUSED, 0 packages saved."),
		Summary.Actions.BoundActions, Summary.UnboundOffensiveArmaments,
		Summary.UnmappedAmmunition, Summary.UnconvertedThrowPolicies,
		Summary.InvalidCanonicalIdentities,
		Summary.bRegistryValid ? TEXT("true") : TEXT("false")));
	return Root->ItemDefinitions.Num() == 22 &&
		Root->WeaponCatalogue->Archetypes.Num() == 82 &&
		!Summary.ReadyForBinaryCutover();
}
#endif
