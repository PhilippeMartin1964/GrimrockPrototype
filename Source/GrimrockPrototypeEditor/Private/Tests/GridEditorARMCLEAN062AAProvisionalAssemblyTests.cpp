#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06CompleteProductionPreview.h"
#include "ARMEMENT/GridArmClean06ProvisionalPresentationAssembly.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062AAProvisionalPresentationAssembly,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProvisionalPresentationBindingAssembly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062AAProvisionalPresentationAssembly::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("Read official 22 ARM manifest"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
	{
		AddError(Error);
		return false;
	}
	const UGridWeaponCatalogueAsset* Numerics =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	const UGridWeaponVisualCatalogueAsset* Visuals =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* Icons =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	if (!TestNotNull(TEXT("Source ARM numerics"), Numerics) ||
		!TestNotNull(TEXT("Source ARM visuals"), Visuals) ||
		!TestNotNull(TEXT("Source combat icons"), Icons))
	{
		return false;
	}
	const bool bNumbersDirty = Numerics->GetOutermost()->IsDirty();
	const bool bVisualsDirty = Visuals->GetOutermost()->IsDirty();
	const bool bIconsDirty = Icons->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridItemSystemAsset> Candidate(
		NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06CompleteProductionPreview::FSummary Baseline;
	if (!TestTrue(TEXT("Construct fully guarded production preview"),
		GridArmClean06CompleteProductionPreview::Build(
			Rows, *Numerics, *Visuals, *Icons, *Candidate, Baseline)))
	{
		return false;
	}
	const int32 ActionsBefore = Candidate->ActionCatalogue->Actions.Num();
	const int32 PresentationsBefore =
		Candidate->ActionPresentationCatalogue->Presentations.Num();
	const int32 HistoricalBindings = Baseline.Core.Actions.BoundActions;
	GridArmClean06ProvisionalPresentationAssembly::FSummary Provisional;
	if (!TestTrue(TEXT("Stage only 06.2Z-proven existing ActionId keys"),
		GridArmClean06ProvisionalPresentationAssembly::Apply(
			Rows, *Candidate, Baseline, Provisional)))
	{
		return false;
	}
	TestEqual(TEXT("Every source proposal is staged once"),
		Provisional.AppliedProvisionalBindings,
		Provisional.Source.ProposedFromExistingActionId);
	TestEqual(TEXT("Only existing ActionIds can become a provisional presentation"),
		Provisional.RemainingUnboundAttacks,
		Baseline.Core.UnboundOffensiveArmaments -
			Provisional.AppliedProvisionalBindings);
	TestEqual(TEXT("Existing action definitions remain unchanged"),
		Candidate->ActionCatalogue->Actions.Num(), ActionsBefore);
	TestEqual(TEXT("No presentation is lost or added without a proposal"),
		Candidate->ActionPresentationCatalogue->Presentations.Num(),
		PresentationsBefore + Provisional.AppliedProvisionalBindings);
	TestEqual(TEXT("Historical binding count is immutable evidence"),
		Baseline.Core.Actions.BoundActions, HistoricalBindings);
	TestEqual(TEXT("Provisional status remains explicit"),
		Provisional.bContainsUnapprovedPresentationKeys,
		Provisional.AppliedProvisionalBindings > 0);
	TArray<FString> Errors;
	TestTrue(TEXT("Staged global presentation catalogue remains valid"),
		Candidate->ActionPresentationCatalogue->Validate(Errors));
	for (const GridArmClean06PresentationKeyPreview::FProposal& P :
		Provisional.Source.Proposals)
	{
		const FGridCanonicalWeaponArchetype* Arm =
			Candidate->WeaponCatalogue->Find(P.ArchetypeId);
		const FGridCanonicalActionPresentation* Presentation =
			Candidate->ActionPresentationCatalogue->Find(P.ExistingActionId);
		if (!Arm || !Presentation ||
			Presentation->PresentationId != P.ExistingActionId ||
			!Arm->Actions.ContainsByPredicate(
				[&P](const FGridItemActionBinding& Binding)
				{
					return Binding.ActionId == P.ExistingActionId &&
						Binding.PresentationId == P.ExistingActionId;
				}))
		{
			AddError(TEXT("A proposed item/action/presentation triad has broken referential integrity"));
			return false;
		}
	}
	GridArmClean06ProvisionalPresentationAssembly::FSummary Second;
	TestTrue(TEXT("Reprojection is idempotent on an already staged transient registry"),
		GridArmClean06ProvisionalPresentationAssembly::Apply(
			Rows, *Candidate, Baseline, Second));
	TestEqual(TEXT("Idempotent second pass creates no duplicate profiles"),
		Second.AppliedProvisionalBindings, 0);
	TestEqual(TEXT("No duplicate presentation references on second pass"),
		Candidate->ActionPresentationCatalogue->Presentations.Num(),
		PresentationsBefore + Provisional.AppliedProvisionalBindings);
	TestFalse(TEXT("Staging unapproved presentation keys NEVER authorizes cutover"),
		Baseline.ReadyForBinaryCutover());
	TestTrue(TEXT("Numeric source unchanged"), Numerics->GetOutermost()->IsDirty() == bNumbersDirty);
	TestTrue(TEXT("Visual source unchanged"), Visuals->GetOutermost()->IsDirty() == bVisualsDirty);
	TestTrue(TEXT("Icon source unchanged"), Icons->GetOutermost()->IsDirty() == bIconsDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2AA: %d historical presentation bindings retained, "
		"%d existing ActionIds provisionally linked, %d still unbound, "
		"0 binary data assets written; cutover remains blocked."),
		HistoricalBindings, Provisional.AppliedProvisionalBindings,
		Provisional.RemainingUnboundAttacks));
	return Candidate->ActionCatalogue->Actions.Num() == ActionsBefore &&
		!Baseline.ReadyForBinaryCutover();
}

#endif
