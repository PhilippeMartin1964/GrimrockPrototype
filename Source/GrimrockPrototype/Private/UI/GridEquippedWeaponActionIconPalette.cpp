#include "UI/GridEquippedWeaponActionIconPalette.h"

#include "Runtime/Combat/GridDualWieldPolicy.h"
#include "Runtime/GridItemDefinitionAsset.h"

TSoftObjectPtr<UTexture2D> UGridEquippedWeaponActionIconPalette::FindIcon(EGridEquippedWeaponActionIconKind Kind) const
{
	const TSoftObjectPtr<UTexture2D>* Found = Icons.Find(Kind);
	return Found ? *Found : TSoftObjectPtr<UTexture2D>();
}

EGridEquippedWeaponActionIconKind FGridEquippedWeaponActionIconResolver::Classify(
	const FGridAvailableCombatAction& Action, const UGridItemDefinitionAsset* Definition)
{
	using EKind = EGridEquippedWeaponActionIconKind;
	if (!IsValid(Definition) || Action.Definition.SourcePolicy != EGridCombatActionSourcePolicy::Equipment ||
		(Definition->ItemType != EGridItemType::Weapon && Definition->ItemType != EGridItemType::Shield))
	{
		return EKind::None;
	}

	// Passive shields/orbs have no catalogue action: displaying a defensive
	// shortcut requires a REAL authored defense action, never equipment alone.
	const bool bDefense = Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Defense ||
		Action.Definition.ActionType == EGridCombatActionType::Defend;
	const bool bAttack = Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack;
	if (!bDefense && !bAttack)
	{
		return EKind::None;
	}
	if (bAttack && Action.Definition.ActionId == FGridDualWieldPolicy::ActionId())
	{
		return EKind::MeleeDualWield;
	}

	// Legacy, non-catalogue items retain their own icon instead of being
	// classified heuristically from a localized display name.
	if (!Definition->bUseWeaponCatalogue)
	{
		return EKind::None;
	}
	const FString Archetype = Definition->WeaponArchetypeId.ToString();
	if (Archetype.Len() != 6 || !Archetype.StartsWith(TEXT("ARM")) || !Archetype.Mid(3).IsNumeric())
	{
		return EKind::None;
	}
	const int32 Id = FCString::Atoi(*Archetype.Mid(3));
	if (Id < 1 || Id > 82)
	{
		return EKind::None;
	}

	if (bDefense)
	{
		// A future action authored on ARM013 can use a distinct parry icon.
		if (Definition->ItemType == EGridItemType::Shield && Id >= 72 && Id <= 82)
		{
			return EKind::DefenseBlock;
		}
		return Id == 13 ? EKind::DefenseParry : EKind::None;
	}
	if (Definition->ItemType != EGridItemType::Weapon)
	{
		return EKind::None;
	}

	// ARMEMENT01.2 catalogue (82 immutable archetypes). Visual-only mapping:
	// the actual action cost, range and weapon restrictions remain in the
	// item and TurnManager; this switch NEVER grants an attack.
	// An authored THROW action takes precedence over the base hand family:
	// e.g. a throwable hatchet may have a distinct melee and throw action.
	if (Action.Definition.ActionType == EGridCombatActionType::RangedAttack &&
		Definition->IsCombatThrowable())
	{
		return EKind::ThrownWeapon;
	}
	if (Id <= 44 && Action.Definition.ActionType == EGridCombatActionType::MeleeAttack)
	{
		if (Id <= 26) return EKind::MeleeOneHand;
		if (Id <= 37) return EKind::MeleeTwoHanded;
		return EKind::MeleePolearm;
	}
	if (Id >= 45 && Id <= 53 && Action.Definition.ActionType == EGridCombatActionType::RangedAttack)
	{
		if (Id <= 48) return EKind::RangedBow;
		if (Id <= 51) return EKind::RangedCrossbow;
		return EKind::RangedOther;
	}
	if (Id >= 54 && Id <= 60)
	{
		if (Action.Definition.ActionType == EGridCombatActionType::MeleeAttack)
		{
			return EKind::MeleeOneHand;
		}
		if (Action.Definition.ActionType == EGridCombatActionType::RangedAttack)
		{
			return EKind::ThrownWeapon;
		}
	}
	if (Id >= 61 && Id <= 69)
	{
		if (Action.Definition.ActionType != EGridCombatActionType::RangedAttack &&
			Action.Definition.ActionType != EGridCombatActionType::MeleeAttack)
		{
			return EKind::None;
		}
		if (Id <= 65 || Id == 69) return EKind::MagicOneHand;
		return EKind::MagicTwoHanded;
	}
	// ARM070-071 are passive focus equipment; ARM072-082 passive shields.
	return EKind::None;
}
