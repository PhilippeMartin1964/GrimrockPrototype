#pragma once

#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridPuzzleCategoryTags.h"

/**
 * ARM-CLEAN06.3AX/AY/AZ/BA: strictly project one authored
 * Heal, RemoveStatus or ApplyStatus Use action onto the proven QuickItem
 * executor. Single explicit Ally targets are combat-only; status actions too.
 *
 * Zero AP is intentional: canonical Use has NO authored AP field. The schema
 * must be extended and content converted before an AP cost can be charged.
 * Reject other kinds/effects instead of inventing an execution policy.
 */
struct FGridQuickItemUseReadView
{
    bool bRegisteredIdentity = false;
    bool bResolved = false;
    bool bExplorationCompatible = false;
    FGridCombatActionDefinition Action;
};

struct FGridQuickItemUseReadModel
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
            (Bound.Action.Targeting != EGridCombatTargetingPolicy::Self &&
             Bound.Action.Targeting != EGridCombatTargetingPolicy::Ally) ||
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
        Candidate.SourceTags = GridPuzzleCategoryTags::WithoutReserved(Item.Tags);
        Candidate.TargetingPolicy = Bound.Action.Targeting;
        Candidate.MaximumResolvedTargets = 1;
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

    /** Single exact status cleansing, combat only; never invent status tags or
     * infer multiple removals from a level or a legacy definition. */
    static bool TryProjectSingleStatusRemoval(
        const FGridResolvedItem& Item, const FGridResolvedBoundAction& Bound,
        FGridCombatActionDefinition& Out)
    {
        Out = FGridCombatActionDefinition();
        if (!Item.IsValid() || Item.bIsArmament || !Item.AmmoKindId.IsNone() ||
            !Bound.IsValid() || Bound.ItemId != Item.ItemId ||
            Bound.ItemLevel != Item.ItemLevel ||
            Bound.Action.Kind != EGridCanonicalActionKind::Use ||
            (Bound.Action.Targeting != EGridCombatTargetingPolicy::Self &&
             Bound.Action.Targeting != EGridCombatTargetingPolicy::Ally) ||
            Bound.Action.bRangedAttack || Bound.Action.bRequiresLineOfSight ||
            Bound.Action.WeaponDamagePercent != 0 ||
            Bound.Action.SourceItemQuantityCost != 1 ||
            Bound.Action.ManaCost < 0 || Bound.Action.CooldownRounds < 0 ||
            Bound.Action.Effects.Num() != 1 ||
            Bound.Action.Effects[0].Kind != EGridCanonicalEffectKind::RemoveStatus ||
            Bound.Action.Effects[0].Magnitude != 1 ||
            Bound.Action.Effects[0].StatusId.IsNone())
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
        Candidate.SourceTags = GridPuzzleCategoryTags::WithoutReserved(Item.Tags);
        Candidate.TargetingPolicy = Bound.Action.Targeting;
        Candidate.MaximumResolvedTargets = 1;
        Candidate.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
        Candidate.ActionPointCost = 0; // Use has no authored PA field yet.
        Candidate.ResourceCosts.ManaCost = Bound.Action.ManaCost;
        Candidate.ResourceCosts.SourceItemQuantityCost = 1;
        Candidate.CooldownRounds = Bound.Action.CooldownRounds;
        Candidate.PresentationProfileId = Bound.PresentationId;

        FGridCombatStatusRemovalProfile& Removal = Candidate.StatusRemovals.AddDefaulted_GetRef();
        Removal.EffectIds.Add(Bound.Action.Effects[0].StatusId);
        Removal.MaximumRemovals = 1;
        // The generic availability reader checks the default Any side; the
        // actual QuickItem executor is already restricted to Self/Party.
        Removal.TargetSide = EGridCombatStatusRemovalTargetSide::Any;
        if (!Candidate.IsValid())
        {
            return false;
        }
        Out = MoveTemp(Candidate);
        return true;
    }

    /**
     * AZ: one self-targeted status application in combat only.
     *
     * Magnitude=1 is interpreted exclusively as one initial stack; the
     * authoritative status definition owns duration, potency and stacking
     * behaviour. No asset lookup/fallback to historic item definitions.
     * The standard combat catalogue checks WouldAnyMutate before payment.
     */
    static bool TryProjectSingleStatusApplication(
        const FGridResolvedItem& Item, const FGridResolvedBoundAction& Bound,
        FGridCombatActionDefinition& Out)
    {
        Out = FGridCombatActionDefinition();
        if (!Item.IsValid() || Item.bIsArmament || !Item.AmmoKindId.IsNone() ||
            !Bound.IsValid() || Bound.ItemId != Item.ItemId ||
            Bound.ItemLevel != Item.ItemLevel ||
            Bound.Action.Kind != EGridCanonicalActionKind::Use ||
            (Bound.Action.Targeting != EGridCombatTargetingPolicy::Self &&
             Bound.Action.Targeting != EGridCombatTargetingPolicy::Ally) ||
            Bound.Action.bRangedAttack || Bound.Action.bRequiresLineOfSight ||
            Bound.Action.WeaponDamagePercent != 0 ||
            Bound.Action.SourceItemQuantityCost != 1 ||
            Bound.Action.ManaCost < 0 || Bound.Action.CooldownRounds < 0 ||
            Bound.Action.Effects.Num() != 1 ||
            Bound.Action.Effects[0].Kind != EGridCanonicalEffectKind::ApplyStatus ||
            Bound.Action.Effects[0].Magnitude != 1 ||
            Bound.Action.Effects[0].StatusId.IsNone())
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
        Candidate.SourceTags = GridPuzzleCategoryTags::WithoutReserved(Item.Tags);
        Candidate.TargetingPolicy = Bound.Action.Targeting;
        Candidate.MaximumResolvedTargets = 1;
        Candidate.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
        Candidate.ActionPointCost = 0; // Use has no authored AP cost.
        Candidate.ResourceCosts.ManaCost = Bound.Action.ManaCost;
        Candidate.ResourceCosts.SourceItemQuantityCost = 1;
        Candidate.CooldownRounds = Bound.Action.CooldownRounds;
        Candidate.PresentationProfileId = Bound.PresentationId;

        FGridCombatStatusApplicationProfile& Application =
            Candidate.StatusApplications.AddDefaulted_GetRef();
        Application.StatusEffectId = Bound.Action.Effects[0].StatusId;
        Application.Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;
        Application.ArmorGate = EGridCombatStatusArmorGate::None;
        Application.InitialStackCount = 1;
        Application.DurationOverride = INDEX_NONE;
        Application.PotencyOverride = INDEX_NONE;
        if (!Candidate.IsValid())
        {
            return false;
        }
        Out = MoveTemp(Candidate);
        return true;
    }

    static FGridQuickItemUseReadView Resolve(
        const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
    {
        FGridQuickItemUseReadView Out;
        Out.bRegisteredIdentity = Registry &&
            Registry->ContainsItem(Instance.ItemDefinitionId);
        if (!Out.bRegisteredIdentity || !Instance.IsValid())
        {
            return Out;
        }

        FGridResolvedItem Item;
        if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Item) ||
            Item.AvailableActions.Num() != 1)
        {
            return Out; // Registered invalid N0/N21 must fail closed.
        }

        // Directly consume the authored, lossless combat definition from the
        // one canonical action catalogue. No synthetic PA, target, status,
        // effect, cooldown, icon or source tags, and NO legacy fallback.
        FGridCombatActionDefinition Detailed;
        if (Registry->ResolveDetailedNonArmAction(Instance.ItemDefinitionId,
                Instance.ItemLevel, Item.AvailableActions[0].ActionId, Detailed))
        {
            if (Detailed.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem ||
                Detailed.ActionId != Item.AvailableActions[0].ActionId ||
                Detailed.ResourceCosts.SourceItemQuantityCost < 1)
            {
                return Out;
            }
            Out.Action = MoveTemp(Detailed);
            Out.bResolved = true;
            Out.bExplorationCompatible =
                Out.Action.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
                Out.Action.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
                Out.Action.EffectProfile.IsValid() &&
                Out.Action.StatusRemovals.IsEmpty() &&
                Out.Action.StatusApplications.IsEmpty() &&
                Out.Action.ResourceCosts.SourceItemQuantityCost == 1 &&
                Out.Action.ResourceCosts.ManaCost == 0 &&
                Out.Action.CooldownRounds == 0;
            return Out;
        }
        int32 SupportedCount = 0;
        for (const FGridItemActionBinding& Binding : Item.AvailableActions)
        {
            FGridResolvedBoundAction Bound;
            FGridCombatActionDefinition Candidate;
            if (!Registry->ResolveBoundAction(Instance.ItemDefinitionId,
                    Instance.ItemLevel, Binding.ActionId, Bound) ||
                (!TryProjectSingleHeal(Item, Bound, Candidate) &&
                    !TryProjectSingleStatusRemoval(Item, Bound, Candidate) &&
                    !TryProjectSingleStatusApplication(Item, Bound, Candidate)))
            {
                continue;
            }
            ++SupportedCount;
            if (SupportedCount > 1)
            {
                // Never select an arbitrary action for an item hotbar binding.
                return Out;
            }
            Out.Action = MoveTemp(Candidate);
        }
        if (SupportedCount != 1)
        {
            return Out;
        }
        Out.bResolved = true;
        // Exploration only executes direct self-healing. Explicit Ally
        // targeting and all status mutations belong to combat executors.
        Out.bExplorationCompatible =
            Out.Action.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
            Out.Action.EffectProfile.IsValid() &&
            Out.Action.StatusRemovals.IsEmpty() &&
            Out.Action.StatusApplications.IsEmpty() &&
            Out.Action.ResourceCosts.ManaCost == 0 &&
            Out.Action.CooldownRounds == 0;
        return Out;
    }
};
