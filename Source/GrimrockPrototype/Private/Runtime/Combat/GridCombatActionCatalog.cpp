#include "Runtime/Combat/GridCombatActionCatalog.h"

#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/Combat/GridQuickItemResolver.h"
#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"

#include "RPG/RPGClassProgressionTransactionService.h"

#define LOCTEXT_NAMESPACE "GridCombatActionCatalog"

namespace
{
	bool IsClassActionSource(EGridCombatActionSourcePolicy SourcePolicy)
	{
		return SourcePolicy == EGridCombatActionSourcePolicy::Ability || SourcePolicy == EGridCombatActionSourcePolicy::Spell;
	}

	bool IsSupportedAttackTargeting(EGridCombatTargetingPolicy TargetingPolicy)
	{
		return TargetingPolicy == EGridCombatTargetingPolicy::FirstAxialTarget || TargetingPolicy == EGridCombatTargetingPolicy::Cell ||
			TargetingPolicy == EGridCombatTargetingPolicy::Area;
	}

	bool HasMatchingEquippedWeaponSource(
		const FGridCombatActionCatalogContext& Context, const FGridCombatWeaponAttackProfile& WeaponProfile)
	{
		if (!WeaponProfile.bUseEquippedWeapon || WeaponProfile.bAllowUnarmed)
		{
			return true;
		}
		for (int32 Index = 0; Index < Context.EquippedOffensiveSourceTagSets.Num(); ++Index)
		{
			if (!WeaponProfile.MatchesItemTags(Context.EquippedOffensiveSourceTagSets[Index]))
			{
				continue;
			}
			if (!WeaponProfile.AllowedPhysicalSubtypes.IsEmpty())
			{
				if (!Context.EquippedOffensivePhysicalSubtypes.IsValidIndex(Index) ||
					!WeaponProfile.AllowedPhysicalSubtypes.Contains(Context.EquippedOffensivePhysicalSubtypes[Index]))
				{
					continue;
				}
			}
			if (WeaponProfile.bRequireRangedWeapon &&
				(!Context.EquippedOffensiveRangeCells.IsValidIndex(Index) || Context.EquippedOffensiveRangeCells[Index] <= 1))
			{
				continue;
			}
			return true;
		}
		return false;
	}

	bool IsUI0143e2SpellbookProjection(const FGridCombatActionContribution& Contribution)
	{
		const FGridCombatActionDefinition& Definition = Contribution.Definition;
		const bool bSupportedTargeting = Definition.TargetingPolicy == EGridCombatTargetingPolicy::Self ||
			Definition.TargetingPolicy == EGridCombatTargetingPolicy::Ally || Definition.TargetingPolicy == EGridCombatTargetingPolicy::FirstAxialTarget;
		return Definition.SourcePolicy == EGridCombatActionSourcePolicy::Spell && !Definition.ActionId.IsNone() &&
			Contribution.SourceDefinitionId == Definition.ActionId && Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
			bSupportedTargeting;
	}

	void CollectMON2083MissingRequirements(
		const FGridCombatActionCatalogContext& Context, const FGridCombatActionDefinition& Definition, TArray<FName>& OutMissingRequirements)
	{
		OutMissingRequirements.Reset();
		for (const FName Requirement : Definition.Requirements)
		{
			if (!Context.SatisfiedRequirements.Contains(Requirement))
			{
				OutMissingRequirements.AddUnique(Requirement);
			}
		}
		OutMissingRequirements.Sort(
			[](const FName Left, const FName Right)
			{
				return Left.ToString().Compare(Right.ToString(), ESearchCase::CaseSensitive) < 0;
			});
	}

