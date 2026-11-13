#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ProductionActionsCandidate.h"
#include "ARMEMENT/GridArmClean06ProductionCandidate.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062QProductionActions,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionCatalogues22",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062QProductionActions::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Manifest of 22 active ARM weapons"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	const UGridWeaponCatalogueAsset* SourceNumerics =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	const UGridWeaponVisualCatalogueAsset* SourceVisuals =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* SourceIcons =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	if (!TestNotNull(TEXT("Real N1-N20 weapon catalogue"), SourceNumerics) ||
		!TestNotNull(TEXT("Real weapon visuals"), SourceVisuals) ||
		!TestNotNull(TEXT("Real action icons"), SourceIcons))
	{
		return false;
	}
	const bool bNumericDirty = SourceNumerics->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridItemSystemAsset> Candidate(
		NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06ProductionCandidate::FSummary IdentitySummary;
	if (!TestTrue(TEXT("Compose 22 authentic identities and visual profiles"),
		GridArmClean06ProductionCandidate::BuildDefinitionsAndVisuals(
			Manifest, *SourceVisuals, *SourceIcons, *Candidate, IdentitySummary)))
	{
		return false;
	}
	GridArmClean06ProductionActionsCandidate::FSummary ActionSummary;
	if (!TestTrue(TEXT("Attach only provable canonical attack/presentation data"),
		GridArmClean06ProductionActionsCandidate::AddProvenActionsAndPresentations(
			Manifest, *SourceNumerics, *Candidate, ActionSummary)))
	{
		return false;
	}
	TestEqual(TEXT("Three passive entries produce no active attacks"),
		ActionSummary.PassiveDefinitions, 3);
	TestEqual(TEXT("Nineteen active ARM actions are either proved or preserved"),
		ActionSummary.StandardActions + ActionSummary.CustomizedActionsUnconverted, 19);
	TestEqual(TEXT("No imagined ActionIds"), Candidate->ActionCatalogue->Actions.Num(),
		ActionSummary.StandardActions);
	TestEqual(TEXT("Only one shared silent preset, never fifteen copies"),
		Candidate->ActionPresentationCatalogue->Presentations.Num(),
		ActionSummary.BoundActions - ActionSummary.NeutralDefaultBindings +
			(ActionSummary.NeutralDefaultBindings > 0 ? 1 : 0));
	TestEqual(TEXT("Actual primary attacks all have a canonical binding"),
		ActionSummary.BoundActions, 19);
	TestEqual(TEXT("Exactly fifteen current attacks use the historic silent default"),
		ActionSummary.NeutralDefaultBindings, 15);
	TestTrue(TEXT("Actual bindings never outnumber proved actions"),
		ActionSummary.BoundActions <= ActionSummary.StandardActions);

	// Only the declared, explicit migration keys may fill historical gaps.
	// The existing per-asset PresentationProfileId always takes precedence.
	int32 MigratableMissingKeys = 0;
	int32 ObservedNeutralDefaults = 0;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		if (Row.IsPassive())
		{
			TestEqual(TEXT("Passive ARM has no migration presentation ID"),
				Row.MigrationPresentationId, FString(TEXT("None")));
			continue;
		}
		const UGridItemDefinitionAsset* Source =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		if (!TestNotNull(TEXT("Official manifest action source"), Source))
		{
			return false;
		}
		const FName AttackId(*Row.AttackId);
		const FGridCombatActionDefinition* Primary =
			Source->CombatActions.FindByPredicate(
				[AttackId](const FGridCombatActionDefinition& A)
				{
					return A.ActionId == AttackId;
				});
		if (!TestNotNull(TEXT("Actual authored primary action"), Primary))
		{
			return false;
		}
		TestTrue(TEXT("Presentation migration has a distinct explicit key"),
			!Row.MigrationPresentationId.IsEmpty() &&
			Row.MigrationPresentationId != Row.AttackId);
		const bool bHasAuthoredPresentation = Source->HasValidPlayerAttackPresentation();
		const bool bInvalidOptIn =
			Source->bProvidesAttackPresentation && !bHasAuthoredPresentation;
		const FName SelectedId = !bHasAuthoredPresentation ?
			FName(TEXT("PRES_AttackNoFX")) :
			(Primary->PresentationProfileId.IsNone() ?
				FName(*Row.MigrationPresentationId) : Primary->PresentationProfileId);
		const bool bConvertedStandard =
			Candidate->ActionCatalogue->Find(AttackId) != nullptr;
		if (bConvertedStandard && Primary->PresentationProfileId.IsNone() &&
			bHasAuthoredPresentation)
		{
			++MigratableMissingKeys;
		}
		const FGridCanonicalActionPresentation* Resolved =
			Candidate->ActionPresentationCatalogue->Find(SelectedId);
		if (bConvertedStandard && !bInvalidOptIn)
		{
			TestNotNull(TEXT("Valid source or proven neutral default must resolve"), Resolved);
			if (!bHasAuthoredPresentation) { ++ObservedNeutralDefaults; }
		}
		else
		{
			TestNull(TEXT("Invalid opt-in cannot produce a presentation"), Resolved);
		}
	}
	TestEqual(TEXT("Neutral bindings match the actual 22-item source opt-ins"),
		ObservedNeutralDefaults, ActionSummary.NeutralDefaultBindings);
	TestEqual(TEXT("No presentation key is absent after manifest authoring"),
		ActionSummary.NoAuthoredPresentationId, 0);
	TestTrue(TEXT("Migratable source gaps do not exceed real bindings"),
		MigratableMissingKeys <= ActionSummary.BoundActions);

	TArray<FString> Errors;
	TestFalse(TEXT("Incomplete 82-ARM numeric semantics must not register as complete"),
		Candidate->Validate(Errors));
	TestTrue(TEXT("Full canonical cutover remains closed"), Errors.Num() > 0);
	GridArmClean06ProductionActionsCandidate::FSummary SecondPass;
	TestFalse(TEXT("Second pass must not overwrite newly populated catalogues"),
		GridArmClean06ProductionActionsCandidate::AddProvenActionsAndPresentations(
			Manifest, *SourceNumerics, *Candidate, SecondPass));
	TestTrue(TEXT("Source weapon numerics remain clean"),
		SourceNumerics->GetOutermost()->IsDirty() == bNumericDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2Q: %d standard attacks; %d customized attacks preserved; "
		"%d other locally authored actions preserved; %d direct bindings "
		"with real PresentationId; %d without PresentationId; "
		"%d without local presentation; %d duplicate presentation keys. "
		"No binary package saved."),
		Candidate->ActionCatalogue->Actions.Num(),
		19 - Candidate->ActionCatalogue->Actions.Num(),
		ActionSummary.OtherLocalActionsUnconverted,
		Candidate->ActionPresentationCatalogue->Presentations.Num(),
		ActionSummary.NoAuthoredPresentationId,
		ActionSummary.NoLocalPresentation,
		ActionSummary.ConflictingPresentationId));
	return Candidate->ActionCatalogue->Actions.Num() <= 19 &&
		SourceNumerics->GetOutermost()->IsDirty() == bNumericDirty;
}

#endif
