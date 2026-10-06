#include "Runtime/Combat/GridCombatModifierResolver.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatResolver.h"

namespace
{
	int32 SaturatingAdd(int32 Left, int32 Right)
	{
		return static_cast<int32>(
			FMath::Clamp<int64>(static_cast<int64>(Left) + static_cast<int64>(Right), static_cast<int64>(MIN_int32), static_cast<int64>(MAX_int32)));
	}

	template <typename T>
	bool MatchesFilter(const TArray<T>& Filter, const T& Value)
	{
		return Filter.IsEmpty() || Filter.Contains(Value);
	}

	float PercentToMultiplier(int32 PercentModifier)
	{
		return FMath::Max(0.0f, 1.0f + static_cast<float>(PercentModifier) / 100.0f);
	}

	int32 ClampActionPointCost(const FGridCombatActionDefinition& Definition, int32 Cost)
	{
		const int32 MinimumCost = Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack ? 1 : 0;
		return FMath::Clamp(Cost, MinimumCost, 6);
	}

	void AddResistanceSet(FGridDamageResistanceSet& Target, const FGridDamageResistanceSet& Source)
	{
		Target.PhysicalResistance = SaturatingAdd(Target.PhysicalResistance, Source.PhysicalResistance);
		Target.FireResistance = SaturatingAdd(Target.FireResistance, Source.FireResistance);
		Target.IceResistance = SaturatingAdd(Target.IceResistance, Source.IceResistance);
		Target.LightningResistance = SaturatingAdd(Target.LightningResistance, Source.LightningResistance);
		Target.PoisonResistance = SaturatingAdd(Target.PoisonResistance, Source.PoisonResistance);
		Target.HolyResistance = SaturatingAdd(Target.HolyResistance, Source.HolyResistance);
		Target.NecroticResistance = SaturatingAdd(Target.NecroticResistance, Source.NecroticResistance);
		Target.ArcaneResistance = SaturatingAdd(Target.ArcaneResistance, Source.ArcaneResistance);
	}

	bool CollectCharacterOwnerRequirements(const FGridCharacterInventoryState& Character, TSet<FName>& OutRequirements)
	{
		OutRequirements.Reset();
		const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!IsValid(ClassDefinition))
		{
			return Character.SelectedClassProgressionChoiceIds.IsEmpty();
		}

		TSet<FName> SelectedChoiceIds;
		for (const FName ChoiceId : Character.SelectedClassProgressionChoiceIds)
		{
			if (ChoiceId.IsNone() || SelectedChoiceIds.Contains(ChoiceId))
			{
				return false;
			}
			SelectedChoiceIds.Add(ChoiceId);
		}

		return FRPGClassProgressionService::CollectSatisfiedRequirements(
			ClassDefinition, Character.Level, SelectedChoiceIds, OutRequirements);
	}

	bool AreOwnerRequirementsSatisfied(const TArray<FName>& RequiredIds, const TSet<FName>& OwnerRequirements)
	{
		for (const FName RequirementId : RequiredIds)
		{
			if (!OwnerRequirements.Contains(RequirementId))
			{
				return false;
			}
		}
		return true;
	}

	bool CollectChoiceModifiersWithRequirements(const FGridCharacterInventoryState& Character, const TSet<FName>& OwnerRequirements,
		TArray<FGridCombatModifierProfile>& OutProfiles)
	{
		OutProfiles.Reset();
		const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!IsValid(ClassDefinition))
		{
			return Character.SelectedClassProgressionChoiceIds.IsEmpty();
		}

		for (const FName ChoiceId : Character.SelectedClassProgressionChoiceIds)
		{
			const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId);
			if (!Choice)
			{
				OutProfiles.Reset();
				return false;
			}

			for (const FGridCombatModifierProfile& AuthoredProfile : Choice->CombatModifiers)
			{
				if (!AreOwnerRequirementsSatisfied(AuthoredProfile.RequiredOwnerRequirementIds, OwnerRequirements))
				{
					continue;
				}
				FGridCombatModifierProfile RuntimeProfile = AuthoredProfile;
				RuntimeProfile.RequiredOwnerRequirementIds.Reset();
				OutProfiles.Add(MoveTemp(RuntimeProfile));
			}
		}
		return true;
	}
}

