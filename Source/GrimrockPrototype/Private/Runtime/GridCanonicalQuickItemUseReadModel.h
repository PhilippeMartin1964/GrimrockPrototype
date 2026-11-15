#pragma once

#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.3AX: strict, transient projection of exactly one authored
 * canonical self-Heal Use action onto the already proven QuickItem executor.
 *
 * Zero AP is intentional: canonical Use has NO authored AP field. The schema
 * must be extended and content converted before an AP cost can be charged.
 * Reject other kinds/effects instead of inventing an execution policy.
 */
struct FGridCanonicalQuickItemUseReadView
{
    bool bCanonicalIdentity = false;
    bool bResolved = false;
    bool bExplorationCompatible = false;
    FGridCombatActionDefinition Action;
};

struct FGridCanonicalQuickItemUseReadModel
{
    static bool TryProjectSingleHeal(
        const FGridResolvedItem& Item, const FGridResolvedBoundAction& Bound,
        FGridCombatActionDefinition& Out)
    {
        Out = FGridCombatActionDefinition();
        if (!Item.IsValid() || Item.bIsArmament || !Item.AmmoKindId.IsNone() ||
            !Bound.IsValid() || Bound.ItemId != Item.ItemId ||
            Bound.ItemLevel != Item.ItemLevel ||
            Bound.Action.Kind != EGridCanonicalActionKind::Use ||
            Bound.Action.Targeting != EGridCombatTargetingPolicy::Self ||
            Bound.Action.bRangedAttack || Bound.Action.bRequiresLineOfSight ||
            Bound.Action.WeaponDamagePercent != 0 ||
            Bound.Action.SourceItemQuantityCost != 1 ||
            Bound.Action.ManaCost < 0 || Bound.Action.CooldownRounds < 0 ||
            Bound.Action.Effects.Num() != 1 ||
            Bound.Action.Effects[0].Kind != EGridCanonicalEffectKind::Heal ||
            Bound.Action.Effects[0].Magnitude <= 0 ||
            !Bound.Action.Effects[0].StatusId.IsNone())
        {
            return false;
        }

        FGridCombatActionDefinition Candidate;
        Candidate.ActionId = Bound.ActionId;
        Candidate.DisplayName = Item.DisplayName;
        Candidate.Description = Item.Description;
        Candidate.Icon = Item.ItemIcon;
        Candidate.ActionType = EGridCombatActionType::Ability;
        Candidate.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
        Candidate.SourceTags = Item.Tags;
        Candidate.TargetingPolicy = EGridCombatTargetingPolicy::Self;
        Candidate.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
        Candidate.ActionPointCost = 0; // No AP is authored by FGridCanonicalAction.
        Candidate.ResourceCosts.ManaCost = Bound.Action.ManaCost;
        Candidate.ResourceCosts.SourceItemQuantityCost = 1;
        Candidate.CooldownRounds = Bound.Action.CooldownRounds;
        Candidate.PresentationProfileId = Bound.PresentationId;
        Candidate.EffectProfile.RestoreHealth = Bound.Action.Effects[0].Magnitude;
        if (!Candidate.IsValid())
        {
            return false;
        }
        Out = MoveTemp(Candidate);
        return true;
    }

    static FGridCanonicalQuickItemUseReadView Resolve(
        const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
    {
        FGridCanonicalQuickItemUseReadView Out;
        Out.bCanonicalIdentity = Registry &&
            Registry->HasCanonicalItem(Instance.ItemDefinitionId);
        if (!Out.bCanonicalIdentity || !Instance.IsValid())
        {
            return Out;
        }

        FGridResolvedItem Item;
        if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Item) ||
            Item.AvailableActions.IsEmpty())
        {
            return Out; // Registered invalid N0/N21 must fail closed.
        }

        int32 SupportedCount = 0;
        for (const FGridItemActionBinding& Binding : Item.AvailableActions)
        {
            FGridResolvedBoundAction Bound;
            FGridCombatActionDefinition Candidate;
            if (!Registry->ResolveBoundAction(Instance.ItemDefinitionId,
                    Instance.ItemLevel, Binding.ActionId, Bound) ||
                !TryProjectSingleHeal(Item, Bound, Candidate))
            {
                continue;
            }
            ++SupportedCount;
            if (SupportedCount > 1)
            {
                // Hotbar only identifies ONE action per item source; never
                // select an arbitrary first of multiple self-Heal actions.
                return Out;
            }
            Out.Action = MoveTemp(Candidate);
        }
        if (SupportedCount != 1)
        {
            return Out;
        }
        Out.bResolved = true;
        Out.bExplorationCompatible =
            Out.Action.ResourceCosts.ManaCost == 0 &&
            Out.Action.CooldownRounds == 0;
        return Out;
    }
};
