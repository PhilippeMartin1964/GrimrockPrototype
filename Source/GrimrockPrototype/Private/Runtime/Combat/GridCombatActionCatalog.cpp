#include "Runtime/Combat/GridCombatActionCatalog.h"

#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridItemDefinitionAsset.h"

#define LOCTEXT_NAMESPACE "GridCombatActionCatalog"

namespace
{
    bool IsClassActionSource (
        EGridCombatActionSourcePolicy SourcePolicy)
    {
        return SourcePolicy == EGridCombatActionSourcePolicy::Ability ||
            SourcePolicy == EGridCombatActionSourcePolicy::Spell;
    }

    bool IsSupportedAttackTargeting (
        EGridCombatTargetingPolicy TargetingPolicy)
    {
        return TargetingPolicy ==
                EGridCombatTargetingPolicy::FirstAxialTarget ||
            TargetingPolicy == EGridCombatTargetingPolicy::Cell ||
            TargetingPolicy == EGridCombatTargetingPolicy::Area;
    }

    bool IsUI0143e2SpellbookProjection (
        const FGridCombatActionContribution& Contribution)
    {
        const FGridCombatActionDefinition& Definition =
            Contribution.Definition;
        const bool bSupportedTargeting =
            Definition.TargetingPolicy ==
                EGridCombatTargetingPolicy::Self ||
            Definition.TargetingPolicy ==
                EGridCombatTargetingPolicy::Ally ||
            Definition.TargetingPolicy ==
                EGridCombatTargetingPolicy::FirstAxialTarget;
        return Definition.SourcePolicy ==
                EGridCombatActionSourcePolicy::Spell &&
            !Definition.ActionId.IsNone () &&
            Contribution.SourceDefinitionId == Definition.ActionId &&
            Definition.ResolutionProfile ==
                EGridCombatActionResolutionProfile::Effect &&
            bSupportedTargeting;
    }