FGridCombatModifierContext FGridCombatModifierResolver::MakeActionContext(const FGridCombatActionDefinition& Definition, FName SourceDefinitionId)
{
	FGridCombatModifierContext Context;
	Context.ActionId = Definition.ActionId;
	Context.SourceDefinitionId = SourceDefinitionId;
	Context.SourceTags = Definition.SourceTags;
	Context.SourcePolicy = Definition.SourcePolicy;
	Context.ActionType = Definition.ActionType;
	Context.TargetingPolicy = Definition.TargetingPolicy;
	if (Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack && Definition.OffensiveProfile.IsValid())
	{
		Context.bHasDamageDescriptor = true;
		Context.DamageType = Definition.OffensiveProfile.AttackDefinition.DamageType;
		Context.PhysicalSubtype = Definition.OffensiveProfile.AttackDefinition.PhysicalSubtype;
	}
	return Context;
}

FGridCombatModifierContext FGridCombatModifierResolver::MakeResolvedActionAttackContext(const FGridCombatActionDefinition& Definition,
	FName SourceDefinitionId, const FGridOffensiveEquipmentProfile& ResolvedOffensiveProfile, const TArray<FName>& ResolvedItemTags)
{
	FGridCombatModifierContext Context = MakeActionContext(Definition, SourceDefinitionId);
	for (const FName ItemTag : ResolvedItemTags)
	{
		if (!ItemTag.IsNone())
		{
			Context.SourceTags.AddUnique(ItemTag);
		}
	}
	if (ResolvedOffensiveProfile.IsValid())
	{
		Context.bHasDamageDescriptor = true;
		Context.DamageType = ResolvedOffensiveProfile.AttackDefinition.DamageType;
		Context.PhysicalSubtype = ResolvedOffensiveProfile.AttackDefinition.DamageType == EGridDamageType::Physical
			? ResolvedOffensiveProfile.AttackDefinition.PhysicalSubtype
			: EGridPhysicalDamageSubtype::None;
	}
	return Context;
}

FGridCombatModifierContext FGridCombatModifierResolver::MakeAttackContext(FName ActionId, FName SourceDefinitionId,
	EGridCombatActionSourcePolicy SourcePolicy, EGridCombatActionType ActionType, EGridDamageType DamageType,
	EGridPhysicalDamageSubtype PhysicalSubtype, const TArray<FName>& SourceTags)
{
	FGridCombatModifierContext Context;
	Context.ActionId = ActionId;
	Context.SourceDefinitionId = SourceDefinitionId;
	Context.SourcePolicy = SourcePolicy;
	Context.ActionType = ActionType;
	Context.SourceTags = SourceTags;
	Context.bHasDamageDescriptor = true;
	Context.DamageType = DamageType;
	Context.PhysicalSubtype = DamageType == EGridDamageType::Physical ? PhysicalSubtype : EGridPhysicalDamageSubtype::None;
	return Context;
}

void FGridCombatModifierResolver::AddTargetContext(FGridCombatModifierContext& Context, EGridCombatTargetingPolicy TargetingPolicy,
	bool bRearArc, bool bTargetHasActedThisRound, bool bTargetHasPhysicalControl)
{
	Context.TargetingPolicy = TargetingPolicy;
	Context.TargetConditions.Reset();
	if (bRearArc) Context.TargetConditions.Add(EGridCombatTargetCondition::RearArc);
	Context.TargetConditions.Add(bTargetHasActedThisRound ? EGridCombatTargetCondition::HasActedThisRound
														 : EGridCombatTargetCondition::HasNotActedThisRound);
	if (bTargetHasPhysicalControl) Context.TargetConditions.Add(EGridCombatTargetCondition::PhysicalControl);
}