	EGridCombatActionAvailabilityReason EvaluateMON126Availability(
		const FGridCombatActionCatalogContext& Context, const FGridCombatActionContribution& Contribution)
	{
		const FGridCombatActionDefinition& Definition = Contribution.Definition;
		if (Context.CharacterIndex == INDEX_NONE || !Context.CharacterId.IsValid())
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
			return EGridCombatActionAvailabilityReason::InsufficientActionPoints;
		}
		if (Context.CurrentMana < Definition.ResourceCosts.ManaCost)
		{
			return EGridCombatActionAvailabilityReason::InsufficientMana;
		}
		if (Definition.ResourceCosts.SourceItemQuantityCost > 0 && Contribution.AvailableSourceQuantity < Definition.ResourceCosts.SourceItemQuantityCost)
		{
			return EGridCombatActionAvailabilityReason::InsufficientSourceItems;
		}
		for (const FName Requirement : Definition.Requirements)
		{
			if (!Context.SatisfiedRequirements.Contains(Requirement))
			{
				return EGridCombatActionAvailabilityReason::MissingRequirement;
			}
		}
		if (const int32* RemainingCooldown = Context.RemainingCooldownRounds.Find(Definition.ActionId))
		{
			if (*RemainingCooldown > 0)
			{
				return EGridCombatActionAvailabilityReason::CooldownActive;
			}
		}
		if (Definition.WeaponAttackProfile.bUseEquippedWeapon &&
			!HasMatchingEquippedWeaponSource(Context, Definition.WeaponAttackProfile))
		{
			return EGridCombatActionAvailabilityReason::RequiredOffensiveEquipmentUnavailable;
		}

