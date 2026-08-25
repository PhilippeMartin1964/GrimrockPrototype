#include "Runtime/GridItemDefinitionAsset.h"

#include "Engine/StaticMesh.h"

bool UGridItemDefinitionAsset::IsValidDefinition() const
{
	if (ItemDefinitionId.IsNone())
	{
		return false;
	}

	if (!HasValidCombatActions())
	{
		return false;
	}

	if (bProvidesQuickItemCombatAction)
	{
		FGridCombatActionDefinition QuickItemDefinition;
		if (!BuildQuickItemCombatActionDefinition(QuickItemDefinition))
		{
			return false;
		}
	}

	if (!bProvidesAttack)
	{
		return true;
	}

	return HasValidOffensiveProfile() &&
		(CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand) || CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::OffHand));
}

bool UGridItemDefinitionAsset::CanEquipToSlot(EGridEquipmentSlot Slot) const
{
	return Slot != EGridEquipmentSlot::None && CompatibleEquipmentSlots.Contains(Slot);
}

bool UGridItemDefinitionAsset::HasValidOffensiveProfile() const
{
	return bProvidesAttack && OffensiveProfile.IsValid();
}

bool UGridItemDefinitionAsset::CanProvideAttackFromSlot(EGridEquipmentSlot Slot) const
{
	if ((Slot != EGridEquipmentSlot::MainHand && Slot != EGridEquipmentSlot::OffHand) || !CompatibleEquipmentSlots.Contains(Slot))
	{
		return false;
	}
	if (!CombatActions.IsEmpty())
	{
		return CombatActions.ContainsByPredicate(
			[](const FGridCombatActionDefinition& Action)
			{
				return Action.IsValid() && Action.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
					Action.ResolutionProfile == EGridCombatActionResolutionProfile::Attack;
			});
	}
	return HasValidOffensiveProfile();
}

bool UGridItemDefinitionAsset::HasValidCombatActions() const
{
	TSet<FName> ActionIds;
	for (const FGridCombatActionDefinition& Action : CombatActions)
	{
		if (!Action.IsValid() || Action.SourcePolicy != EGridCombatActionSourcePolicy::Equipment || ActionIds.Contains(Action.ActionId))
		{
			return false;
		}
		ActionIds.Add(Action.ActionId);
	}
	return true;
}

bool UGridItemDefinitionAsset::BuildQuickItemCombatActionDefinition(FGridCombatActionDefinition& OutDefinition) const
{
	OutDefinition = FGridCombatActionDefinition();
	if (!bProvidesQuickItemCombatAction || ItemDefinitionId.IsNone() || (ItemType != EGridItemType::Potion && ItemType != EGridItemType::Scroll))
	{
		return false;
	}

	OutDefinition = QuickItemCombatAction;
	OutDefinition.ActionId = FGridCombatHotbarBinding::MakeQuickItemActionId(ItemDefinitionId);
	OutDefinition.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	OutDefinition.ResourceCosts.SourceItemQuantityCost = bThrowable ? 1 : FMath::Max(1, OutDefinition.ResourceCosts.SourceItemQuantityCost);
	if (OutDefinition.DisplayName.IsEmpty())
	{
		OutDefinition.DisplayName = DisplayName;
	}
	if (OutDefinition.Description.IsEmpty())
	{
		OutDefinition.Description = Description;
	}
	if (OutDefinition.Icon.IsNull())
	{
		OutDefinition.Icon = Icon;
	}

	if (!OutDefinition.IsValid())
	{
		OutDefinition = FGridCombatActionDefinition();
		return false;
	}

	const bool bSupportedAttack = OutDefinition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
		OutDefinition.TargetingPolicy == EGridCombatTargetingPolicy::FirstAxialTarget;
	const bool bSupportedSelfEffect = OutDefinition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		OutDefinition.TargetingPolicy == EGridCombatTargetingPolicy::Self && OutDefinition.EffectProfile.IsValid();
	if (!bSupportedAttack && !bSupportedSelfEffect)
	{
		OutDefinition = FGridCombatActionDefinition();
		return false;
	}
	return true;
}

bool UGridItemDefinitionAsset::BuildInventoryCombatActionDefinition(int32 DefaultAttackActionPointCost, FGridCombatActionDefinition& OutDefinition) const
{
	if (BuildQuickItemCombatActionDefinition(OutDefinition))
	{
		return true;
	}

	OutDefinition = FGridCombatActionDefinition();
	if (!bThrowable || ItemDefinitionId.IsNone())
	{
		return false;
	}

	const FGridCombatActionDefinition* ConfiguredThrow = CombatActions.FindByPredicate(
		[](const FGridCombatActionDefinition& Candidate)
		{
			return Candidate.IsValid() && Candidate.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
				Candidate.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
				Candidate.TargetingPolicy == EGridCombatTargetingPolicy::FirstAxialTarget;
		});
	if (ConfiguredThrow)
	{
		OutDefinition = *ConfiguredThrow;
	}
	else if (HasValidOffensiveProfile() &&
		(CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand) || CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::OffHand)))
	{
		OutDefinition.ActionType = EGridCombatActionType::RangedAttack;
		OutDefinition.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		OutDefinition.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		OutDefinition.ActionPointCost = FMath::Clamp(DefaultAttackActionPointCost, 1, 6);
		OutDefinition.RangeCells = OffensiveProfile.RangeCells;
		OutDefinition.PresentationProfileId = OffensiveProfile.AttackId;
		OutDefinition.OffensiveProfile = OffensiveProfile;
	}
	else
	{
		return false;
	}

	OutDefinition.ActionId = FGridCombatHotbarBinding::MakeQuickItemActionId(ItemDefinitionId);
	OutDefinition.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	OutDefinition.ResourceCosts.SourceItemQuantityCost = 1;
	if (OutDefinition.DisplayName.IsEmpty())
	{
		OutDefinition.DisplayName = DisplayName;
	}
	if (OutDefinition.Description.IsEmpty())
	{
		OutDefinition.Description = Description;
	}
	if (OutDefinition.Icon.IsNull())
	{
		OutDefinition.Icon = Icon;
	}

	if (!OutDefinition.IsValid())
	{
		OutDefinition = FGridCombatActionDefinition();
		return false;
	}
	return true;
}

bool UGridItemDefinitionAsset::HasValidPlayerAttackPresentation() const
{
	return bProvidesAttackPresentation && PlayerAttackPresentationProfile.IsValid() &&
		(PlayerAttackPresentationProfile.MotionStyle != EGridPlayerAttackMotionStyle::Throw || bThrowable);
}

UStaticMesh* UGridItemDefinitionAsset::LoadHeldMesh() const
{
	if (!EquippedMesh.IsNull())
	{
		if (UStaticMesh* Mesh = EquippedMesh.LoadSynchronous())
		{
			return Mesh;
		}
	}
	return WorldMesh.LoadSynchronous();
}
