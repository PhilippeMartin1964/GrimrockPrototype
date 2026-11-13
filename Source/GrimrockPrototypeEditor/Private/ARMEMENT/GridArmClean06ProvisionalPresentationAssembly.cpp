#include "ARMEMENT/GridArmClean06ProvisionalPresentationAssembly.h"

bool GridArmClean06ProvisionalPresentationAssembly::Apply(
	const TArray<FGridWeaponPipelineRow>& Manifest,
	UGridItemSystemAsset& TransientRoot,
	const GridArmClean06CompleteProductionPreview::FSummary& Baseline,
	FSummary& OutSummary)
{
	OutSummary = FSummary();
	if (Manifest.Num() != 22 || TransientRoot.ItemDefinitions.Num() != 23 ||
		!TransientRoot.WeaponCatalogue || !TransientRoot.ActionCatalogue ||
		!TransientRoot.ActionPresentationCatalogue ||
		!GridArmClean06PresentationKeyPreview::Build(
			Manifest, TransientRoot, OutSummary.Source))
	{
		return false;
	}
	if (OutSummary.Source.ProposedFromExistingActionId >
		Baseline.Core.UnboundOffensiveArmaments)
	{
		return false;
	}

	// Stage both catalogues and test them before any transient write.
	TArray<FGridCanonicalWeaponArchetype> StagedArms =
		TransientRoot.WeaponCatalogue->Archetypes;
	TArray<FGridCanonicalActionPresentation> StagedPresentations =
		TransientRoot.ActionPresentationCatalogue->Presentations;
	TSet<FName> NewKeys;
	for (const GridArmClean06PresentationKeyPreview::FProposal& Proposal :
		OutSummary.Source.Proposals)
	{
		if (Proposal.ArchetypeId.IsNone() || Proposal.ExistingActionId.IsNone() ||
			Proposal.Presentation.PresentationId != Proposal.ExistingActionId ||
			!Proposal.Presentation.IsValid() ||
			NewKeys.Contains(Proposal.Presentation.PresentationId) ||
			!TransientRoot.ActionCatalogue->Find(Proposal.ExistingActionId) ||
			StagedPresentations.ContainsByPredicate(
				[&Proposal](const FGridCanonicalActionPresentation& Profile)
				{
					return Profile.PresentationId == Proposal.Presentation.PresentationId;
				}))
		{
			return false;
		}
		FGridCanonicalWeaponArchetype* Arm = StagedArms.FindByPredicate(
			[&Proposal](const FGridCanonicalWeaponArchetype& Row)
			{
				return Row.ArchetypeId == Proposal.ArchetypeId;
			});
		if (!Arm || !Arm->bHandAndKindProven ||
			Arm->Kind != EGridCanonicalArmamentKind::Weapon ||
			Arm->AttackSourceSlots.IsEmpty() ||
			Arm->Actions.ContainsByPredicate(
				[&Proposal](const FGridItemActionBinding& Binding)
				{
					return Binding.ActionId == Proposal.ExistingActionId;
				}))
		{
			return false;
		}
		FGridItemActionBinding Binding;
		Binding.ActionId = Proposal.ExistingActionId;
		Binding.PresentationId = Proposal.Presentation.PresentationId;
		Arm->Actions.Add(Binding);
		StagedPresentations.Add(Proposal.Presentation);
		NewKeys.Add(Proposal.Presentation.PresentationId);
	}

	UGridActionPresentationCatalogueAsset* Scratch =
		NewObject<UGridActionPresentationCatalogueAsset>(GetTransientPackage());
	Scratch->Presentations = StagedPresentations;
	TArray<FString> Errors;
	if (!Scratch->Validate(Errors))
	{
		return false;
	}

	TransientRoot.WeaponCatalogue->Archetypes = MoveTemp(StagedArms);
	TransientRoot.ActionPresentationCatalogue->Presentations =
		MoveTemp(StagedPresentations);
	OutSummary.AppliedProvisionalBindings = NewKeys.Num();
	OutSummary.RemainingUnboundAttacks =
		Baseline.Core.UnboundOffensiveArmaments - NewKeys.Num();
	OutSummary.bContainsUnapprovedPresentationKeys = NewKeys.Num() > 0;
	OutSummary.bPresentationCatalogueValid = true;

	// Baseline.Core intentionally remains a record of *historically authored*
	// bindings: provisional keys are NOT authorized by a successful Validate().
	return true;
}
