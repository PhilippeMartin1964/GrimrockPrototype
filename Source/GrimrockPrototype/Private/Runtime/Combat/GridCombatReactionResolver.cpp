#include "Runtime/Combat/GridCombatReactionResolver.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridInventoryTypes.h"

namespace
{
	bool CollectReactionOwnerRequirements(const FGridCharacterInventoryState& Character, TSet<FName>& OutRequirements)
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

	bool AreReactionOwnerRequirementsSatisfied(const TArray<FName>& RequiredIds, const TSet<FName>& OwnerRequirements)
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

	void AddProjectedReactionBinding(const FGridCombatReactionProfile& AuthoredProfile, const TSet<FName>& OwnerRequirements,
		FName OwningStatusEffectId, TArray<FGridCombatReactionBinding>& OutBindings)
	{
		if (!AreReactionOwnerRequirementsSatisfied(AuthoredProfile.RequiredOwnerRequirementIds, OwnerRequirements))
		{
			return;
		}
		FGridCombatReactionBinding& Binding = OutBindings.AddDefaulted_GetRef();
		Binding.Profile = AuthoredProfile;
		Binding.Profile.RequiredOwnerRequirementIds.Reset();
		Binding.OwningStatusEffectId = OwningStatusEffectId;
	}
}

bool FGridCombatReactionLedger::CanTrigger(
	const FGridCombatReactionProfile& Profile, const FGuid& OwnerCombatantId, const FGridCombatReactionEvent& Event) const
{
	if (!Profile.IsValid() || !OwnerCombatantId.IsValid() || !Event.IsValid())
	{
		return false;
	}

	const FGridCombatReactionUsageKey Key{ OwnerCombatantId, Profile.ReactionId };
	switch (Profile.Limit)
	{
		case EGridCombatReactionLimit::Unlimited:
			return true;
		case EGridCombatReactionLimit::OncePerRound:
		{
			const int32* LastRound = LastRoundByReaction.Find(Key);
			return !LastRound || *LastRound != Event.RoundNumber;
		}
		case EGridCombatReactionLimit::OncePerAction:
		{
			FGridCombatReactionActionUsageKey ActionKey;
			ActionKey.Reaction = Key;
			ActionKey.ActionInstanceId = Event.ActionInstanceId;
			return !UsedActions.Contains(ActionKey);
		}
		default:
			return false;
	}
}

bool FGridCombatReactionLedger::Commit(
	const FGridCombatReactionProfile& Profile, const FGuid& OwnerCombatantId, const FGridCombatReactionEvent& Event)
{
	if (!CanTrigger(Profile, OwnerCombatantId, Event))
	{
		return false;
	}

	const FGridCombatReactionUsageKey Key{ OwnerCombatantId, Profile.ReactionId };
	if (Profile.Limit == EGridCombatReactionLimit::OncePerRound)
	{
		LastRoundByReaction.Add(Key, Event.RoundNumber);
	}
	else if (Profile.Limit == EGridCombatReactionLimit::OncePerAction)
	{
		FGridCombatReactionActionUsageKey ActionKey;
		ActionKey.Reaction = Key;
		ActionKey.ActionInstanceId = Event.ActionInstanceId;
		UsedActions.Add(ActionKey);
	}
	return true;
}

void FGridCombatReactionLedger::Reset()
{
	LastRoundByReaction.Reset();
	UsedActions.Reset();
}

bool FGridCombatReactionResolver::Matches(const FGridCombatReactionProfile& Profile, const FGridCombatReactionEvent& Event)
{
	if (!Profile.IsValid() || !Profile.RequiredOwnerRequirementIds.IsEmpty() || !Event.IsValid() || Profile.Trigger != Event.Trigger)
	{
		return false;
	}
	if (Event.bReactionGenerated && !Profile.bAllowReactionGeneratedEvents)
	{
		return false;
	}
	if (!Profile.ActionIds.IsEmpty() && !Profile.ActionIds.Contains(Event.ActionId))
	{
		return false;
	}
	if (!Profile.SourcePolicies.IsEmpty() && !Profile.SourcePolicies.Contains(Event.SourcePolicy))
	{
		return false;
	}
	if (!Profile.ActionTypes.IsEmpty() && !Profile.ActionTypes.Contains(Event.ActionType))
	{
		return false;
	}
	if (!Profile.DamageTypes.IsEmpty() && !Profile.DamageTypes.Contains(Event.DamageType))
	{
		return false;
	}
	for (const FName RequiredTag : Profile.RequiredSourceTags)
	{
		if (!Event.SourceTags.Contains(RequiredTag))
		{
			return false;
		}
	}
	if (Profile.bRequireOffensiveAction && !Event.bOffensiveAction)
	{
		return false;
	}
	for (const FName EffectId : Profile.RequiredTargetStatusEffectIdsFromOwner)
	{
		if (!Event.TargetStatusEffectIdsFromOwner.Contains(EffectId))
		{
			return false;
		}
	}
	return true;
}

