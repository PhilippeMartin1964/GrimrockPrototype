#include "Runtime/GridItemDefinitionAsset.h"

#include "Engine/StaticMesh.h"

namespace
{
	constexpr float GridThrowableWeightPerStrengthKg = 0.25f;
	constexpr float GridThrowReferenceStrength = 10.0f;
	constexpr float GridThrowMaxWeightSpeedPenalty = 0.35f;
}

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

	return true;
}

bool UGridItemDefinitionAsset::CanEquipToSlot(EGridEquipmentSlot Slot) const
{
	if (Slot == EGridEquipmentSlot::None)
	{
		return false;
	}

	if ((Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand) && GetEffectiveHandUsage() == EGridItemHandUsage::OneHanded)
	{
		return true;
	}

	return CompatibleEquipmentSlots.Contains(Slot);
}

EGridItemHandUsage UGridItemDefinitionAsset::GetEffectiveHandUsage() const
{
	if (HandUsage != EGridItemHandUsage::Auto)
	{
		return HandUsage;
	}

	if (bThrowable || CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand) || CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::OffHand))
	{
		return EGridItemHandUsage::OneHanded;
	}

	return EGridItemHandUsage::NotHandHeld;
}

bool UGridItemDefinitionAsset::IsPhysicallyThrowable() const
{
	return GetEffectiveHandUsage() == EGridItemHandUsage::OneHanded && FMath::IsFinite(Weight) && Weight >= 0.0f && FMath::IsFinite(ThrowSpeed) &&
		ThrowSpeed > KINDA_SMALL_NUMBER;
}

bool UGridItemDefinitionAsset::IsCombatThrowable() const
{
	return bCombatThrowWeapon || bThrowable;
}

float UGridItemDefinitionAsset::GetMaxThrowableWeightForStrength(int32 Strength) const
{
	return FMath::Max(0, Strength) * GridThrowableWeightPerStrengthKg;
}

bool UGridItemDefinitionAsset::CanBeThrownByStrength(int32 Strength) const
{
	if (Strength <= 0 || !IsPhysicallyThrowable())
	{
		return false;
	}

	return Weight <= GetMaxThrowableWeightForStrength(Strength) + KINDA_SMALL_NUMBER;
}

float UGridItemDefinitionAsset::GetThrowSpeedScaleForStrength(int32 Strength) const
{
	if (!CanBeThrownByStrength(Strength))
	{
		return 0.0f;
	}

	const float MaxWeight = FMath::Max(KINDA_SMALL_NUMBER, GetMaxThrowableWeightForStrength(Strength));
	const float WeightRatio = FMath::Clamp(Weight / MaxWeight, 0.0f, 1.0f);
	const float StrengthFactor = FMath::Clamp(FMath::Sqrt(FMath::Max(1.0f, static_cast<float>(Strength)) / GridThrowReferenceStrength), 0.65f, 1.5f);
	const float WeightFactor = FMath::Lerp(1.0f, 1.0f - GridThrowMaxWeightSpeedPenalty, WeightRatio);
	return FMath::Clamp(StrengthFactor * WeightFactor, 0.5f, 1.5f);
}

bool UGridItemDefinitionAsset::CanProvideAttackFromSlot(EGridEquipmentSlot Slot) const
{
	if ((Slot != EGridEquipmentSlot::MainHand && Slot != EGridEquipmentSlot::OffHand) || !CompatibleEquipmentSlots.Contains(Slot))
	{
		return false;
	}
	return CombatActions.ContainsByPredicate(
		[](const FGridCombatActionDefinition& Action)
		{
			return Action.IsValid() && Action.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
				Action.ResolutionProfile == EGridCombatActionResolutionProfile::Attack;
		});
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
	OutDefinition.ResourceCosts.SourceItemQuantityCost = IsCombatThrowable() ? 1 : FMath::Max(1, OutDefinition.ResourceCosts.SourceItemQuantityCost);
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

bool UGridItemDefinitionAsset::BuildInventoryCombatActionDefinition(FGridCombatActionDefinition& OutDefinition) const
{
	if (BuildQuickItemCombatActionDefinition(OutDefinition))
	{
		return true;
	}

	OutDefinition = FGridCombatActionDefinition();
	if (!IsCombatThrowable() || ItemDefinitionId.IsNone())
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
	if (!ConfiguredThrow)
	{
		return false;
	}
	OutDefinition = *ConfiguredThrow;

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
		(PlayerAttackPresentationProfile.MotionStyle != EGridPlayerAttackMotionStyle::Throw || IsCombatThrowable());
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
