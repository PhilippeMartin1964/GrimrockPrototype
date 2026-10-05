#include "Runtime/Combat/GridCombatModifierResolver.h"

#include "RPG/RPGClassAsset.h"
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
}

FGridCombatModifierContext FGridCombatModifierResolver::MakeActionContext(const FGridCombatActionDefinition& Definition, FName SourceDefinitionId)
{
	FGridCombatModifierContext Context;
	Context.ActionId = Definition.ActionId;
	Context.SourceDefinitionId = SourceDefinitionId;
	Context.SourcePolicy = Definition.SourcePolicy;
	Context.ActionType = Definition.ActionType;
	if (Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack && Definition.OffensiveProfile.IsValid())
	{
		Context.bHasDamageDescriptor = true;
		Context.DamageType = Definition.OffensiveProfile.AttackDefinition.DamageType;
		Context.PhysicalSubtype = Definition.OffensiveProfile.AttackDefinition.PhysicalSubtype;
	}
	return Context;
}

FGridCombatModifierContext FGridCombatModifierResolver::MakeAttackContext(FName ActionId, FName SourceDefinitionId,
	EGridCombatActionSourcePolicy SourcePolicy, EGridCombatActionType ActionType, EGridDamageType DamageType, EGridPhysicalDamageSubtype PhysicalSubtype)
{
	FGridCombatModifierContext Context;
	Context.ActionId = ActionId;
	Context.SourceDefinitionId = SourceDefinitionId;
	Context.SourcePolicy = SourcePolicy;
	Context.ActionType = ActionType;
	Context.bHasDamageDescriptor = true;
	Context.DamageType = DamageType;
	Context.PhysicalSubtype = DamageType == EGridDamageType::Physical ? PhysicalSubtype : EGridPhysicalDamageSubtype::None;
	return Context;
}

bool FGridCombatModifierResolver::Matches(const FGridCombatModifierProfile& Profile, const FGridCombatModifierContext& Context)
{
	if (!Profile.ActionIds.IsEmpty() && !Profile.ActionIds.Contains(Context.ActionId))
	{
		return false;
	}
	if (!Profile.SourceDefinitionIds.IsEmpty() && !Profile.SourceDefinitionIds.Contains(Context.SourceDefinitionId))
	{
		return false;
	}
	if (!MatchesFilter(Profile.SourcePolicies, Context.SourcePolicy) || !MatchesFilter(Profile.ActionTypes, Context.ActionType))
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
		AddResistanceSet(OutModifiers.ResistanceModifiers, Profile.ResistanceModifiers);
		OutModifiers.ActionPointCostModifier = SaturatingAdd(OutModifiers.ActionPointCostModifier, Profile.ActionPointCostModifier);
		OutModifiers.ManaCostModifier = SaturatingAdd(OutModifiers.ManaCostModifier, Profile.ManaCostModifier);
		OutModifiers.RangeCellsModifier = SaturatingAdd(OutModifiers.RangeCellsModifier, Profile.RangeCellsModifier);
	}
}

bool FGridCombatModifierResolver::CollectCharacterChoiceModifiers(
	const FGridCharacterInventoryState& Character, TArray<FGridCombatModifierProfile>& OutProfiles)
{
	OutProfiles.Reset();
	const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
	if (!IsValid(ClassDefinition))
	{
		return Character.SelectedClassProgressionChoiceIds.IsEmpty();
	}

	TSet<FName> SeenChoiceIds;
	for (const FName ChoiceId : Character.SelectedClassProgressionChoiceIds)
	{
		if (ChoiceId.IsNone() || SeenChoiceIds.Contains(ChoiceId))
		{
			OutProfiles.Reset();
			return false;
		}
		SeenChoiceIds.Add(ChoiceId);

		const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId);
		if (!Choice)
		{
			OutProfiles.Reset();
			return false;
		}
		OutProfiles.Append(Choice->CombatModifiers);
	}
	return true;
}

bool FGridCombatModifierResolver::CollectStatusModifiers(
	const FGridStatusEffectCollection& StatusEffects, TArray<FGridCombatModifierProfile>& OutProfiles)
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
			OutProfiles.Append(State.DefinitionAsset->CombatModifiers);
		}
	}
	return true;
}

bool FGridCombatModifierResolver::CollectCharacterModifiers(
	const FGridCharacterInventoryState& Character, TArray<FGridCombatModifierProfile>& OutProfiles)
{
	TArray<FGridCombatModifierProfile> ChoiceProfiles;
	TArray<FGridCombatModifierProfile> StatusProfiles;
	if (!CollectCharacterChoiceModifiers(Character, ChoiceProfiles) || !CollectStatusModifiers(Character.StatusEffects, StatusProfiles))
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
	Definition.ResourceCosts.ManaCost = FMath::Max(0, SaturatingAdd(Definition.ResourceCosts.ManaCost, Modifiers.ManaCostModifier));

	if (Definition.RangeCells > 0 || Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack)
	{
		const int32 MinimumRange = Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack ? 1 : 0;
		Definition.RangeCells = FMath::Clamp(SaturatingAdd(Definition.RangeCells, Modifiers.RangeCellsModifier), MinimumRange, 32);
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
}

void FGridCombatModifierResolver::ApplyIncomingAttackModifiers(
	FGridAttackTargetStats& Target, EGridDamageType DamageType, const FGridResolvedCombatModifiers& Modifiers)
{
	Target.Evasion = SaturatingAdd(Target.Evasion, Modifiers.EvasionModifier);
	Target.DamageMultiplier = FMath::Max(0.0f, Target.DamageMultiplier * PercentToMultiplier(Modifiers.IncomingDamagePercentModifier));
	Target.ResistancePercent = FMath::Clamp(SaturatingAdd(Target.ResistancePercent,
		FGridCombatResolver::GetResistancePercent(Modifiers.ResistanceModifiers, DamageType)), -100, 100);
}