void FGridCombatModifierResolver::AddTargetStatusContext(FGridCombatModifierContext& Context,
	const FGridStatusEffectCollection& StatusEffects, const FGuid& ActingSourceId, FName TargetMonsterCategoryId,
	const TArray<FName>& TargetSemanticTags)
{
	Context.TargetStatusEffectIds.Reset();
	Context.TargetStatusEffectIdsFromSource.Reset();
	Context.TargetEnvironmentTags.Reset();
	Context.TargetMonsterCategoryId = TargetMonsterCategoryId;
	Context.TargetSemanticTags = TargetSemanticTags;
	if (!TargetMonsterCategoryId.IsNone())
	{
		Context.TargetSemanticTags.AddUnique(TargetMonsterCategoryId);
	}
	for (const FGridStatusEffectRuntimeState& State : StatusEffects.ActiveEffects)
	{
		if (!State.IsValid())
		{
			continue;
		}
		Context.TargetStatusEffectIds.AddUnique(State.EffectId);
		Context.TargetEnvironmentTags.AddUnique(State.EffectId);
		if (IsValid(State.DefinitionAsset))
		{
			for (const FName Tag : State.DefinitionAsset->StatusTags)
			{
				if (!Tag.IsNone())
				{
					Context.TargetEnvironmentTags.AddUnique(Tag);
				}
			}
		}
		if (ActingSourceId.IsValid() && State.SourceId == ActingSourceId)
		{
			Context.TargetStatusEffectIdsFromSource.AddUnique(State.EffectId);
		}
	}
}

void FGridCombatModifierResolver::AddTargetSurfaceContext(FGridCombatModifierContext& Context, EGridCombatSurfaceType SurfaceType)
{
	const TCHAR* Suffix = nullptr;
	switch (SurfaceType)
	{
		case EGridCombatSurfaceType::Fire: Suffix = TEXT("Fire"); break;
		case EGridCombatSurfaceType::Water: Suffix = TEXT("Water"); break;
		case EGridCombatSurfaceType::Ice: Suffix = TEXT("Ice"); break;
		case EGridCombatSurfaceType::Poison: Suffix = TEXT("Poison"); break;
		case EGridCombatSurfaceType::Oil: Suffix = TEXT("Oil"); break;
		case EGridCombatSurfaceType::ElectrifiedWater: Suffix = TEXT("ElectrifiedWater"); break;
		case EGridCombatSurfaceType::Blood: Suffix = TEXT("Blood"); break;
		case EGridCombatSurfaceType::Smoke: Suffix = TEXT("Smoke"); break;
		case EGridCombatSurfaceType::PoisonCloud: Suffix = TEXT("PoisonCloud"); break;
		case EGridCombatSurfaceType::None:
		default:
			return;
	}
	Context.TargetEnvironmentTags.AddUnique(FName(*FString::Printf(TEXT("Surface.%s"), Suffix)));
}