		if (Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
		{
			const bool bSupportedQuickItemProfile =
				(Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack && IsSupportedAttackTargeting(Definition.TargetingPolicy)) ||
				(Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect && Definition.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
					(Definition.EffectProfile.IsValid() || Definition.QuickItemScaling.RestoreHealthSkillRankScale > 0 ||
						Definition.QuickItemScaling.RestoreManaSkillRankScale > 0 || !Definition.StatusApplications.IsEmpty() ||
						!Definition.StatusRemovals.IsEmpty() || !Definition.ArmorEffects.IsEmpty())) ||
				(Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
					(Definition.TargetingPolicy == EGridCombatTargetingPolicy::Cell || Definition.TargetingPolicy == EGridCombatTargetingPolicy::Area) &&
					(!Definition.SurfaceEffects.IsEmpty() || !Definition.SurfaceConversions.IsEmpty() ||
						Definition.SurfaceInteraction != EGridCombatSurfaceInteraction::None)) ||
				(Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
					(Definition.TargetingPolicy == EGridCombatTargetingPolicy::Ally ||
						Definition.TargetingPolicy == EGridCombatTargetingPolicy::Party ||
						Definition.TargetingPolicy == EGridCombatTargetingPolicy::FrontRowParty) &&
					(Definition.EffectProfile.IsValid() || Definition.QuickItemScaling.RestoreHealthSkillRankScale > 0 ||
						Definition.QuickItemScaling.RestoreManaSkillRankScale > 0 || !Definition.StatusApplications.IsEmpty() ||
						!Definition.StatusRemovals.IsEmpty() || !Definition.ArmorEffects.IsEmpty())) ||
				(Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
					Definition.TargetingPolicy == EGridCombatTargetingPolicy::Hostile &&
					(!Definition.StatusApplications.IsEmpty() || !Definition.StatusRemovals.IsEmpty() || !Definition.ArmorEffects.IsEmpty()));
			if (!Context.bEnableQuickItemExecutors || !bSupportedQuickItemProfile)
			{
				return EGridCombatActionAvailabilityReason::ExecutionNotImplemented;
			}

			if (Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect)
			{
				FGridResolvedCombatModifiers QuickItemModifiers;
				FGridCombatModifierResolver::Resolve(Context.CombatModifiers,
					FGridCombatModifierResolver::MakeActionContext(Definition, Contribution.SourceDefinitionId), QuickItemModifiers);
				FGridCombatActionEffectProfile EffectiveEffect;
				FGridQuickItemResolver::ResolveEffectProfile(Definition, Context.SkillRanks, QuickItemModifiers, EffectiveEffect);
				const int32 HealthAfter =
					FMath::Clamp(Context.CurrentHealth + EffectiveEffect.RestoreHealth, 0, FMath::Max(0, Context.MaximumHealth));
				const int32 ManaAfter = FMath::Clamp(
					Context.CurrentMana - Definition.ResourceCosts.ManaCost + EffectiveEffect.RestoreMana, 0, FMath::Max(0, Context.MaximumMana));
				FGridAttackTargetStats SelfTarget;
				SelfTarget.CurrentHealth = Context.CurrentHealth;
				SelfTarget.PhysicalArmor = Context.CurrentPhysicalArmor;
				SelfTarget.MagicalArmor = Context.CurrentMagicalArmor;
				const bool bStatusWouldMutate = FGridCombatStatusApplicationResolver::WouldAnyMutate(
					Definition.StatusApplications, Context.CharacterId, SelfTarget, nullptr, Context.CurrentStatusEffects);
				TArray<FName> StatusRemovalIds;
				FGridCombatTargetingResolver::CollectStatusRemovalIds(Context.CurrentStatusEffects, Definition.StatusRemovals, StatusRemovalIds);
				const bool bStatusRemovalWouldMutate = !StatusRemovalIds.IsEmpty();
				const FGridResolvedCombatModifiers& ArmorModifiers = QuickItemModifiers;
				FGridCombatArmorPoolSnapshot ArmorSnapshot;
				ArmorSnapshot.CurrentPhysicalArmor = Context.CurrentPhysicalArmor;
				ArmorSnapshot.CurrentMagicalArmor = Context.CurrentMagicalArmor;
				ArmorSnapshot.ReferencePhysicalArmor = Context.ReferencePhysicalArmor;
				ArmorSnapshot.ReferenceMagicalArmor = Context.ReferenceMagicalArmor;
				FGridCombatArmorEffectResolver::ApplyReferenceModifiers(ArmorSnapshot, ArmorModifiers);
				const bool bArmorWouldMutate =
					FGridCombatArmorEffectResolver::WouldAnyRestore(Definition.ArmorEffects, ArmorSnapshot, ArmorModifiers, &Context.ArmorEffectSource);
				int32 MovementMobilityCost = 0;
				bool bHasSupportedMovement = false;
				for (const FGridCombatMovementEffectProfile& Movement : Definition.MovementEffects)
				{
					if (Movement.Subject == EGridCombatMovementSubject::PartyGroup && Movement.DistanceCells == 1)
					{
						bHasSupportedMovement = true;
						MovementMobilityCost += Movement.MobilityActionPointCost;
					}
				}
				if (!Definition.MovementEffects.IsEmpty() && !bHasSupportedMovement)
				{
					return EGridCombatActionAvailabilityReason::ExecutionNotImplemented;
				}
				if (bHasSupportedMovement && Context.RemainingMobilityActionPoints < MovementMobilityCost)
				{
					return EGridCombatActionAvailabilityReason::InsufficientMobilityActionPoints;
				}
				if (HealthAfter <= Context.CurrentHealth && ManaAfter <= Context.CurrentMana && !bStatusWouldMutate &&
					!bStatusRemovalWouldMutate && !bArmorWouldMutate && !bHasSupportedMovement)
				{
					return EGridCombatActionAvailabilityReason::NoApplicableEffect;
				}
			}
		}
		else if (IsUI0143e2SpellbookProjection(Contribution))
		{
			// UI01.4.3e.2 has a dedicated Spellbook executor in the TurnManager.
			// The existing non-item executor gate is already enabled by the
			// runtime catalogue context; do not route these projections through
			// the legacy generic class-action shape validation below.
			if (!Context.bEnableClassActionExecutors)
			{
				return EGridCombatActionAvailabilityReason::ExecutionNotImplemented;
			}
		}
		else if (IsClassActionSource(Definition.SourcePolicy))
		{
			const bool bSupportedAttack =
				Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack && IsSupportedAttackTargeting(Definition.TargetingPolicy);
			const bool bSupportedSelfEffect = Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
				Definition.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
				(Definition.EffectProfile.IsValid() || !Definition.StatusApplications.IsEmpty() || !Definition.StatusRemovals.IsEmpty() ||
					!Definition.ArmorEffects.IsEmpty() || !Definition.MovementEffects.IsEmpty());
			const bool bSupportedCellEffect = Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
				(Definition.TargetingPolicy == EGridCombatTargetingPolicy::Cell || Definition.TargetingPolicy == EGridCombatTargetingPolicy::Area) &&
				(!Definition.SurfaceEffects.IsEmpty() || !Definition.SurfaceConversions.IsEmpty() ||
					Definition.SurfaceInteraction != EGridCombatSurfaceInteraction::None ||
					Definition.TrapEffect.bPlaceTrap || Definition.bRelocatePartyToTargetCell);
			const bool bSupportedPartyEffect = Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
				(Definition.TargetingPolicy == EGridCombatTargetingPolicy::Ally ||
					Definition.TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile ||
					Definition.TargetingPolicy == EGridCombatTargetingPolicy::Party ||
					Definition.TargetingPolicy == EGridCombatTargetingPolicy::FrontRowParty) &&
				(Definition.EffectProfile.IsValid() || !Definition.StatusApplications.IsEmpty() || !Definition.StatusRemovals.IsEmpty() ||
					!Definition.ArmorEffects.IsEmpty());
			const bool bSupportedHostileEffect = Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
				(Definition.TargetingPolicy == EGridCombatTargetingPolicy::Hostile ||
					Definition.TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile) &&
				(!Definition.StatusApplications.IsEmpty() || !Definition.StatusRemovals.IsEmpty() || !Definition.ArmorEffects.IsEmpty());
			if (!Context.bEnableClassActionExecutors ||
				(!bSupportedAttack && !bSupportedSelfEffect && !bSupportedCellEffect && !bSupportedPartyEffect && !bSupportedHostileEffect))
			{
				return EGridCombatActionAvailabilityReason::ExecutionNotImplemented;
			}

			if (bSupportedSelfEffect)
			{
				const int32 HealthAfter = FMath::Clamp(
					Context.CurrentHealth + Definition.EffectProfile.ResolveHealthRestore(Context.MaximumHealth), 0, FMath::Max(0, Context.MaximumHealth));
				const int32 ManaAfter = FMath::Clamp(
					Context.CurrentMana - Definition.ResourceCosts.ManaCost + Definition.EffectProfile.ResolveManaRestore(Context.MaximumMana),
					0, FMath::Max(0, Context.MaximumMana));
				FGridAttackTargetStats SelfTarget;
				SelfTarget.CurrentHealth = Context.CurrentHealth;
				SelfTarget.PhysicalArmor = Context.CurrentPhysicalArmor;
				SelfTarget.MagicalArmor = Context.CurrentMagicalArmor;
				const bool bStatusWouldMutate = FGridCombatStatusApplicationResolver::WouldAnyMutate(
					Definition.StatusApplications, Context.CharacterId, SelfTarget, nullptr, Context.CurrentStatusEffects);
				TArray<FName> StatusRemovalIds;
				FGridCombatTargetingResolver::CollectStatusRemovalIds(Context.CurrentStatusEffects, Definition.StatusRemovals, StatusRemovalIds);
				const bool bStatusRemovalWouldMutate = !StatusRemovalIds.IsEmpty();
				FGridResolvedCombatModifiers ArmorModifiers;
				FGridCombatModifierResolver::Resolve(Context.CombatModifiers,
					FGridCombatModifierResolver::MakeActionContext(Definition, Contribution.SourceDefinitionId), ArmorModifiers);
				FGridCombatArmorPoolSnapshot ArmorSnapshot;
				ArmorSnapshot.CurrentPhysicalArmor = Context.CurrentPhysicalArmor;
				ArmorSnapshot.CurrentMagicalArmor = Context.CurrentMagicalArmor;
				ArmorSnapshot.ReferencePhysicalArmor = Context.ReferencePhysicalArmor;
				ArmorSnapshot.ReferenceMagicalArmor = Context.ReferenceMagicalArmor;
				FGridCombatArmorEffectResolver::ApplyReferenceModifiers(ArmorSnapshot, ArmorModifiers);
				const bool bArmorWouldMutate =
					FGridCombatArmorEffectResolver::WouldAnyRestore(Definition.ArmorEffects, ArmorSnapshot, ArmorModifiers, &Context.ArmorEffectSource);
				int32 MovementMobilityCost = 0;
				bool bHasSupportedMovement = false;
				for (const FGridCombatMovementEffectProfile& Movement : Definition.MovementEffects)
				{
					if (Movement.Subject == EGridCombatMovementSubject::PartyGroup && Movement.DistanceCells == 1)
					{
						bHasSupportedMovement = true;
						MovementMobilityCost += Movement.MobilityActionPointCost;
					}
				}
				if (bHasSupportedMovement && Context.RemainingMobilityActionPoints < MovementMobilityCost)
				{
					return EGridCombatActionAvailabilityReason::InsufficientMobilityActionPoints;
				}
				if (HealthAfter <= Context.CurrentHealth && ManaAfter <= Context.CurrentMana && !bStatusWouldMutate &&
					!bStatusRemovalWouldMutate && !bArmorWouldMutate && !bHasSupportedMovement)
				{
					return EGridCombatActionAvailabilityReason::NoApplicableEffect;
				}
			}
		}
		else if (Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
			(Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
				Definition.TargetingPolicy != EGridCombatTargetingPolicy::FirstAxialTarget))
		{
			return EGridCombatActionAvailabilityReason::ExecutionNotImplemented;
		}
		else if (Definition.ResolutionProfile != EGridCombatActionResolutionProfile::Attack || !IsSupportedAttackTargeting(Definition.TargetingPolicy))
		{
			return EGridCombatActionAvailabilityReason::ExecutionNotImplemented;
		}
		return EGridCombatActionAvailabilityReason::None;
	}
}