    EGridCombatActionAvailabilityReason EvaluateMON126Availability (
        const FGridCombatActionCatalogContext& Context,
        const FGridCombatActionContribution& Contribution)
    {
        const FGridCombatActionDefinition& Definition =
            Contribution.Definition;
        if (Context.CharacterIndex == INDEX_NONE ||
            !Context.CharacterId.IsValid ())
        {
            return EGridCombatActionAvailabilityReason::InvalidCharacter;
        }
        if (!Context.bCombatActive)
        {
            return EGridCombatActionAvailabilityReason::CombatInactive;
        }
        if (Context.bCharacterDefeated)
        {
            return EGridCombatActionAvailabilityReason::CharacterDefeated;
        }
        if (!Context.bActiveCombatant)
        {
            return EGridCombatActionAvailabilityReason::NotActiveCombatant;
        }
        if (Context.bPartyBusy)
        {
            return EGridCombatActionAvailabilityReason::PartyBusy;
        }
        if (Context.RemainingActionPoints < Definition.ActionPointCost)
        {
            return EGridCombatActionAvailabilityReason::
                InsufficientActionPoints;
        }
        if (Context.CurrentMana < Definition.ResourceCosts.ManaCost)
        {
            return EGridCombatActionAvailabilityReason::InsufficientMana;
        }
        if (Definition.ResourceCosts.SourceItemQuantityCost > 0 &&
            Contribution.AvailableSourceQuantity <
                Definition.ResourceCosts.SourceItemQuantityCost)
        {
            return EGridCombatActionAvailabilityReason::
                InsufficientSourceItems;
        }
        for (const FName Requirement : Definition.Requirements)
        {
            if (!Context.SatisfiedRequirements.Contains (Requirement))
            {
                return EGridCombatActionAvailabilityReason::
                    MissingRequirement;
            }
        }
        if (const int32* RemainingCooldown =
            Context.RemainingCooldownRounds.Find (Definition.ActionId))
        {
            if (*RemainingCooldown > 0)
            {
                return EGridCombatActionAvailabilityReason::CooldownActive;
            }
        }

        if (Definition.SourcePolicy ==
            EGridCombatActionSourcePolicy::QuickItem)
        {
            const bool bSupportedQuickItemProfile =
                (Definition.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Attack &&
                    IsSupportedAttackTargeting (
                        Definition.TargetingPolicy)) ||
                (Definition.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Effect &&
                    Definition.TargetingPolicy ==
                        EGridCombatTargetingPolicy::Self &&
                    Definition.EffectProfile.IsValid ());
            if (!Context.bEnableQuickItemExecutors ||
                !bSupportedQuickItemProfile)
            {
                return EGridCombatActionAvailabilityReason::
                    ExecutionNotImplemented;
            }

            if (Definition.ResolutionProfile ==
                    EGridCombatActionResolutionProfile::Effect)
            {
                const int32 HealthAfter = FMath::Clamp (
                    Context.CurrentHealth +
                        Definition.EffectProfile.RestoreHealth,
                    0,
                    FMath::Max (0, Context.MaximumHealth));
                const int32 ManaAfter = FMath::Clamp (
                    Context.CurrentMana -
                        Definition.ResourceCosts.ManaCost +
                        Definition.EffectProfile.RestoreMana,
                    0,
                    FMath::Max (0, Context.MaximumMana));
                if (HealthAfter <= Context.CurrentHealth &&
                    ManaAfter <= Context.CurrentMana)
                {
                    return EGridCombatActionAvailabilityReason::
                        NoApplicableEffect;
                }
            }
        }
        else if (IsUI0143e2SpellbookProjection (Contribution))
        {
            // UI01.4.3e.2 has a dedicated Spellbook executor in the TurnManager.
            // The existing non-item executor gate is already enabled by the
            // runtime catalogue context; do not route these projections through
            // the legacy generic class-action shape validation below.
            if (!Context.bEnableClassActionExecutors)
            {
                return EGridCombatActionAvailabilityReason::
                    ExecutionNotImplemented;
            }
        }
        else if (IsClassActionSource (Definition.SourcePolicy))
        {
            const bool bSupportedAttack =
                Definition.ResolutionProfile ==
                    EGridCombatActionResolutionProfile::Attack &&
                IsSupportedAttackTargeting (
                    Definition.TargetingPolicy);
            const bool bSupportedSelfEffect =
                Definition.ResolutionProfile ==
                    EGridCombatActionResolutionProfile::Effect &&
                Definition.TargetingPolicy ==
                    EGridCombatTargetingPolicy::Self &&
                Definition.EffectProfile.IsValid ();
            if (!Context.bEnableClassActionExecutors ||
                (!bSupportedAttack && !bSupportedSelfEffect))
            {
                return EGridCombatActionAvailabilityReason::
                    ExecutionNotImplemented;
            }

            if (bSupportedSelfEffect)
            {
                const int32 HealthAfter = FMath::Clamp (
                    Context.CurrentHealth +
                        Definition.EffectProfile.RestoreHealth,
                    0,
                    FMath::Max (0, Context.MaximumHealth));
                const int32 ManaAfter = FMath::Clamp (
                    Context.CurrentMana -
                        Definition.ResourceCosts.ManaCost +
                        Definition.EffectProfile.RestoreMana,
                    0,
                    FMath::Max (0, Context.MaximumMana));
                if (HealthAfter <= Context.CurrentHealth &&
                    ManaAfter <= Context.CurrentMana)
                {
                    return EGridCombatActionAvailabilityReason::
                        NoApplicableEffect;
                }
            }
        }
        else if (Definition.SourcePolicy ==
                EGridCombatActionSourcePolicy::Equipment &&
            (Definition.ResolutionProfile !=
                    EGridCombatActionResolutionProfile::Attack ||
                Definition.TargetingPolicy !=
                    EGridCombatTargetingPolicy::FirstAxialTarget))
        {
            return EGridCombatActionAvailabilityReason::
                ExecutionNotImplemented;
        }
        else if (Definition.ResolutionProfile !=
                    EGridCombatActionResolutionProfile::Attack ||
            !IsSupportedAttackTargeting (Definition.TargetingPolicy))
        {
            return EGridCombatActionAvailabilityReason::
                ExecutionNotImplemented;
        }
        return EGridCombatActionAvailabilityReason::None;
    }
}