bool FGridCombatModifierResolver::Matches(const FGridCombatModifierProfile& Profile, const FGridCombatModifierContext& Context)
{
	// Progression-owner requirements are consumed by character projection collectors.
	if (!Profile.RequiredOwnerRequirementIds.IsEmpty())
	{
		return false;
	}
	if (!Profile.ActionIds.IsEmpty() && !Profile.ActionIds.Contains(Context.ActionId))
	{
		return false;
	}
	if (!Profile.SourceDefinitionIds.IsEmpty() && !Profile.SourceDefinitionIds.Contains(Context.SourceDefinitionId))
	{
		return false;
	}
	for (const FName RequiredTag : Profile.RequiredSourceTags)
	{
		if (!Context.SourceTags.Contains(RequiredTag))
		{
			return false;
		}
	}
	if (!MatchesFilter(Profile.SourcePolicies, Context.SourcePolicy) || !MatchesFilter(Profile.ActionTypes, Context.ActionType) ||
		!MatchesFilter(Profile.TargetingPolicies, Context.TargetingPolicy))
	{
		return false;
	}
	if (Profile.bExcludeAreaActions && Context.TargetingPolicy == EGridCombatTargetingPolicy::Area)
	{
		return false;
	}
	if (Profile.bRequirePartyStationarySincePreviousActivation && !Context.bPartyStationarySincePreviousActivation)
	{
		return false;
	}
	if (!Profile.AllowedTargetMonsterCategoryIds.IsEmpty() &&
		!Profile.AllowedTargetMonsterCategoryIds.Contains(Context.TargetMonsterCategoryId))
	{
		return false;
	}
	if (!Profile.AnyTargetSemanticTags.IsEmpty() &&
		!Profile.AnyTargetSemanticTags.ContainsByPredicate(
			[&Context](const FName Tag)
			{
				return Context.TargetSemanticTags.Contains(Tag);
			}))
	{
		return false;
	}
	if (!Profile.AnyTargetEnvironmentTags.IsEmpty() &&
		!Profile.AnyTargetEnvironmentTags.ContainsByPredicate(
			[&Context](const FName Tag)
			{
				return Context.TargetEnvironmentTags.Contains(Tag);
			}))
	{
		return false;
	}
	const TArray<FName>& RequiredTargetStatusSet =
		Profile.bRequiredTargetStatusesFromOwner ? Context.TargetStatusEffectIdsFromSource : Context.TargetStatusEffectIds;
	for (const FName EffectId : Profile.RequiredTargetStatusEffectIds)
	{
		if (!RequiredTargetStatusSet.Contains(EffectId))
		{
			return false;
		}
	}
	for (const EGridCombatTargetCondition Condition : Profile.RequiredTargetConditions)
	{
		if (!Context.HasTargetCondition(Condition))
		{
			return false;
		}
	}
	if (!Profile.AnyTargetConditions.IsEmpty() &&
		!Profile.AnyTargetConditions.ContainsByPredicate([&Context](EGridCombatTargetCondition Condition)
		{
			return Context.HasTargetCondition(Condition);
		}))
	{
		return false;
	}
	if (!Profile.DamageTypes.IsEmpty())
	{
		if (!Context.bHasDamageDescriptor || !Profile.DamageTypes.Contains(Context.DamageType))
		{
			return false;
		}
	}
	if (!Profile.PhysicalSubtypes.IsEmpty())
	{
		if (!Context.bHasDamageDescriptor || Context.DamageType != EGridDamageType::Physical ||
			!Profile.PhysicalSubtypes.Contains(Context.PhysicalSubtype))
		{
			return false;
		}
	}
	return true;
}