bool FGridCombatActionCatalog::ApplyOwnerRequirementVariant(
	FGridCombatActionDefinition& Definition, const TSet<FName>& SatisfiedRequirements)
{
	if (Definition.OwnerVariants.IsEmpty())
	{
		return true;
	}

	const FGridCombatActionOwnerVariantProfile* MatchingVariant = nullptr;
	for (const FGridCombatActionOwnerVariantProfile& Variant : Definition.OwnerVariants)
	{
		const bool bMatches = Variant.RequiredOwnerRequirementIds.ContainsByPredicate(
			[&SatisfiedRequirements](const FName RequirementId)
			{
				return !SatisfiedRequirements.Contains(RequirementId);
			}) == false;
		if (!bMatches)
		{
			continue;
		}
		if (MatchingVariant)
		{
			return false;
		}
		MatchingVariant = &Variant;
	}

	if (!MatchingVariant)
	{
		return false;
	}

	for (const FName SourceTag : MatchingVariant->AddedSourceTags)
	{
		Definition.SourceTags.AddUnique(SourceTag);
	}
	if (MatchingVariant->bOverrideDamageDescriptor)
	{
		Definition.OffensiveProfile.AttackDefinition.DamageType = MatchingVariant->OverrideDamageType;
		Definition.OffensiveProfile.AttackDefinition.PhysicalSubtype =
			MatchingVariant->OverrideDamageType == EGridDamageType::Physical
			? MatchingVariant->OverridePhysicalSubtype
			: EGridPhysicalDamageSubtype::None;
	}
	Definition.StatusApplications.Append(MatchingVariant->StatusApplications);
	Definition.SurfaceEffects.Append(MatchingVariant->SurfaceEffects);
	Definition.SurfaceConversions.Append(MatchingVariant->SurfaceConversions);
	Definition.OwnerVariants.Reset();
	return Definition.IsValid();
}