void FGridCombatActionCatalog::Build (
    const FGridCombatActionCatalogContext& Context,
    const TArray<FGridCombatActionContribution>& Contributions,
    TArray<FGridAvailableCombatAction>& OutActions)
{
    FGridCombatActionCatalogContext EffectiveContext = Context;
    FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements (
        Context.CharacterId,
        EffectiveContext.SatisfiedRequirements);

    OutActions.Reset (Contributions.Num ());
    for (const FGridCombatActionContribution& Contribution : Contributions)
    {
        if (!Contribution.IsValid ())
        {
            continue;
        }

        FGridAvailableCombatAction Available;
        Available.Definition = Contribution.Definition;
        Available.CharacterIndex = EffectiveContext.CharacterIndex;
        Available.CharacterId = EffectiveContext.CharacterId;
        Available.SourceDefinitionId = Contribution.SourceDefinitionId;
        Available.SourceRuntimeId = Contribution.SourceRuntimeId;
        Available.SourceEquipmentSlot =
            Contribution.SourceEquipmentSlot;
        Available.CurrentActionPointCost =
            Contribution.Definition.ActionPointCost;
        Available.CurrentManaCost =
            Contribution.Definition.ResourceCosts.ManaCost;
        Available.CurrentSourceItemQuantityCost =
            Contribution.Definition.ResourceCosts.SourceItemQuantityCost;
        Available.CurrentSourceItemQuantity =
            Contribution.AvailableSourceQuantity;
        Available.AvailabilityReason = EvaluateMON126Availability (
            EffectiveContext,
            Contribution);
        Available.bEnabled =
            Available.AvailabilityReason ==
                EGridCombatActionAvailabilityReason::None;
        Available.DisabledReason = Available.bEnabled
            ? FText::GetEmpty ()
            : GetAvailabilityReasonText (
                Available.AvailabilityReason);
        OutActions.Add (MoveTemp (Available));
    }
}

FGridCombatActionDefinition
FGridCombatActionCatalog::MakeLegacyEquipmentAttackDefinition (
    const UGridItemDefinitionAsset& ItemDefinition,
    int32 ActionPointCost)
{
    FGridCombatActionDefinition Definition;
    if (!ItemDefinition.HasValidOffensiveProfile ())
    {
        return Definition;
    }

    Definition.ActionId = ItemDefinition.OffensiveProfile.AttackId;
    Definition.DisplayName = ItemDefinition.DisplayName;
    Definition.Description = ItemDefinition.Description;
    Definition.Icon = ItemDefinition.Icon;
    Definition.ActionType =
        ItemDefinition.bThrowable ||
            ItemDefinition.OffensiveProfile.RangeCells > 1
        ? EGridCombatActionType::RangedAttack
        : EGridCombatActionType::MeleeAttack;
    Definition.SourcePolicy =
        EGridCombatActionSourcePolicy::Equipment;
    Definition.TargetingPolicy =
        EGridCombatTargetingPolicy::FirstAxialTarget;
    Definition.ResolutionProfile =
        EGridCombatActionResolutionProfile::Attack;
    Definition.ActionPointCost = FMath::Clamp (
        ActionPointCost,
        1,
        6);
    Definition.ResourceCosts.SourceItemQuantityCost =
        ItemDefinition.bThrowable ? 1 : 0;
    Definition.RangeCells = ItemDefinition.OffensiveProfile.RangeCells;
    Definition.PresentationProfileId =
        ItemDefinition.OffensiveProfile.AttackId;
    Definition.OffensiveProfile = ItemDefinition.OffensiveProfile;
    return Definition;
}

FGridCombatActionDefinition
FGridCombatActionCatalog::MakeUnarmedAttackDefinition (
    int32 ActionPointCost)
{
    FGridCombatActionDefinition Definition;
    Definition.ActionId = TEXT ("Attack_Unarmed");
    Definition.DisplayName = LOCTEXT ("UnarmedName", "À mains nues");
    Definition.Description = LOCTEXT (
        "UnarmedDescription",
        "Attaque physique effectuée sans arme.");
    Definition.ActionType = EGridCombatActionType::MeleeAttack;
    Definition.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
    Definition.TargetingPolicy =
        EGridCombatTargetingPolicy::FirstAxialTarget;
    Definition.ResolutionProfile =
        EGridCombatActionResolutionProfile::Attack;
    Definition.ActionPointCost = FMath::Clamp (
        ActionPointCost,
        1,
        6);
    Definition.RangeCells = 1;
    Definition.PresentationProfileId = TEXT ("Unarmed");
    Definition.OffensiveProfile.AttackId = TEXT ("Attack_Unarmed");
    Definition.OffensiveProfile.AttackDefinition.DamageType =
        EGridDamageType::Physical;
    Definition.OffensiveProfile.AttackDefinition.PhysicalSubtype =
        EGridPhysicalDamageSubtype::Bludgeoning;
    Definition.OffensiveProfile.AttackDefinition.MinDamage = 1;
    Definition.OffensiveProfile.AttackDefinition.MaxDamage = 3;
    Definition.OffensiveProfile.DamageScalingAttribute =
        EGridAttackScalingAttribute::Strength;
    Definition.OffensiveProfile.RangeCells = 1;
    return Definition;
}

FText FGridCombatActionCatalog::GetAvailabilityReasonText (
    EGridCombatActionAvailabilityReason Reason)
{
    switch (Reason)
    {
    case EGridCombatActionAvailabilityReason::CombatInactive:
        return LOCTEXT ("CombatInactive", "Aucun combat n’est actif.");
    case EGridCombatActionAvailabilityReason::InvalidCharacter:
        return LOCTEXT ("InvalidCharacter", "Ce personnage est invalide.");
    case EGridCombatActionAvailabilityReason::CharacterDefeated:
        return LOCTEXT ("CharacterDefeated", "Ce personnage est vaincu.");
    case EGridCombatActionAvailabilityReason::NotActiveCombatant:
        return LOCTEXT (
            "NotActiveCombatant",
            "Ce n’est pas le tour de ce personnage.");
    case EGridCombatActionAvailabilityReason::PartyBusy:
        return LOCTEXT ("PartyBusy", "Le groupe est occupé.");
    case EGridCombatActionAvailabilityReason::InsufficientActionPoints:
        return LOCTEXT (
            "InsufficientActionPoints",
            "Ce personnage n’a pas assez de points d’action.");
    case EGridCombatActionAvailabilityReason::InsufficientMana:
        return LOCTEXT (
            "InsufficientMana",
            "Ce personnage n’a pas assez de mana.");
    case EGridCombatActionAvailabilityReason::InsufficientSourceItems:
        return LOCTEXT (
            "InsufficientSourceItems",
            "La source ne contient pas assez d’unités.");
    case EGridCombatActionAvailabilityReason::MissingRequirement:
        return LOCTEXT (
            "MissingRequirement",
            "Une condition requise n’est pas satisfaite.");
    case EGridCombatActionAvailabilityReason::CooldownActive:
        return LOCTEXT (
            "CooldownActive",
            "Cette action est encore en recharge.");
    case EGridCombatActionAvailabilityReason::NoApplicableEffect:
        return LOCTEXT (
            "NoApplicableEffect",
            "Cette action n’aurait actuellement aucun effet utile.");
    case EGridCombatActionAvailabilityReason::ExecutionNotImplemented:
        return LOCTEXT (
            "ExecutionNotImplemented",
            "L’exécution de cette action sera ajoutée dans un prochain jalon.");
    case EGridCombatActionAvailabilityReason::None:
    default:
        return FText::GetEmpty ();
    }
}

#undef LOCTEXT_NAMESPACE