void FGridCombatModifierResolver::Resolve(
	const TArray<FGridCombatModifierProfile>& Profiles, const FGridCombatModifierContext& Context, FGridResolvedCombatModifiers& OutModifiers)
{
	OutModifiers.Reset();
	for (const FGridCombatModifierProfile& Profile : Profiles)
	{
		if (!Profile.IsValid() || !Matches(Profile, Context))
		{
			continue;
		}

		OutModifiers.AccuracyModifier = SaturatingAdd(OutModifiers.AccuracyModifier, Profile.AccuracyModifier);
		OutModifiers.EvasionModifier = SaturatingAdd(OutModifiers.EvasionModifier, Profile.EvasionModifier);
		OutModifiers.OutgoingDamagePercentModifier =
			SaturatingAdd(OutModifiers.OutgoingDamagePercentModifier, Profile.OutgoingDamagePercentModifier);
		OutModifiers.IncomingDamagePercentModifier =
			SaturatingAdd(OutModifiers.IncomingDamagePercentModifier, Profile.IncomingDamagePercentModifier);
		OutModifiers.CriticalChancePercentModifier =
			SaturatingAdd(OutModifiers.CriticalChancePercentModifier, Profile.CriticalChancePercentModifier);
		OutModifiers.CriticalDamagePercentModifier =
			SaturatingAdd(OutModifiers.CriticalDamagePercentModifier, Profile.CriticalDamagePercentModifier);
		OutModifiers.WeaponDamagePercentModifier =
			SaturatingAdd(OutModifiers.WeaponDamagePercentModifier, Profile.WeaponDamagePercentModifier);
		AddResistanceSet(OutModifiers.ResistanceModifiers, Profile.ResistanceModifiers);
		OutModifiers.ActionPointCostModifier = SaturatingAdd(OutModifiers.ActionPointCostModifier, Profile.ActionPointCostModifier);
		OutModifiers.ManaCostModifier = SaturatingAdd(OutModifiers.ManaCostModifier, Profile.ManaCostModifier);
		OutModifiers.MinimumManaCost = FMath::Max(OutModifiers.MinimumManaCost, Profile.MinimumManaCost);
		OutModifiers.RangeCellsModifier = SaturatingAdd(OutModifiers.RangeCellsModifier, Profile.RangeCellsModifier);
		OutModifiers.PositiveEffectPercentModifier =
			SaturatingAdd(OutModifiers.PositiveEffectPercentModifier, Profile.PositiveEffectPercentModifier);
		OutModifiers.OutgoingHealingPercentModifier =
			SaturatingAdd(OutModifiers.OutgoingHealingPercentModifier, Profile.OutgoingHealingPercentModifier);
		OutModifiers.FriendlyDirectDamagePercentModifier =
			SaturatingAdd(OutModifiers.FriendlyDirectDamagePercentModifier, Profile.FriendlyDirectDamagePercentModifier);
		OutModifiers.SelfDirectDamagePercentModifier =
			SaturatingAdd(OutModifiers.SelfDirectDamagePercentModifier, Profile.SelfDirectDamagePercentModifier);
		OutModifiers.QuickItemSecondaryTargetCount =
			FMath::Max(OutModifiers.QuickItemSecondaryTargetCount, Profile.QuickItemSecondaryTargetCount);
		OutModifiers.QuickItemSecondaryMagnitudePercent =
			FMath::Max(OutModifiers.QuickItemSecondaryMagnitudePercent, Profile.QuickItemSecondaryMagnitudePercent);
		OutModifiers.QuickItemSecondaryDurationPercent =
			FMath::Max(OutModifiers.QuickItemSecondaryDurationPercent, Profile.QuickItemSecondaryDurationPercent);
		OutModifiers.PhysicalArmorReferencePercentModifier =
			SaturatingAdd(OutModifiers.PhysicalArmorReferencePercentModifier, Profile.PhysicalArmorReferencePercentModifier);
		OutModifiers.MagicalArmorReferencePercentModifier =
			SaturatingAdd(OutModifiers.MagicalArmorReferencePercentModifier, Profile.MagicalArmorReferencePercentModifier);
		OutModifiers.PhysicalArmorRestorationPercentModifier =
			SaturatingAdd(OutModifiers.PhysicalArmorRestorationPercentModifier, Profile.PhysicalArmorRestorationPercentModifier);
		OutModifiers.MagicalArmorRestorationPercentModifier =
			SaturatingAdd(OutModifiers.MagicalArmorRestorationPercentModifier, Profile.MagicalArmorRestorationPercentModifier);
		OutModifiers.SurfaceDurationRoundsModifier =
			SaturatingAdd(OutModifiers.SurfaceDurationRoundsModifier, Profile.SurfaceDurationRoundsModifier);
		OutModifiers.SurfacePeriodicDamagePercentModifier =
			SaturatingAdd(OutModifiers.SurfacePeriodicDamagePercentModifier, Profile.SurfacePeriodicDamagePercentModifier);
		OutModifiers.SurfaceReactionDamagePercentModifier =
			SaturatingAdd(OutModifiers.SurfaceReactionDamagePercentModifier, Profile.SurfaceReactionDamagePercentModifier);
		OutModifiers.SurfaceReactionAreaRadiusModifier =
			SaturatingAdd(OutModifiers.SurfaceReactionAreaRadiusModifier, Profile.SurfaceReactionAreaRadiusModifier);
	}
}

bool FGridCombatModifierResolver::CollectCharacterChoiceModifiers(
	const FGridCharacterInventoryState& Character, TArray<FGridCombatModifierProfile>& OutProfiles)
{
	TSet<FName> OwnerRequirements;
	if (!CollectCharacterOwnerRequirements(Character, OwnerRequirements))
	{
		OutProfiles.Reset();
		return false;
	}
	return CollectChoiceModifiersWithRequirements(Character, OwnerRequirements, OutProfiles);
}

bool FGridCombatModifierResolver::CollectStatusModifiers(
	const FGridStatusEffectCollection& StatusEffects, TArray<FGridCombatModifierProfile>& OutProfiles)
{
	return CollectStatusModifiers(StatusEffects, TSet<FName>(), OutProfiles);
}

