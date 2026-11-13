#include "ARMEMENT/GridArmClean06PresentationKeyPreview.h"

#include "ARMEMENT/GridArmClean06PresentationProjection.h"
#include "Runtime/GridItemDefinitionAsset.h"

bool GridArmClean06PresentationKeyPreview::Build(
	const TArray<FGridWeaponPipelineRow>& Manifest,
	const UGridItemSystemAsset& Root, FSummary& OutSummary)
{
	OutSummary = FSummary();
	if (Manifest.Num() != 22 || !Root.WeaponCatalogue ||
		!Root.ActionCatalogue || !Root.ActionPresentationCatalogue ||
		Root.ItemDefinitions.Num() != 23)
	{
		return false;
	}
	TSet<FName> SeenArchetypes;
	TSet<FName> SeenActionIds;
	TSet<FName> ProposedPresentationIds;
	const int32 ActionCountBefore = Root.ActionCatalogue->Actions.Num();
	const int32 PresentationCountBefore =
		Root.ActionPresentationCatalogue->Presentations.Num();

	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ArmId(*Row.ArchetypeId);
		if (Row.ProductionState != TEXT("Active") ||
			SeenArchetypes.Contains(ArmId))
		{
			return false;
		}
		SeenArchetypes.Add(ArmId);
		const UGridItemDefinitionAsset* Source =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		const FGridCanonicalWeaponArchetype* Arm =
			Root.WeaponCatalogue->Find(ArmId);
		if (!IsValid(Source) || !Arm ||
			Source->ItemDefinitionId != FName(*Row.ItemDefinitionId) ||
			Source->WeaponArchetypeId != ArmId)
		{
			return false;
		}
		const bool bSourceWasDirty = Source->GetOutermost()->IsDirty();
		if (Row.IsPassive())
		{
			if (!Source->CombatActions.IsEmpty() ||
				!Arm->Actions.IsEmpty())
			{
				return false;
			}
			++OutSummary.PassiveItems;
			continue;
		}
		const FName ExistingActionId(*Row.AttackId);
		if (ExistingActionId.IsNone() ||
			SeenActionIds.Contains(ExistingActionId))
		{
			return false;
		}
		SeenActionIds.Add(ExistingActionId);
		const FGridCombatActionDefinition* Primary =
			Source->CombatActions.FindByPredicate(
				[ExistingActionId](const FGridCombatActionDefinition& Action)
				{
					return Action.ActionId == ExistingActionId;
				});
		if (!Primary || !Primary->IsValid())
		{
			return false;
		}
		OutSummary.OtherLocalActionsPreserved +=
			FMath::Max(0, Source->CombatActions.Num() - 1);
		const bool bAlreadyLinked = Arm->Actions.ContainsByPredicate(
			[ExistingActionId](const FGridItemActionBinding& Binding)
			{
				return Binding.ActionId == ExistingActionId &&
					!Binding.PresentationId.IsNone();
			});
		if (bAlreadyLinked)
		{
			++OutSummary.ExistingBoundActions;
		}
		else if (!Root.ActionCatalogue->Find(ExistingActionId))
		{
			++OutSummary.UnconvertedPrimaryAction;
		}
		else if (!Source->HasValidPlayerAttackPresentation())
		{
			++OutSummary.MissingLocalPresentation;
		}
		else if (!Primary->PresentationProfileId.IsNone())
		{
			// Never replace an explicit historical key with an inferred one.
			++OutSummary.ExplicitPresentationIdStillUnresolved;
		}
		else if (ProposedPresentationIds.Contains(ExistingActionId) ||
			Root.ActionPresentationCatalogue->Find(ExistingActionId))
		{
			// Two owners may legitimately share an ID only if exact profile
			// equality has been proven by a separate, explicit migration.
			++OutSummary.ConflictingPresentationId;
		}
		else
		{
			FGridCanonicalActionPresentation Proposed;
			if (!GridArmClean06PresentationProjection::TryProject(
					ExistingActionId, Source->PlayerAttackPresentationProfile, Proposed) ||
				Proposed.PresentationId != ExistingActionId)
			{
				++OutSummary.PresentationProjectionRejected;
			}
			else
			{
				FProposal& Proposal = OutSummary.Proposals.AddDefaulted_GetRef();
				Proposal.ArchetypeId = ArmId;
				Proposal.ExistingActionId = ExistingActionId;
				Proposal.Presentation = MoveTemp(Proposed);
				ProposedPresentationIds.Add(ExistingActionId);
				++OutSummary.ProposedFromExistingActionId;
			}
		}
		if (Source->GetOutermost()->IsDirty() != bSourceWasDirty)
		{
			return false;
		}
	}
	return SeenArchetypes.Num() == 22 && OutSummary.AllAttacksClassified() &&
		Root.ActionCatalogue->Actions.Num() == ActionCountBefore &&
		Root.ActionPresentationCatalogue->Presentations.Num() ==
			PresentationCountBefore;
}