bool FGridCombatReactionResolver::CollectStatusBindings(
	const FGridStatusEffectCollection& StatusEffects, TArray<FGridCombatReactionBinding>& OutBindings)
{
	return CollectStatusBindings(StatusEffects, TSet<FName>(), OutBindings);
}

bool FGridCombatReactionResolver::CollectStatusBindings(const FGridStatusEffectCollection& StatusEffects,
	const TSet<FName>& OwnerRequirements, TArray<FGridCombatReactionBinding>& OutBindings)
{
	OutBindings.Reset();
	for (const FGridStatusEffectRuntimeState& State : StatusEffects.ActiveEffects)
	{
		if (!State.IsValid() || !IsValid(State.DefinitionAsset) || !State.DefinitionAsset->IsValidDefinition())
		{
			OutBindings.Reset();
			return false;
		}
		for (const FGridCombatReactionProfile& Profile : State.DefinitionAsset->CombatReactions)
		{
			AddProjectedReactionBinding(Profile, OwnerRequirements, State.EffectId, OutBindings);
		}
	}
	return true;
}

bool FGridCombatReactionResolver::CollectCharacterBindings(
	const FGridCharacterInventoryState& Character, TArray<FGridCombatReactionBinding>& OutBindings)
{
	TSet<FName> OwnerRequirements;
	if (!CollectReactionOwnerRequirements(Character, OwnerRequirements))
	{
		OutBindings.Reset();
		return false;
	}

	TArray<FGridCombatReactionBinding> StatusBindings;
	if (!CollectStatusBindings(Character.StatusEffects, OwnerRequirements, StatusBindings))
	{
		OutBindings.Reset();
		return false;
	}

	OutBindings.Reset();
	const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
	if (!IsValid(ClassDefinition))
	{
		OutBindings = MoveTemp(StatusBindings);
		return true;
	}
	if (!ClassDefinition->IsValidDefinition())
	{
		OutBindings.Reset();
		return false;
	}

	for (const FName ChoiceId : Character.SelectedClassProgressionChoiceIds)
	{
		const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId);
		if (!Choice)
		{
			OutBindings.Reset();
			return false;
		}
		for (const FGridCombatReactionProfile& Profile : Choice->CombatReactions)
		{
			AddProjectedReactionBinding(Profile, OwnerRequirements, NAME_None, OutBindings);
		}
	}
	OutBindings.Append(StatusBindings);
	return true;
}

void FGridCombatReactionResolver::ResolveMatches(const TArray<FGridCombatReactionBinding>& Bindings, const FGuid& OwnerCombatantId,
	const FGridCombatReactionEvent& Event, FGridCombatReactionLedger& Ledger, bool bCommit, TArray<FGridCombatReactionMatch>& OutMatches)
{
	OutMatches.Reset();
	for (const FGridCombatReactionBinding& Binding : Bindings)
	{
		if (!Matches(Binding.Profile, Event) || !Ledger.CanTrigger(Binding.Profile, OwnerCombatantId, Event))
		{
			continue;
		}
		if (bCommit && !Ledger.Commit(Binding.Profile, OwnerCombatantId, Event))
		{
			continue;
		}

		FGridCombatReactionMatch& Match = OutMatches.AddDefaulted_GetRef();
		Match.ReactionId = Binding.Profile.ReactionId;
		Match.OwnerCombatantId = OwnerCombatantId;
		Match.OwningStatusEffectId = Binding.OwningStatusEffectId;
		Match.bConsumeOwningStatus = Binding.Profile.bConsumeOwningStatus && !Binding.OwningStatusEffectId.IsNone();
		Match.CounterAttackActionId = Binding.Profile.CounterAttackActionId;
		Match.CounterAttackRangeCells = Binding.Profile.CounterAttackRangeCells;
		Match.CounterAttackWeaponProfile = Binding.Profile.CounterAttackWeaponProfile;
		Match.InterceptFinalDamagePercent = Binding.Profile.InterceptFinalDamagePercent;
		Match.bRequireOwnerFrontRow = Binding.Profile.bRequireOwnerFrontRow;
		Match.bRequireEventTargetFrontRow = Binding.Profile.bRequireEventTargetFrontRow;
		Match.ApplyOwnerStatusEffectId = Binding.Profile.ApplyOwnerStatusEffectId;
		Match.ApplyOwnerStatusDurationOverride = Binding.Profile.ApplyOwnerStatusDurationOverride;
		Match.TransferOwnedTargetStatusEffectId = Binding.Profile.TransferOwnedTargetStatusEffectId;
		Match.TransferTargetRangeCells = Binding.Profile.TransferTargetRangeCells;
		Match.TransferStatusDurationOverride = Binding.Profile.TransferStatusDurationOverride;
		Match.Event = Event;
	}
}