bool FGridCombatModifierResolver::CollectStatusModifiers(const FGridStatusEffectCollection& StatusEffects,
	const TSet<FName>& OwnerRequirements, TArray<FGridCombatModifierProfile>& OutProfiles)
{
	OutProfiles.Reset();
	for (const FGridStatusEffectRuntimeState& State : StatusEffects.ActiveEffects)
	{
		if (!State.IsValid() || !IsValid(State.DefinitionAsset) || !State.DefinitionAsset->IsValidDefinition())
		{
			OutProfiles.Reset();
			return false;
		}
		for (int32 StackIndex = 0; StackIndex < State.StackCount; ++StackIndex)
		{
			for (const FGridCombatModifierProfile& AuthoredProfile : State.DefinitionAsset->CombatModifiers)
			{
				if (!AreOwnerRequirementsSatisfied(AuthoredProfile.RequiredOwnerRequirementIds, OwnerRequirements))
				{
					continue;
				}
				FGridCombatModifierProfile RuntimeProfile = AuthoredProfile;
				RuntimeProfile.RequiredOwnerRequirementIds.Reset();
				OutProfiles.Add(MoveTemp(RuntimeProfile));
			}
		}
	}
	return true;
}

bool FGridCombatModifierResolver::CollectCharacterModifiers(
	const FGridCharacterInventoryState& Character, TArray<FGridCombatModifierProfile>& OutProfiles)
{
	TSet<FName> OwnerRequirements;
	if (!CollectCharacterOwnerRequirements(Character, OwnerRequirements))
	{
		OutProfiles.Reset();
		return false;
	}

	TArray<FGridCombatModifierProfile> ChoiceProfiles;
	TArray<FGridCombatModifierProfile> StatusProfiles;
	if (!CollectChoiceModifiersWithRequirements(Character, OwnerRequirements, ChoiceProfiles) ||
		!CollectStatusModifiers(Character.StatusEffects, OwnerRequirements, StatusProfiles))
	{
		OutProfiles.Reset();
		return false;
	}
	OutProfiles = MoveTemp(ChoiceProfiles);
	OutProfiles.Append(StatusProfiles);
	return true;
}

void FGridCombatModifierResolver::ApplyToActionDefinitionProjection(
	FGridCombatActionDefinition& Definition, const FGridResolvedCombatModifiers& Modifiers)
{
	Definition.ActionPointCost =
		ClampActionPointCost(Definition, SaturatingAdd(Definition.ActionPointCost, Modifiers.ActionPointCostModifier));
	const int32 AuthoredManaCost = Definition.ResourceCosts.ManaCost;
	const int32 ModifiedManaCost = FMath::Max(0, SaturatingAdd(AuthoredManaCost, Modifiers.ManaCostModifier));
	Definition.ResourceCosts.ManaCost =
		AuthoredManaCost > 0 ? FMath::Max(Modifiers.MinimumManaCost, ModifiedManaCost) : 0;

	if (Definition.RangeCells > 0 || Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack)
	{
		const int32 MinimumRange = Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack ? 1 : 0;
		Definition.RangeCells = FMath::Clamp(SaturatingAdd(Definition.RangeCells, Modifiers.RangeCellsModifier), MinimumRange, 32);
	}
	if (Definition.WeaponAttackProfile.bUseEquippedWeapon && Definition.WeaponAttackProfile.bUseWeaponRange &&
		Modifiers.RangeCellsModifier != 0)
	{
		// Dynamic weapon-range actions ignore Definition.RangeCells during weapon projection,
		// so compose the same runtime delta into their authored weapon-range modifier.
		Definition.WeaponAttackProfile.WeaponRangeModifier = FMath::Clamp(
			SaturatingAdd(Definition.WeaponAttackProfile.WeaponRangeModifier, Modifiers.RangeCellsModifier), -31, 31);
	}
	if (Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack && Definition.OffensiveProfile.IsValid())
	{
		Definition.OffensiveProfile.RangeCells = Definition.RangeCells;
	}
}