void FGridCombatActionCatalog::Build(
	const FGridCombatActionCatalogContext& Context, const TArray<FGridCombatActionContribution>& Contributions, TArray<FGridAvailableCombatAction>& OutActions)
{
	FGridCombatActionCatalogContext EffectiveContext = Context;
	FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(Context.CharacterId, EffectiveContext.SatisfiedRequirements);

	OutActions.Reset(Contributions.Num());
	for (const FGridCombatActionContribution& Contribution : Contributions)
	{
		if (!Contribution.IsValid())
		{
			continue;
		}

		FGridCombatActionContribution EffectiveContribution = Contribution;
		if (!ApplyOwnerRequirementVariant(EffectiveContribution.Definition, EffectiveContext.SatisfiedRequirements))
		{
			continue;
		}
		FGridResolvedCombatModifiers ResolvedModifiers;
		FGridCombatModifierResolver::Resolve(EffectiveContext.CombatModifiers,
			FGridCombatModifierResolver::MakeActionContext(EffectiveContribution.Definition, Contribution.SourceDefinitionId), ResolvedModifiers);
		FGridCombatModifierResolver::ApplyToActionDefinitionProjection(EffectiveContribution.Definition, ResolvedModifiers);
		if (!EffectiveContribution.IsValid())
		{
			continue;
		}

		FGridAvailableCombatAction Available;
		Available.Definition = EffectiveContribution.Definition;
		Available.CharacterIndex = EffectiveContext.CharacterIndex;
		Available.CharacterId = EffectiveContext.CharacterId;
		Available.SourceDefinitionId = EffectiveContribution.SourceDefinitionId;
		Available.SourceRuntimeId = EffectiveContribution.SourceRuntimeId;
		Available.SourceEquipmentSlot = EffectiveContribution.SourceEquipmentSlot;
		Available.CurrentActionPointCost = EffectiveContribution.Definition.ActionPointCost;
		Available.CurrentManaCost = EffectiveContribution.Definition.ResourceCosts.ManaCost;
		Available.CurrentSourceItemQuantityCost = EffectiveContribution.Definition.ResourceCosts.SourceItemQuantityCost;
		Available.CurrentSourceItemQuantity = EffectiveContribution.AvailableSourceQuantity;
		CollectMON2083MissingRequirements(EffectiveContext, EffectiveContribution.Definition, Available.MissingRequirements);
		Available.AvailabilityReason = EvaluateMON126Availability(EffectiveContext, EffectiveContribution);
		Available.bEnabled = Available.AvailabilityReason == EGridCombatActionAvailabilityReason::None;
		Available.DisabledReason = Available.bEnabled ? FText::GetEmpty() : GetAvailabilityReasonText(Available.AvailabilityReason);
		OutActions.Add(MoveTemp(Available));
	}
}

