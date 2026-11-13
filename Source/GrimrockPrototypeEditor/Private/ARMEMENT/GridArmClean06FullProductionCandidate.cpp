#include "ARMEMENT/GridArmClean06FullProductionCandidate.h"

#include "Runtime/GridItemDefinitionAsset.h"

bool GridArmClean06FullProductionCandidate::Build(
	const TArray<FGridWeaponPipelineRow>& Manifest,
	const UGridWeaponCatalogueAsset& NumericSource,
	const UGridWeaponVisualCatalogueAsset& VisualSource,
	const UGridEquippedWeaponActionIconPalette& IconSource,
	UGridItemSystemAsset& TransientRoot, FSummary& OutSummary)
{
	OutSummary = FSummary();
	if (Manifest.Num() != 22 ||
		!TransientRoot.ItemDefinitions.IsEmpty() ||
		TransientRoot.WeaponCatalogue || TransientRoot.WeaponVisualCatalogue ||
		TransientRoot.ActionCatalogue || TransientRoot.ActionPresentationCatalogue)
	{
		return false;
	}
	if (!GridArmClean06ProductionCandidate::BuildDefinitionsAndVisuals(
			Manifest, VisualSource, IconSource, TransientRoot, OutSummary.Items))
	{
		return false;
	}
	if (!GridArmClean06ProductionActionsCandidate::AddProvenActionsAndPresentations(
			Manifest, NumericSource, TransientRoot, OutSummary.Actions))
	{
		return false;
	}

	UGridCanonicalWeaponCatalogueAsset* ProjectedNumerics =
		NewObject<UGridCanonicalWeaponCatalogueAsset>(&TransientRoot);
	if (!GridArmClean06ProductionNumericsCandidate::Build(
			NumericSource, Manifest, *ProjectedNumerics, OutSummary.Numerics))
	{
		return false;
	}
	TransientRoot.WeaponCatalogue = ProjectedNumerics;

	TSet<FName> Seen;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ArmId(*Row.ArchetypeId);
		if (Row.ProductionState != TEXT("Active") || Seen.Contains(ArmId))
		{
			return false;
		}
		Seen.Add(ArmId);
		const UGridItemDefinitionAsset* Item =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		FGridCanonicalWeaponArchetype* Arm = ProjectedNumerics->Archetypes.FindByPredicate(
			[ArmId](const FGridCanonicalWeaponArchetype& Candidate)
			{
				return Candidate.ArchetypeId == ArmId;
			});
		if (!IsValid(Item) || !Arm || !Arm->bHandAndKindProven ||
			Item->ItemDefinitionId != FName(*Row.ItemDefinitionId) ||
			Item->WeaponArchetypeId != ArmId)
		{
			return false;
		}
		const bool bWasDirty = Item->GetOutermost()->IsDirty();
		if (Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand))
		{
			Arm->AttackSourceSlots.Add(EGridEquipmentSlot::MainHand);
			++OutSummary.AttacksMainHand;
		}
		if (Item->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand))
		{
			Arm->AttackSourceSlots.Add(EGridEquipmentSlot::OffHand);
			++OutSummary.AttacksOffHand;
		}
		const FGridItemActionBinding* Link =
			OutSummary.Actions.BindingsByArchetype.Find(ArmId);
		if (Arm->Kind == EGridCanonicalArmamentKind::Weapon)
		{
			if (Arm->AttackSourceSlots.IsEmpty())
			{
				return false;
			}
			if (Link)
			{
				if (Link->ActionId != FName(*Row.AttackId) ||
					!TransientRoot.ActionCatalogue->Find(Link->ActionId) ||
					!TransientRoot.ActionPresentationCatalogue->Find(Link->PresentationId))
				{
					return false;
				}
				Arm->Actions.Add(*Link);
			}
			else
			{
				++OutSummary.UnboundOffensiveArmaments;
			}
		}
		else if (!Arm->AttackSourceSlots.IsEmpty() || Link)
		{
			return false;
		}
		if (!Item->RequiredAmmunitionItemDefinitionId.IsNone())
		{
			++OutSummary.UnmappedAmmunition;
		}
		if (Item->bCombatThrowWeapon)
		{
			++OutSummary.UnconvertedThrowPolicies;
		}
		if (Item->GetOutermost()->IsDirty() != bWasDirty)
		{
			return false;
		}
	}
	if (Seen.Num() != 22 || OutSummary.AttacksMainHand != 19 ||
		OutSummary.AttacksOffHand != 2 ||
		OutSummary.UnboundOffensiveArmaments !=
			19 - OutSummary.Actions.BoundActions)
	{
		return false;
	}
	OutSummary.InvalidCanonicalIdentities = OutSummary.Items.NonCanonicalIdentities;
	TArray<FString> ValidationErrors;
	OutSummary.bRegistryValid = TransientRoot.Validate(ValidationErrors);
	// Root may be invalid until action/presentation IDs, ammo kinds, throws,
	// and any custom local effects are migrated. Invalidity is reported,
	// not papered over by inventing entries or silently omitting attack hands.
	return true;
}
