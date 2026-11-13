#include "ARMEMENT/GridArmClean06ProductionActionsCandidate.h"

#include "ARMEMENT/GridArmClean06ActionProjection.h"
#include "ARMEMENT/GridArmClean06PresentationProjection.h"
#include "Runtime/GridItemDefinitionAsset.h"

bool GridArmClean06ProductionActionsCandidate::AddProvenActionsAndPresentations(
	const TArray<FGridWeaponPipelineRow>& Manifest,
	const UGridWeaponCatalogueAsset& SourceWeaponCatalogue,
	UGridItemSystemAsset& TransientRoot, FSummary& OutSummary)
{
	OutSummary = FSummary();
	if (Manifest.Num() != 22 || !SourceWeaponCatalogue.IsValidCatalogue() ||
		TransientRoot.ItemDefinitions.Num() != 22 ||
		!TransientRoot.WeaponVisualCatalogue || TransientRoot.WeaponCatalogue ||
		TransientRoot.ActionCatalogue || TransientRoot.ActionPresentationCatalogue)
	{
		return false;
	}

	UGridActionCatalogueAsset* Actions = NewObject<UGridActionCatalogueAsset>(&TransientRoot);
	UGridActionPresentationCatalogueAsset* Presentations =
		NewObject<UGridActionPresentationCatalogueAsset>(&TransientRoot);
	TSet<FName> SeenArchetypes;
	TSet<FName> SeenActionIds;
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
		const FGridWeaponLevelProfile* N1 =
			SourceWeaponCatalogue.FindProfile(ArmId, 1);
		if (!IsValid(Source) || !N1 || Source->ItemDefinitionId !=
			FName(*Row.ItemDefinitionId) || Source->WeaponArchetypeId != ArmId)
		{
			return false;
		}
		const bool bWasDirty = Source->GetOutermost()->IsDirty();
		if (Row.IsPassive())
		{
			if (!Source->CombatActions.IsEmpty()) { return false; }
			++OutSummary.PassiveDefinitions;
			continue;
		}

		const FName AttackId(*Row.AttackId);
		const FGridCombatActionDefinition* Primary =
			Source->CombatActions.FindByPredicate(
				[AttackId](const FGridCombatActionDefinition& Entry)
				{
					return Entry.ActionId == AttackId;
				});
		if (AttackId.IsNone() || !Primary || SeenActionIds.Contains(AttackId))
		{
			return false;
		}
		SeenActionIds.Add(AttackId);
		OutSummary.OtherLocalActionsUnconverted +=
			FMath::Max(0, Source->CombatActions.Num() - 1);

		FGridCanonicalAction Canonical;
		const GridArmClean06ActionProjection::EResult Result =
			GridArmClean06ActionProjection::TryProjectGeneratedAttack(
				Row, *Source, *N1, *Primary, Canonical);
		if (Result == GridArmClean06ActionProjection::EResult::InvalidInput)
		{
			return false;
		}
		if (Result == GridArmClean06ActionProjection::EResult::NonStandardAction)
		{
			++OutSummary.CustomizedActionsUnconverted;
			continue; // original remains authoritative until a typed projection exists
		}
		Actions->Actions.Add(Canonical);
		++OutSummary.StandardActions;

		// This is the precise OLD runtime rule from
		// UGridPlayerAttackPresentationComponent::ResolveProfile:
		// disabled presentation uses FGridPlayerAttackPresentationProfile().
		// Author that single neutral value ONCE in the target catalogue,
		// never as a runtime fallback or a fabricated sound/Niagara effect.
		const bool bHasPresentation = Source->HasValidPlayerAttackPresentation();
		if (!bHasPresentation && Source->bProvidesAttackPresentation)
		{
			++OutSummary.NoLocalPresentation; // invalid opt-in, never conceal it
			continue;
		}
		const FName NeutralId(TEXT("PRES_AttackNoFX"));
		const FName PresentationId = bHasPresentation ?
			(Primary->PresentationProfileId.IsNone() ?
				FName(*Row.MigrationPresentationId) : Primary->PresentationProfileId) :
			NeutralId;
		if (PresentationId.IsNone())
		{
			++OutSummary.NoAuthoredPresentationId;
			continue;
		}
		if (Presentations->Find(PresentationId))
		{
			// Only the one value-identical neutral profile may be shared
			// without an explicit authoring/equality decision.
			if (bHasPresentation || PresentationId != NeutralId)
			{
				++OutSummary.ConflictingPresentationId;
				continue;
			}
		}
		else
		{
			const FGridPlayerAttackPresentationProfile Historical =
				bHasPresentation ? Source->PlayerAttackPresentationProfile :
					FGridPlayerAttackPresentationProfile();
			FGridCanonicalActionPresentation Projected;
			if (!GridArmClean06PresentationProjection::TryProject(
					PresentationId, Historical, Projected))
			{
				return false;
			}
			Presentations->Presentations.Add(MoveTemp(Projected));
		}
		if (!bHasPresentation) { ++OutSummary.NeutralDefaultBindings; }
		FGridItemActionBinding Link;
		Link.ActionId = Canonical.ActionId;
		Link.PresentationId = PresentationId;
		OutSummary.BindingsByArchetype.Add(ArmId, Link);
		++OutSummary.BoundActions;
		if (Source->GetOutermost()->IsDirty() != bWasDirty)
		{
			return false;
		}
	}

	if (SeenArchetypes.Num() != 22 || OutSummary.PassiveDefinitions != 3 ||
		OutSummary.PassiveDefinitions + OutSummary.StandardActions +
			OutSummary.CustomizedActionsUnconverted != 22 ||
		Actions->Actions.Num() != OutSummary.StandardActions ||
		Presentations->Presentations.Num() != OutSummary.BoundActions -
			OutSummary.NeutralDefaultBindings +
			(OutSummary.NeutralDefaultBindings > 0 ? 1 : 0))
	{
		return false;
	}
	TArray<FString> Errors;
	if (!Actions->Validate(Errors) || !Presentations->Validate(Errors))
	{
		return false;
	}
	TransientRoot.ActionCatalogue = Actions;
	TransientRoot.ActionPresentationCatalogue = Presentations;
	return true;
}