void FGridCombatModifierResolver::ApplyOutgoingAttackModifiers(FGridAttackSourceStats& Source, const FGridResolvedCombatModifiers& Modifiers)
{
	Source.Accuracy = SaturatingAdd(Source.Accuracy, Modifiers.AccuracyModifier);
	Source.DamageMultiplier = FMath::Max(0.0f, Source.DamageMultiplier * PercentToMultiplier(Modifiers.OutgoingDamagePercentModifier));
	Source.CriticalChancePercent = FMath::Clamp(SaturatingAdd(Source.CriticalChancePercent, Modifiers.CriticalChancePercentModifier), 0, 100);
	Source.CriticalDamagePercent = FMath::Clamp(SaturatingAdd(Source.CriticalDamagePercent, Modifiers.CriticalDamagePercentModifier), 100, 1000);
	Source.RawDamagePercent = FMath::Clamp(SaturatingAdd(Source.RawDamagePercent, Modifiers.WeaponDamagePercentModifier), 0, 1000);
}

void FGridCombatModifierResolver::ApplyDirectDamageSkillScaling(
	const FGridCombatActionDefinition& Definition, const TArray<FRPGSkillRank>& SkillRanks, FGridAttackSourceStats& InOutSource,
	const FRPGAttributes* Attributes)
{
	if (!Definition.DirectDamageScaling.IsValid() || !Definition.DirectDamageScaling.HasAnyScaling())
	{
		return;
	}

	int64 Bonus = 0;
	if (Attributes && Definition.DirectDamageScaling.AttributeModifierScale > 0)
	{
		int32 AttributeValue = 0;
		switch (Definition.DirectDamageScaling.ScalingAttribute)
		{
			case EGridAttackScalingAttribute::Strength: AttributeValue = Attributes->Strength; break;
			case EGridAttackScalingAttribute::Dexterity: AttributeValue = Attributes->Dexterity; break;
			case EGridAttackScalingAttribute::Constitution: AttributeValue = Attributes->Constitution; break;
			case EGridAttackScalingAttribute::Intelligence: AttributeValue = Attributes->Intelligence; break;
			case EGridAttackScalingAttribute::Wisdom: AttributeValue = Attributes->Wisdom; break;
			case EGridAttackScalingAttribute::Charisma: AttributeValue = Attributes->Charisma; break;
			case EGridAttackScalingAttribute::None:
			default: break;
		}
		Bonus += static_cast<int64>(URPGCharacterRulesLibrary::GetAttributeModifier(AttributeValue)) *
			static_cast<int64>(Definition.DirectDamageScaling.AttributeModifierScale);
	}

	if (Definition.DirectDamageScaling.SkillRankScale > 0)
	{
		const FRPGSkillRank* Rank = SkillRanks.FindByPredicate(
			[&Definition](const FRPGSkillRank& Candidate)
			{
				return Candidate.SkillId == Definition.DirectDamageScaling.ScalingSkillId && Candidate.Rank > 0;
			});
		if (Rank)
		{
			Bonus += static_cast<int64>(Rank->Rank) * static_cast<int64>(Definition.DirectDamageScaling.SkillRankScale);
		}
	}

	InOutSource.DamageBonus = static_cast<int32>(FMath::Clamp<int64>(
		static_cast<int64>(InOutSource.DamageBonus) + Bonus, static_cast<int64>(MIN_int32), static_cast<int64>(MAX_int32)));
}

int32 FGridCombatModifierResolver::ApplyOutgoingHealingModifier(int32 RawHealing, const FGridResolvedCombatModifiers& Modifiers)
{
	if (RawHealing <= 0)
	{
		return 0;
	}
	const int32 PercentModifier = Modifiers.OutgoingHealingPercentModifier;
	const int32 SafePercent = FMath::Max(0, 100 + PercentModifier);
	const int64 Scaled = static_cast<int64>(RawHealing) * static_cast<int64>(SafePercent) / 100;
	int32 Resolved = static_cast<int32>(FMath::Clamp<int64>(Scaled, 0, MAX_int32));
	if (PercentModifier > 0)
	{
		Resolved = FMath::Max(Resolved, RawHealing + 1);
	}
	return Resolved;
}