FGridCombatActionDefinition FGridCombatActionCatalog::MakeUnarmedAttackDefinition(int32 ActionPointCost)
{
	FGridCombatActionDefinition Definition;
	Definition.ActionId = TEXT("Attack_Unarmed");
	Definition.DisplayName = LOCTEXT("UnarmedName", "À mains nues");
	Definition.Description = LOCTEXT("UnarmedDescription", "Attaque physique effectuée sans arme.");
	Definition.ActionType = EGridCombatActionType::MeleeAttack;
	Definition.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	Definition.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	Definition.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	Definition.ActionPointCost = FMath::Clamp(ActionPointCost, 1, 6);
	Definition.RangeCells = 1;
	Definition.PresentationProfileId = TEXT("Unarmed");
	Definition.OffensiveProfile.AttackId = TEXT("Attack_Unarmed");
	Definition.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
	Definition.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Bludgeoning;
	Definition.OffensiveProfile.AttackDefinition.MinDamage = 1;
	Definition.OffensiveProfile.AttackDefinition.MaxDamage = 3;
	Definition.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::Strength;
	Definition.OffensiveProfile.RangeCells = 1;
	return Definition;
}

FText FGridCombatActionCatalog::GetAvailabilityReasonText(EGridCombatActionAvailabilityReason Reason)
{
	switch (Reason)
	{
		case EGridCombatActionAvailabilityReason::CombatInactive:
			return LOCTEXT("CombatInactive", "Aucun combat n’est actif.");
		case EGridCombatActionAvailabilityReason::InvalidCharacter:
			return LOCTEXT("InvalidCharacter", "Ce personnage est invalide.");
		case EGridCombatActionAvailabilityReason::CharacterDefeated:
			return LOCTEXT("CharacterDefeated", "Ce personnage est vaincu.");
		case EGridCombatActionAvailabilityReason::NotActiveCombatant:
			return LOCTEXT("NotActiveCombatant", "Ce n’est pas le tour de ce personnage.");
		case EGridCombatActionAvailabilityReason::PartyBusy:
			return LOCTEXT("PartyBusy", "Le groupe est occupé.");
		case EGridCombatActionAvailabilityReason::InsufficientActionPoints:
			return LOCTEXT("InsufficientActionPoints", "Ce personnage n’a pas assez de points d’action.");
		case EGridCombatActionAvailabilityReason::InsufficientMobilityActionPoints:
			return LOCTEXT("InsufficientMobilityActionPoints", "Le groupe n’a pas assez de points de mobilité.");
		case EGridCombatActionAvailabilityReason::InsufficientMana:
			return LOCTEXT("InsufficientMana", "Ce personnage n’a pas assez de mana.");
		case EGridCombatActionAvailabilityReason::InsufficientSourceItems:
			return LOCTEXT("InsufficientSourceItems", "La source ne contient pas assez d’unités.");
		case EGridCombatActionAvailabilityReason::MissingRequirement:
			return LOCTEXT("MissingRequirement", "Une condition requise n’est pas satisfaite.");
		case EGridCombatActionAvailabilityReason::RequiredOffensiveEquipmentUnavailable:
			return LOCTEXT("RequiredOffensiveEquipmentUnavailable", "Cette action requiert une arme équipée compatible.");
		case EGridCombatActionAvailabilityReason::CooldownActive:
			return LOCTEXT("CooldownActive", "Cette action est encore en recharge.");
		case EGridCombatActionAvailabilityReason::NoApplicableEffect:
			return LOCTEXT("NoApplicableEffect", "Cette action n’aurait actuellement aucun effet utile.");
		case EGridCombatActionAvailabilityReason::ExecutionNotImplemented:
			return LOCTEXT("ExecutionNotImplemented", "L’exécution de cette action sera ajoutée dans un prochain jalon.");
		case EGridCombatActionAvailabilityReason::None:
		default:
			return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE
