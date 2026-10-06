#include "Runtime/Combat/GridCombatReactionResolver.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGCharacterRulesLibrary.h"
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
		const TSet<FName>* StatusSourceRequirements, FName OwningStatusEffectId, const FGuid& OwningStatusSourceId,
		TArray<FGridCombatReactionBinding>& OutBindings)
	{
		if (!AreReactionOwnerRequirementsSatisfied(AuthoredProfile.RequiredOwnerRequirementIds, OwnerRequirements))
		{
			return;
		}
		if (!AuthoredProfile.RequiredStatusSourceRequirementIds.IsEmpty() &&
			(!StatusSourceRequirements ||
				!AreReactionOwnerRequirementsSatisfied(AuthoredProfile.RequiredStatusSourceRequirementIds, *StatusSourceRequirements)))
		{
			return;
		}
		FGridCombatReactionBinding& Binding = OutBindings.AddDefaulted_GetRef();
		Binding.Profile = AuthoredProfile;
		Binding.Profile.RequiredOwnerRequirementIds.Reset();
		Binding.Profile.RequiredStatusSourceRequirementIds.Reset();
		Binding.OwningStatusEffectId = OwningStatusEffectId;
		Binding.OwningStatusSourceId = OwningStatusSourceId;
	}

	const FGridCharacterInventoryState* FindPartyCharacterById(const FGridPartyInventoryState& PartyState, const FGuid& CharacterId)
	{
		if (!CharacterId.IsValid())
		{
			return nullptr;
		}
		if (const FGridCharacterInventoryState* Found = PartyState.ActiveCharacters.FindByPredicate(
				[&CharacterId](const FGridCharacterInventoryState& Candidate)
				{
					return Candidate.CharacterId == CharacterId;
				}))
		{
			return Found;
		}
		return PartyState.CharacterPool.FindByPredicate(
			[&CharacterId](const FGridCharacterInventoryState& Candidate)
			{
				return Candidate.CharacterId == CharacterId;
			});
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
	if (!Profile.IsValid() || !Profile.RequiredOwnerRequirementIds.IsEmpty() ||
		!Profile.RequiredStatusSourceRequirementIds.IsEmpty() || !Event.IsValid() || Profile.Trigger != Event.Trigger)
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
	if (Profile.bRequireWeaponAttack && !Event.bWeaponAttack)
	{
		return false;
	}
	if (Profile.bRequireAppliedDamage && !Event.bAppliedDamage)
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
			AddProjectedReactionBinding(Profile, OwnerRequirements, nullptr, State.EffectId, State.SourceId, OutBindings);
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
			AddProjectedReactionBinding(Profile, OwnerRequirements, nullptr, NAME_None, FGuid(), OutBindings);
		}
	}
	OutBindings.Append(StatusBindings);
	return true;
}

bool FGridCombatReactionResolver::CollectCharacterBindings(
	const FGridCharacterInventoryState& Character, const FGridPartyInventoryState& PartyState,
	TArray<FGridCombatReactionBinding>& OutBindings)
{
	TSet<FName> OwnerRequirements;
	if (!CollectReactionOwnerRequirements(Character, OwnerRequirements))
	{
		OutBindings.Reset();
		return false;
	}

	OutBindings.Reset();
	for (const FGridStatusEffectRuntimeState& State : Character.StatusEffects.ActiveEffects)
	{
		if (!State.IsValid() || !IsValid(State.DefinitionAsset) || !State.DefinitionAsset->IsValidDefinition())
		{
			OutBindings.Reset();
			return false;
		}

		TSet<FName> SourceRequirements;
		const FGridCharacterInventoryState* SourceCharacter = FindPartyCharacterById(PartyState, State.SourceId);
		const bool bHasSourceRequirements =
			SourceCharacter && CollectReactionOwnerRequirements(*SourceCharacter, SourceRequirements);
		for (const FGridCombatReactionProfile& Profile : State.DefinitionAsset->CombatReactions)
		{
			AddProjectedReactionBinding(
				Profile, OwnerRequirements, bHasSourceRequirements ? &SourceRequirements : nullptr, State.EffectId, State.SourceId, OutBindings);
		}
	}

	const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
	if (!IsValid(ClassDefinition))
	{
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
			AddProjectedReactionBinding(Profile, OwnerRequirements, nullptr, NAME_None, FGuid(), OutBindings);
		}
	}
	return true;
}

