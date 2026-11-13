#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ProductionCandidate.h"
#include "ARMEMENT/GridArmClean06ActionIconsProjection.h"
#include "ARMEMENT/GridArmClean06VisualProjection.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062PProductionCandidate,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionCandidate22",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062PProductionCandidate::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Official ARM production manifest"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	const UGridWeaponVisualCatalogueAsset* SourceVisuals =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* SourceIcons =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	if (!TestNotNull(TEXT("Source weapon visuals"), SourceVisuals) ||
		!TestNotNull(TEXT("Source icon palette"), SourceIcons))
	{
		return false;
	}
	const bool bVisualsDirty = SourceVisuals->GetOutermost()->IsDirty();
	const bool bIconsDirty = SourceIcons->GetOutermost()->IsDirty();

	TStrongObjectPtr<UGridItemSystemAsset> Root(
		NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06ProductionCandidate::FSummary Summary;
	if (!TestTrue(TEXT("Build 22 real canonical definitions and visuals in memory"),
		GridArmClean06ProductionCandidate::BuildDefinitionsAndVisuals(
			Manifest, *SourceVisuals, *SourceIcons, *Root, Summary)))
	{
		return false;
	}
	TestEqual(TEXT("Exactly 22 canonical concrete item definitions"), Root->ItemDefinitions.Num(), 22);
	TestEqual(TEXT("Three shield/focus definitions"), Summary.PassiveDefinitions, 3);
	TestTrue(TEXT("Action palette copied exactly"),
		GridArmClean06ActionIconsProjection::IsIdentical(*SourceIcons,
			*Root->WeaponVisualCatalogue));
	TestEqual(TEXT("Each production archetype has one visual binding"),
		Root->WeaponVisualCatalogue->ArchetypeBindings.Num(), 22);
	TSet<FName> Ids;
	TSet<FName> Arms;
	for (const UGridCanonicalItemDefinitionAsset* Item : Root->ItemDefinitions)
	{
		const UGridArmamentDefinitionAsset* Arm =
			Cast<UGridArmamentDefinitionAsset>(Item);
		if (!Arm || Ids.Contains(Arm->ItemId) || Arms.Contains(Arm->ArchetypeId) ||
			!Root->WeaponVisualCatalogue->FindForArchetype(Arm->ArchetypeId))
		{
			AddError(TEXT("Incomplete or duplicate concrete ARM registration"));
			return false;
		}
		Ids.Add(Arm->ItemId);
		Arms.Add(Arm->ArchetypeId);
	}
	TestEqual(TEXT("22 unique ARM IDs"), Arms.Num(), 22);
	TestEqual(TEXT("22 unique ItemIds"), Ids.Num(), 22);

	TArray<FString> IncompleteErrors;
	TestFalse(TEXT("Partial candidate must NOT pass the full item-system gate"),
		Root->Validate(IncompleteErrors));
	TestTrue(TEXT("Full item system still requires numeric/action/presentation catalogues"),
		IncompleteErrors.Num() > 0);
	TestFalse(TEXT("A populated registry cannot be overwritten"),
		GridArmClean06ProductionCandidate::BuildDefinitionsAndVisuals(
			Manifest, *SourceVisuals, *SourceIcons, *Root, Summary));
	TestEqual(TEXT("Rejected overwrite keeps the 22 candidate records"),
		Root->ItemDefinitions.Num(), 22);
	TestEqual(TEXT("Original visual catalogue unchanged"),
		SourceVisuals->GetOutermost()->IsDirty(), bVisualsDirty);
	TestEqual(TEXT("Original icon palette unchanged"),
		SourceIcons->GetOutermost()->IsDirty(), bIconsDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2P: 22 real concrete definitions, %d distinct visuals, "
		"3 passive, %d legacy entries incompatible with final ID schema. "
		"No asset saves; full registration intentionally unavailable."),
		Summary.UniqueVisualProfiles, Summary.NonCanonicalIdentities));
	return Ids.Num() == 22 && Arms.Num() == 22 &&
		SourceVisuals->GetOutermost()->IsDirty() == bVisualsDirty &&
		SourceIcons->GetOutermost()->IsDirty() == bIconsDirty;
}

#endif
