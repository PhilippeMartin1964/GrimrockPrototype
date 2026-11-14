#pragma once

#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Combat/GridDualWieldPolicy.h"

/**
 * Cosmetic category of an ALREADY AVAILABLE equipped action.
 * Never grants an attack, a passive guard, a block or a spell.
 * Canonical ArchetypeId is the only ARM group authority; no old DA_Weapon.
 */
struct FGridCanonicalActionIconReadModel
{
    static EGridEquippedWeaponActionIconKind Classify(
        const FGridResolvedItem& Item, const FGridCombatActionDefinition& Action)
    {
        using Kind = EGridEquippedWeaponActionIconKind;
        if (!Item.IsValid() || !Item.bIsArmament ||
            Action.SourcePolicy != EGridCombatActionSourcePolicy::Equipment ||
            Action.ActionId.IsNone())
        {
            return Kind::None;
        }

        const bool bDefense = Action.ResolutionProfile == EGridCombatActionResolutionProfile::Defense ||
            Action.ActionType == EGridCombatActionType::Defend;
        const bool bAttack = Action.ResolutionProfile == EGridCombatActionResolutionProfile::Attack;
        if (!bDefense && !bAttack)
        {
            return Kind::None;
        }

        // Dual wield is the only special composite: it is supplied by the
        // existing physical two-weapon policy, not added to a weapon catalogue.
        if (Action.ActionId == FGridDualWieldPolicy::ActionId())
        {
            return bAttack && Action.ActionType == EGridCombatActionType::MeleeAttack &&
                Item.Weapon.Kind == EGridCanonicalArmamentKind::Weapon
                ? Kind::MeleeDualWield : Kind::None;
        }

        // An icon for an unbound ActionId would accidentally imply an active
        // action on passive equipment. Requiring a bound action prevents it.
        if (!Item.AvailableActions.ContainsByPredicate(
            [&Action](const FGridItemActionBinding& Binding)
            {
                return Binding.ActionId == Action.ActionId;
            }))
        {
            return Kind::None;
        }

        const FString Archetype = Item.ArchetypeId.ToString();
        if (Archetype.Len() != 6 || !Archetype.StartsWith(TEXT("ARM")) ||
            !Archetype.Mid(3).IsNumeric())
        {
            return Kind::None;
        }
        const int32 Id = FCString::Atoi(*Archetype.Mid(3));
        if (Id < 1 || Id > 82)
        {
            return Kind::None;
        }
        if (bDefense)
        {
            if (Item.Weapon.Kind == EGridCanonicalArmamentKind::Shield && Id >= 72 && Id <= 82)
            {
                return Kind::DefenseBlock;
            }
            return Id == 13 && Item.Weapon.Kind == EGridCanonicalArmamentKind::Weapon
                ? Kind::DefenseParry : Kind::None;
        }
        if (Item.Weapon.Kind != EGridCanonicalArmamentKind::Weapon)
        {
            return Kind::None; // Passive focus and shield have no attack icon.
        }

        // These are presentation families only; gameplay action existence
        // and source-hand permission were already checked by the caller.
        if (Action.ActionType == EGridCombatActionType::RangedAttack && Id <= 44)
        {
            return Kind::ThrownWeapon; // A real authored throwing mode.
        }
        if (Id >= 1 && Id <= 44 && Action.ActionType == EGridCombatActionType::MeleeAttack)
        {
            if (Id <= 26) return Kind::MeleeOneHand;
            if (Id <= 37) return Kind::MeleeTwoHanded;
            return Kind::MeleePolearm;
        }
        if (Id >= 45 && Id <= 53 && Action.ActionType == EGridCombatActionType::RangedAttack)
        {
            if (Id <= 48) return Kind::RangedBow;
            if (Id <= 51) return Kind::RangedCrossbow;
            return Kind::RangedOther;
        }
        if (Id >= 54 && Id <= 60)
        {
            if (Action.ActionType == EGridCombatActionType::MeleeAttack) return Kind::MeleeOneHand;
            if (Action.ActionType == EGridCombatActionType::RangedAttack) return Kind::ThrownWeapon;
        }
        if (Id >= 61 && Id <= 69 &&
            (Action.ActionType == EGridCombatActionType::MeleeAttack ||
             Action.ActionType == EGridCombatActionType::RangedAttack))
        {
            return (Id <= 65 || Id == 69) ? Kind::MagicOneHand : Kind::MagicTwoHanded;
        }
        return Kind::None;
    }
};