int32 FGridCombatModifierResolver::ResolveDirectHealthRestore(const FGridCombatActionDefinition& Definition,
	const FGridCombatActionEffectProfile& EffectProfile, const FRPGAttributes& SourceAttributes,
	const TArray<FRPGSkillRank>& SourceSkillRanks, const FGridResolvedCombatModifiers& SourceModifiers,
	int32 TargetCurrentHealth, int32 TargetMaximumHealth)
{
	if (!Definition.HealingScaling.IsValid() || TargetMaximumHealth <= 0)
	{
		return 0;
	}

	int64 RawHealing = EffectProfile.ResolveHealthRestore(TargetMaximumHealth);
	if (Definition.HealingScaling.AttributeModifierScale > 0)
	{
		int32 AttributeValue = 0;
		switch (Definition.HealingScaling.ScalingAttribute)
		{
			case EGridAttackScalingAttribute::Strength: AttributeValue = SourceAttributes.Strength; break;
			case EGridAttackScalingAttribute::Dexterity: AttributeValue = SourceAttributes.Dexterity; break;
			case EGridAttackScalingAttribute::Constitution: AttributeValue = SourceAttributes.Constitution; break;
			case EGridAttackScalingAttribute::Intelligence: AttributeValue = SourceAttributes.Intelligence; break;
			case EGridAttackScalingAttribute::Wisdom: AttributeValue = SourceAttributes.Wisdom; break;
			case EGridAttackScalingAttribute::Charisma: AttributeValue = SourceAttributes.Charisma; break;
			case EGridAttackScalingAttribute::None:
			default: break;
		}
		RawHealing += static_cast<int64>(URPGCharacterRulesLibrary::GetAttributeModifier(AttributeValue)) *
			static_cast<int64>(Definition.HealingScaling.AttributeModifierScale);
	}

	if (Definition.HealingScaling.SkillRankScale > 0)
	{
		const FRPGSkillRank* Rank = SourceSkillRanks.FindByPredicate(
			[&Definition](const FRPGSkillRank& Candidate)
			{
				return Candidate.SkillId == Definition.HealingScaling.ScalingSkillId && Candidate.Rank > 0;
			});
		if (Rank)
		{
			RawHealing += static_cast<int64>(Rank->Rank) * static_cast<int64>(Definition.HealingScaling.SkillRankScale);
		}
	}

	if (Definition.HealingScaling.MinimumTargetHealthPercent > 0)
	{
		const int32 TargetFloor = FGridCombatActionEffectProfile::ResolveMaximumPercentAmount(
			TargetMaximumHealth, Definition.HealingScaling.MinimumTargetHealthPercent);
		RawHealing = FMath::Max<int64>(RawHealing, FMath::Max(0, TargetFloor - TargetCurrentHealth));
	}

	return ApplyOutgoingHealingModifier(
		static_cast<int32>(FMath::Clamp<int64>(RawHealing, 0, MAX_int32)), SourceModifiers);
}

void FGridCombatModifierResolver::ApplyFriendlyDirectDamageModifiers(
	FGridAttackSourceStats& InOutSource, const FGridResolvedCombatModifiers& Modifiers, bool bSelfTarget)
{
	InOutSource.DamageMultiplier = FMath::Max(
		0.0f, InOutSource.DamageMultiplier * PercentToMultiplier(Modifiers.FriendlyDirectDamagePercentModifier));
	if (bSelfTarget)
	{
		InOutSource.DamageMultiplier = FMath::Max(
			0.0f, InOutSource.DamageMultiplier * PercentToMultiplier(Modifiers.SelfDirectDamagePercentModifier));
	}
}

void FGridCombatModifierResolver::ApplyIncomingAttackModifiers(
	FGridAttackTargetStats& Target, EGridDamageType DamageType, const FGridResolvedCombatModifiers& Modifiers)
{
	Target.Evasion = SaturatingAdd(Target.Evasion, Modifiers.EvasionModifier);
	Target.DamageMultiplier = FMath::Max(0.0f, Target.DamageMultiplier * PercentToMultiplier(Modifiers.IncomingDamagePercentModifier));
	Target.ResistancePercent = FMath::Clamp(SaturatingAdd(Target.ResistancePercent,
		FGridCombatResolver::GetResistancePercent(Modifiers.ResistanceModifiers, DamageType)), -100, 100);
}