int32 FGridCombatReactionResolver::ResolveSecondaryDirectDamageAmount(
	const FGridCombatReactionProfile& Profile, const FRPGAttributes& Attributes)
{
	if (!Profile.IsValid() || Profile.SecondaryDirectDamage <= 0)
	{
		return 0;
	}
	int32 AttributeValue = 0;
	switch (Profile.SecondaryDirectDamageScalingAttribute)
	{
		case EGridAttackScalingAttribute::Strength: AttributeValue = Attributes.Strength; break;
		case EGridAttackScalingAttribute::Dexterity: AttributeValue = Attributes.Dexterity; break;
		case EGridAttackScalingAttribute::Constitution: AttributeValue = Attributes.Constitution; break;
		case EGridAttackScalingAttribute::Intelligence: AttributeValue = Attributes.Intelligence; break;
		case EGridAttackScalingAttribute::Wisdom: AttributeValue = Attributes.Wisdom; break;
		case EGridAttackScalingAttribute::Charisma: AttributeValue = Attributes.Charisma; break;
		case EGridAttackScalingAttribute::None:
		default:
			return Profile.SecondaryDirectDamage;
	}
	return FMath::Max(0, Profile.SecondaryDirectDamage +
		URPGCharacterRulesLibrary::GetAttributeModifier(AttributeValue) * Profile.SecondaryDirectDamageAttributeModifierScale);
}

void FGridCombatReactionResolver::ResolveMatches(const TArray<FGridCombatReactionBinding>& Bindings, const FGuid& OwnerCombatantId,
	const FGridCombatReactionEvent& Event, FGridCombatReactionLedger& Ledger, bool bCommit, TArray<FGridCombatReactionMatch>& OutMatches)
{
	OutMatches.Reset();
	for (const FGridCombatReactionBinding& Binding : Bindings)
	{
		if ((Binding.Profile.bRequireOwnerAsEventSource && Event.SourceCombatantId != OwnerCombatantId) ||
			!Matches(Binding.Profile, Event) || !Ledger.CanTrigger(Binding.Profile, OwnerCombatantId, Event))
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
		Match.OwningStatusSourceId = Binding.OwningStatusSourceId;
		Match.bConsumeOwningStatus = Binding.Profile.bConsumeOwningStatus && !Binding.OwningStatusEffectId.IsNone();
		Match.CounterAttackActionId = Binding.Profile.CounterAttackActionId;
		Match.CounterAttackRangeCells = Binding.Profile.CounterAttackRangeCells;
		Match.CounterAttackWeaponProfile = Binding.Profile.CounterAttackWeaponProfile;
		Match.InterceptFinalDamagePercent = Binding.Profile.InterceptFinalDamagePercent;
		Match.bRequireOwnerFrontRow = Binding.Profile.bRequireOwnerFrontRow;
		Match.bRequireEventTargetFrontRow = Binding.Profile.bRequireEventTargetFrontRow;
		Match.SecondaryDirectDamage = Binding.Profile.SecondaryDirectDamage;
		Match.SecondaryDirectDamageType = Binding.Profile.SecondaryDirectDamageType;
		Match.SecondaryDirectDamagePhysicalSubtype = Binding.Profile.SecondaryDirectDamagePhysicalSubtype;
		Match.SecondaryDirectDamageScalingAttribute = Binding.Profile.SecondaryDirectDamageScalingAttribute;
		Match.SecondaryDirectDamageAttributeModifierScale = Binding.Profile.SecondaryDirectDamageAttributeModifierScale;
		Match.ApplyOwnerStatusEffectId = Binding.Profile.ApplyOwnerStatusEffectId;
		Match.ApplyOwnerStatusDurationOverride = Binding.Profile.ApplyOwnerStatusDurationOverride;
		Match.TransferOwnedTargetStatusEffectId = Binding.Profile.TransferOwnedTargetStatusEffectId;
		Match.TransferTargetRangeCells = Binding.Profile.TransferTargetRangeCells;
		Match.TransferStatusDurationOverride = Binding.Profile.TransferStatusDurationOverride;
		Match.Event = Event;
	}
}
