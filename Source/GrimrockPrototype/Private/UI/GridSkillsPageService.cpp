#include "UI/GridSkillsPageService.h"

#include "UI/RPGTalentPresentationAsset.h"
#include "Runtime/Monsters/GridMonsterCategoryAsset.h"

#include "Engine/AssetManager.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillService.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
	const FPrimaryAssetType RPGSkillPrimaryAssetType(TEXT("RPGSkill"));
	using FChoiceArray = TArray<const FRPGClassProgressionChoiceDefinition*>;

	bool ValidateAndSortDefinitions(const TArray<const URPGSkillAsset*>& SkillDefinitions, TArray<const URPGSkillAsset*>& OutSortedDefinitions)
	{
		OutSortedDefinitions.Reset(SkillDefinitions.Num());
		TSet<FName> SeenSkillIds;
		for (const URPGSkillAsset* Definition : SkillDefinitions)
		{
			if (!IsValid(Definition) || !Definition->IsValidDefinition() || SeenSkillIds.Contains(Definition->SkillId))
			{
				OutSortedDefinitions.Reset();
				return false;
			}
			SeenSkillIds.Add(Definition->SkillId);
			OutSortedDefinitions.Add(Definition);
		}
		OutSortedDefinitions.Sort(
			[](const URPGSkillAsset& Left, const URPGSkillAsset& Right)
			{
				const FString LeftLabel = Left.DisplayName.IsEmpty() ? Left.SkillId.ToString() : Left.DisplayName.ToString();
				const FString RightLabel = Right.DisplayName.IsEmpty() ? Right.SkillId.ToString() : Right.DisplayName.ToString();
				const int32 LabelComparison = LeftLabel.Compare(RightLabel, ESearchCase::IgnoreCase);
				if (LabelComparison != 0)
				{
					return LabelComparison < 0;
				}
				return Left.SkillId.ToString().Compare(Right.SkillId.ToString(), ESearchCase::CaseSensitive) < 0;
			});
		return true;
	}

	URPGClassAsset* ResolveClassDefinition(const FGridCharacterInventoryState& Character)
	{
		URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition))
		{
			ClassDefinition = FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
		}
		return IsValid(ClassDefinition) && ClassDefinition->IsValidDefinition() ? ClassDefinition : nullptr;
	}

	int32 ResolveTalentTier(int32 MinimumLevel)
	{
		switch (MinimumLevel)
		{
			case 2: return 1;
			case 6: return 2;
			case 10: return 3;
			case 14: return 4;
			case 18: return 5;
			default: return 0;
		}
	}

	bool BuildSelectedChoiceSet(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, TSet<FName>& OutSelectedChoiceIds)
	{
		OutSelectedChoiceIds.Reset();
		TArray<FName> SelectedChoiceIds;
		if (!FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(PartyInventoryComponent, CharacterIndex, SelectedChoiceIds))
		{
			return false;
		}
		for (const FName ChoiceId : SelectedChoiceIds)
		{
			if (ChoiceId.IsNone() || OutSelectedChoiceIds.Contains(ChoiceId))
			{
				OutSelectedChoiceIds.Reset();
				return false;
			}
			OutSelectedChoiceIds.Add(ChoiceId);
		}
		return true;
	}

	bool TryMapChoiceState(bool bSelected, ERPGClassProgressionChoiceAvailabilityReason Availability, EGridTalentNodeState& OutState)
	{
		if (bSelected)
		{
			OutState = EGridTalentNodeState::Acquired;
			return Availability == ERPGClassProgressionChoiceAvailabilityReason::AlreadySelected ||
				Availability == ERPGClassProgressionChoiceAvailabilityReason::None;
		}
		switch (Availability)
		{
			case ERPGClassProgressionChoiceAvailabilityReason::None:
				OutState = EGridTalentNodeState::Available; return true;
			case ERPGClassProgressionChoiceAvailabilityReason::LevelTooLow:
				OutState = EGridTalentNodeState::LockedLevel; return true;
			case ERPGClassProgressionChoiceAvailabilityReason::MissingPrerequisite:
				OutState = EGridTalentNodeState::LockedPrerequisite; return true;
			case ERPGClassProgressionChoiceAvailabilityReason::InsufficientChoicePoints:
				OutState = EGridTalentNodeState::LockedPoints; return true;
			case ERPGClassProgressionChoiceAvailabilityReason::MutuallyExclusiveChoice:
				OutState = EGridTalentNodeState::LockedExclusive; return true;
			default:
				return false;
		}
	}

	bool AreNameArraysEquivalent(const TArray<FName>& Left, const TArray<FName>& Right)
	{
		if (Left.Num() != Right.Num()) return false;
		TSet<FName> RightSet;
		for (const FName Id : Right)
		{
			if (Id.IsNone() || RightSet.Contains(Id)) return false;
			RightSet.Add(Id);
		}
		TSet<FName> LeftSet;
		for (const FName Id : Left)
		{
			if (Id.IsNone() || LeftSet.Contains(Id) || !RightSet.Contains(Id)) return false;
			LeftSet.Add(Id);
		}
		return LeftSet.Num() == RightSet.Num();
	}

	void BuildSatisfiedIdsForChoice(const FRPGClassProgressionChoiceDefinition& Choice, TSet<FName>& OutIds)
	{
		OutIds.Reset();
		OutIds.Add(Choice.ChoiceId);
		for (const FName RequirementId : Choice.GrantedRequirementIds) OutIds.Add(RequirementId);
	}

	FText MonsterCategoryDisplayName(FName CategoryId)
	{
		if (const UGridMonsterCategoryAsset* Category =
				UGridMonsterCategoryAsset::ResolveByCategoryId(CategoryId))
		{
			return Category->DisplayName;
		}
		return FText::GetEmpty();
	}

	FText SkillDisplayName(FName SkillId)
	{
		const URPGSkillAsset* Definition =
			FRPGSkillRequirementProjectionService::ResolveDefinitionBySkillId(SkillId);
		return IsValid(Definition) ? Definition->DisplayName : FText::GetEmpty();
	}
	FName PhysicalSubtypeTextId(EGridPhysicalDamageSubtype Type)
	{
		switch (Type)
		{
			case EGridPhysicalDamageSubtype::Slashing: return TEXT("Talent.Context.Weapon.Slashing");
			case EGridPhysicalDamageSubtype::Piercing: return TEXT("Talent.Context.Weapon.Piercing");
			case EGridPhysicalDamageSubtype::Bludgeoning: return TEXT("Talent.Context.Weapon.Bludgeoning");
			default: return NAME_None;
		}
	}

	FName SourcePolicyTextId(EGridCombatActionSourcePolicy Policy)
	{
		switch (Policy)
		{
			case EGridCombatActionSourcePolicy::Universal: return TEXT("Talent.Context.Source.Universal");
			case EGridCombatActionSourcePolicy::Equipment: return TEXT("Talent.Context.Source.Equipment");
			case EGridCombatActionSourcePolicy::Ability: return TEXT("Talent.Context.Source.Ability");
			case EGridCombatActionSourcePolicy::Spell: return TEXT("Talent.Context.Source.Spell");
			case EGridCombatActionSourcePolicy::QuickItem: return TEXT("Talent.Context.Source.QuickItem");
			default: return NAME_None;
		}
	}

	FName DamageTypeTextId(EGridDamageType Type)
	{
		switch (Type)
		{
			case EGridDamageType::Physical: return TEXT("Talent.Context.Damage.Physical");
			case EGridDamageType::Fire: return TEXT("Talent.Context.Damage.Fire");
			case EGridDamageType::Ice: return TEXT("Talent.Context.Damage.Ice");
			case EGridDamageType::Lightning: return TEXT("Talent.Context.Damage.Lightning");
			case EGridDamageType::Poison: return TEXT("Talent.Context.Damage.Poison");
			case EGridDamageType::Holy: return TEXT("Talent.Context.Damage.Holy");
			case EGridDamageType::Necrotic: return TEXT("Talent.Context.Damage.Necrotic");
			case EGridDamageType::Arcane: return TEXT("Talent.Context.Damage.Arcane");
			default: return NAME_None;
		}
	}

	FName TargetConditionTextId(EGridCombatTargetCondition Condition)
	{
		switch (Condition)
		{
			case EGridCombatTargetCondition::RearArc: return TEXT("Talent.Context.Target.RearArc");
			case EGridCombatTargetCondition::HasActedThisRound: return TEXT("Talent.Context.Target.HasActedThisRound");
			case EGridCombatTargetCondition::HasNotActedThisRound: return TEXT("Talent.Context.Target.HasNotActedThisRound");
			case EGridCombatTargetCondition::PhysicalControl: return TEXT("Talent.Context.Target.PhysicalControl");
			default: return NAME_None;
		}
	}

	FName ReactionTriggerTextId(EGridCombatReactionTrigger Trigger)
	{
		switch (Trigger)
		{
			case EGridCombatReactionTrigger::ActionResolved: return TEXT("Talent.Reaction.Trigger.ActionResolved");
			case EGridCombatReactionTrigger::AttackHit: return TEXT("Talent.Reaction.Trigger.AttackHit");
			case EGridCombatReactionTrigger::AttackMiss: return TEXT("Talent.Reaction.Trigger.AttackMiss");
			case EGridCombatReactionTrigger::TargetDefeated: return TEXT("Talent.Reaction.Trigger.TargetDefeated");
			case EGridCombatReactionTrigger::DirectDamageReceived: return TEXT("Talent.Reaction.Trigger.DirectDamageReceived");
			case EGridCombatReactionTrigger::SurfaceReaction: return TEXT("Talent.Reaction.Trigger.SurfaceReaction");
			case EGridCombatReactionTrigger::IncomingAttackHit: return TEXT("Talent.Reaction.Trigger.IncomingAttackHit");
			case EGridCombatReactionTrigger::OwnedStatusTargetDefeated: return TEXT("Talent.Reaction.Trigger.OwnedStatusTargetDefeated");
			default: return NAME_None;
		}
	}

	FName ReactionLimitTextId(EGridCombatReactionLimit Limit)
	{
		switch (Limit)
		{
			case EGridCombatReactionLimit::Unlimited: return TEXT("Talent.Reaction.Limit.Unlimited");
			case EGridCombatReactionLimit::OncePerRound: return TEXT("Talent.Reaction.Limit.OncePerRound");
			case EGridCombatReactionLimit::OncePerAction: return TEXT("Talent.Reaction.Limit.OncePerAction");
			default: return NAME_None;
		}
	}

	FName TargetingPolicyTextId(EGridCombatTargetingPolicy Policy)
	{
		switch (Policy)
		{
			case EGridCombatTargetingPolicy::Self: return TEXT("Talent.Target.Self");
			case EGridCombatTargetingPolicy::Ally: return TEXT("Talent.Target.Ally");
			case EGridCombatTargetingPolicy::FirstAxialTarget: return TEXT("Talent.Target.FirstAxialTarget");
			case EGridCombatTargetingPolicy::Cell: return TEXT("Talent.Target.Cell");
			case EGridCombatTargetingPolicy::Area: return TEXT("Talent.Target.Area");
			case EGridCombatTargetingPolicy::Hostile: return TEXT("Talent.Target.Hostile");
			case EGridCombatTargetingPolicy::Party: return TEXT("Talent.Target.Party");
			case EGridCombatTargetingPolicy::FrontRowParty: return TEXT("Talent.Target.FrontRowParty");
			case EGridCombatTargetingPolicy::AllyOrHostile: return TEXT("Talent.Target.AllyOrHostile");
			default: return NAME_None;
		}
	}

	void CollectUnlockedActions(
		const URPGClassAsset& ClassDefinition,
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<const FGridCombatActionDefinition*>& OutActions)
	{
		OutActions.Reset();

		TSet<FName> SatisfiedIds;
		BuildSatisfiedIdsForChoice(Choice, SatisfiedIds);
		for (const FGridCombatActionDefinition& Action : ClassDefinition.CombatActions)
		{
			const bool bUnlockedByChoice = Action.Requirements.ContainsByPredicate(
				[&SatisfiedIds](const FName RequirementId)
				{
					return SatisfiedIds.Contains(RequirementId);
				});
			if (bUnlockedByChoice)
			{
				OutActions.Add(&Action);
			}
		}
	}



	void AddDetailLine(
		TArray<FGridTalentDetailLineView>& Lines,
		FName LabelTextId,
		const FString& Value,
		EGridTalentDetailLineSemantic Semantic = EGridTalentDetailLineSemantic::None)
	{
		if (Value.IsEmpty())
		{
			return;
		}
		FGridTalentDetailLineView Line;
		Line.Semantic = Semantic;
		Line.LabelTextId = LabelTextId;
		Line.Value = FText::FromString(Value);
		Lines.Add(MoveTemp(Line));
	}

	void AddDetailTextLine(
		TArray<FGridTalentDetailLineView>& Lines,
		FName LabelTextId,
		FName ValueTextId,
		EGridTalentDetailLineSemantic Semantic = EGridTalentDetailLineSemantic::None)
	{
		if (ValueTextId.IsNone())
		{
			return;
		}
		FGridTalentDetailLineView Line;
		Line.Semantic = Semantic;
		Line.LabelTextId = LabelTextId;
		Line.ValueTextId = ValueTextId;
		Lines.Add(MoveTemp(Line));
	}

	void AddNumericLine(
		TArray<FGridTalentDetailLineView>& Lines,
		FName LabelTextId,
		int32 Value,
		EGridTalentDetailValueStyle ValueStyle,
		EGridTalentDetailLineSemantic Semantic = EGridTalentDetailLineSemantic::None)
	{
		FGridTalentDetailLineView Line;
		Line.Semantic = Semantic;
		Line.LabelTextId = LabelTextId;
		Line.NumericValue = Value;
		Line.bHasNumericValue = true;
		Line.ValueStyle = ValueStyle;
		Lines.Add(MoveTemp(Line));
	}

	void AddDetailNumberLine(
		TArray<FGridTalentDetailLineView>& Lines,
		FName LabelTextId,
		int32 Value,
		EGridTalentDetailLineSemantic Semantic = EGridTalentDetailLineSemantic::None)
	{
		AddNumericLine(Lines, LabelTextId, Value, EGridTalentDetailValueStyle::Integer, Semantic);
	}

	void AddPercentLine(
		TArray<FGridTalentDetailLineView>& Lines,
		FName LabelTextId,
		int32 Value,
		EGridTalentDetailLineSemantic Semantic = EGridTalentDetailLineSemantic::None)
	{
		AddNumericLine(Lines, LabelTextId, Value, EGridTalentDetailValueStyle::Percent, Semantic);
	}

	void AddSignedValueLine(TArray<FGridTalentDetailLineView>& Lines, FName LabelTextId, int32 Value)
	{
		if (Value != 0)
		{
			AddNumericLine(Lines, LabelTextId, Value, EGridTalentDetailValueStyle::SignedInteger);
		}
	}

	void AddSignedPercentLine(TArray<FGridTalentDetailLineView>& Lines, FName LabelTextId, int32 Value)
	{
		if (Value != 0)
		{
			AddNumericLine(Lines, LabelTextId, Value, EGridTalentDetailValueStyle::SignedPercent);
		}
	}

	void BuildStructuredSkillPartyEffects(
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<FGridTalentDetailLineView>& Out)
	{
		for (const FRPGSkillProgressionModifier& Skill : Choice.SkillModifiers)
		{
			const FText Name = SkillDisplayName(Skill.SkillId);
			if (!Name.IsEmpty())
			{
				AddDetailLine(Out, TEXT("Talent.Skill.Name"), Name.ToString());
			}
			if (Skill.CheckModifier != 0)
			{
				AddSignedValueLine(Out, TEXT("Talent.Skill.CheckModifier"), Skill.CheckModifier);
			}
			if (Skill.RequirementGrantRankModifier > 0)
			{
				AddSignedValueLine(
					Out,
					TEXT("Talent.Skill.RequirementGrantRankModifier"),
					Skill.RequirementGrantRankModifier);
			}
			if (Skill.SafeFailureMargin > 0)
			{
				AddDetailNumberLine(
					Out,
					TEXT("Talent.Skill.SafeFailureMargin"),
					Skill.SafeFailureMargin);
			}
			if (Skill.bRequireRangedContext)
			{
				AddDetailTextLine(
					Out,
					TEXT("Talent.Skill.Context"),
					TEXT("Talent.Skill.Context.Ranged"));
			}
			for (const FName CategoryId : Skill.RelatedMonsterCategoryIds)
			{
				const FText CategoryName = MonsterCategoryDisplayName(CategoryId);
				if (!CategoryName.IsEmpty())
				{
					AddDetailLine(
						Out,
						TEXT("Talent.Skill.RelatedCategory"),
						CategoryName.ToString());
				}
			}
		}

		for (const FRPGPartyProgressionModifier& Party : Choice.PartyModifiers)
		{
			for (const FName SkillId : Party.GroupSkillIds)
			{
				const FText Name = SkillDisplayName(SkillId);
				if (!Name.IsEmpty())
				{
					AddDetailLine(Out, TEXT("Talent.Party.GroupSkill"), Name.ToString());
				}
			}
			if (Party.GroupSkillCheckModifier != 0)
			{
				AddSignedValueLine(
					Out,
					TEXT("Talent.Party.GroupSkillCheckModifier"),
					Party.GroupSkillCheckModifier);
			}
			if (Party.MaximumMobilityActionPointsModifier != 0)
			{
				AddSignedValueLine(
					Out,
					TEXT("Talent.Party.MaximumMobilityActionPointsModifier"),
					Party.MaximumMobilityActionPointsModifier);
			}
		}

		if (Choice.FirstRoundInitiativeModifier != 0)
		{
			AddSignedValueLine(
				Out,
				TEXT("Talent.Effect.FirstRoundInitiative"),
				Choice.FirstRoundInitiativeModifier);
		}
	}

	void AddStatusDurationLine(
		TArray<FGridTalentDetailLineView>& Lines,
		const UGridStatusEffectDefinitionAsset& Definition,
		int32 DurationOverride,
		FName TurnsLabelTextId,
		FName RoundsLabelTextId,
		FName PermanentLabelTextId)
	{
		const int32 Duration =
			DurationOverride == INDEX_NONE ? Definition.DefaultDuration : DurationOverride;
		switch (Definition.DurationUnit)
		{
			case EGridStatusEffectDurationUnit::Turns:
				AddDetailNumberLine(Lines, TurnsLabelTextId, Duration);
				break;
			case EGridStatusEffectDurationUnit::Rounds:
				AddDetailNumberLine(Lines, RoundsLabelTextId, Duration);
				break;
			case EGridStatusEffectDurationUnit::Permanent:
				AddDetailTextLine(Lines, PermanentLabelTextId, TEXT("Talent.Value.Permanent"));
				break;
			default:
				break;
		}
	}

	void AddResolvedStatusLine(
		TArray<FGridTalentDetailLineView>& Lines,
		FName LabelTextId,
		FName StatusEffectId,
		const UGridStatusEffectDefinitionAsset*& OutDefinition)
	{
		OutDefinition = FGridStatusEffectPersistence::ResolveDefinitionByEffectId(StatusEffectId);
		if (IsValid(OutDefinition) && !OutDefinition->DisplayName.IsEmpty())
		{
			AddDetailLine(Lines, LabelTextId, OutDefinition->DisplayName.ToString());
		}
	}

	void BuildStructuredReactionEffects(
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<FGridTalentDetailLineView>& Out)
	{
		for (const FGridCombatReactionProfile& Reaction : Choice.CombatReactions)
		{
			AddDetailTextLine(
				Out, TEXT("Talent.Reaction.Trigger"), ReactionTriggerTextId(Reaction.Trigger));
			AddDetailTextLine(
				Out, TEXT("Talent.Reaction.Limit"), ReactionLimitTextId(Reaction.Limit));

			if (Reaction.CounterAttackWeaponProfile.bUseEquippedWeapon)
			{
				AddPercentLine(
					Out,
					TEXT("Talent.Reaction.CounterAttackWeaponDamagePercent"),
					Reaction.CounterAttackWeaponProfile.WeaponDamagePercent);
				AddDetailNumberLine(
					Out,
					TEXT("Talent.Reaction.CounterAttackRangeCells"),
					Reaction.CounterAttackRangeCells);
			}
			if (Reaction.InterceptFinalDamagePercent > 0)
			{
				AddPercentLine(
					Out,
					TEXT("Talent.Reaction.InterceptFinalDamagePercent"),
					Reaction.InterceptFinalDamagePercent);
			}
			if (Reaction.SurfaceReactionDamagePercentModifier != 0)
			{
				AddSignedPercentLine(
					Out,
					TEXT("Talent.Reaction.SurfaceDamagePercentModifier"),
					Reaction.SurfaceReactionDamagePercentModifier);
			}
			if (Reaction.SurfaceReactionAreaRadiusModifier != 0)
			{
				AddSignedValueLine(
					Out,
					TEXT("Talent.Reaction.SurfaceRadiusModifier"),
					Reaction.SurfaceReactionAreaRadiusModifier);
			}
			if (Reaction.SecondaryDirectDamage > 0)
			{
				AddDetailNumberLine(
					Out,
					TEXT("Talent.Reaction.SecondaryDirectDamage"),
					Reaction.SecondaryDirectDamage);
				AddDetailTextLine(
					Out,
					TEXT("Talent.Reaction.SecondaryDamageType"),
					DamageTypeTextId(Reaction.SecondaryDirectDamageType));
			}

			if (!Reaction.ApplyOwnerStatusEffectId.IsNone())
			{
				const UGridStatusEffectDefinitionAsset* Definition = nullptr;
				AddResolvedStatusLine(
					Out,
					TEXT("Talent.Reaction.ApplyOwnerStatus"),
					Reaction.ApplyOwnerStatusEffectId,
					Definition);
				if (IsValid(Definition))
				{
					AddStatusDurationLine(
						Out,
						*Definition,
						Reaction.ApplyOwnerStatusDurationOverride,
						TEXT("Talent.Reaction.ApplyOwnerStatusDurationTurns"),
						TEXT("Talent.Reaction.ApplyOwnerStatusDurationRounds"),
						TEXT("Talent.Reaction.ApplyOwnerStatusDuration"));
				}
			}

			if (!Reaction.TransferOwnedTargetStatusEffectId.IsNone())
			{
				const UGridStatusEffectDefinitionAsset* Definition = nullptr;
				AddResolvedStatusLine(
					Out,
					TEXT("Talent.Reaction.TransferStatus"),
					Reaction.TransferOwnedTargetStatusEffectId,
					Definition);
				AddDetailNumberLine(
					Out,
					TEXT("Talent.Reaction.TransferRangeCells"),
					Reaction.TransferTargetRangeCells);
				if (IsValid(Definition))
				{
					AddStatusDurationLine(
						Out,
						*Definition,
						Reaction.TransferStatusDurationOverride,
						TEXT("Talent.Reaction.TransferStatusDurationTurns"),
						TEXT("Talent.Reaction.TransferStatusDurationRounds"),
						TEXT("Talent.Reaction.TransferStatusDuration"));
				}
			}

			if (Reaction.bConsumeOwningStatus)
			{
				AddDetailTextLine(
					Out, NAME_None, TEXT("Talent.Reaction.ConsumeOwningStatus"));
			}
		}
	}

	void BuildStructuredCombatModifierEffects(
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<FGridTalentDetailLineView>& Out)
	{
		for (const FGridCombatModifierProfile& Modifier : Choice.CombatModifiers)
		{
			for (const EGridCombatActionSourcePolicy Policy : Modifier.SourcePolicies)
			{
				AddDetailTextLine(Out, NAME_None, SourcePolicyTextId(Policy), EGridTalentDetailLineSemantic::Context);
			}
			for (const EGridPhysicalDamageSubtype Subtype : Modifier.PhysicalSubtypes)
			{
				AddDetailTextLine(Out, NAME_None, PhysicalSubtypeTextId(Subtype), EGridTalentDetailLineSemantic::Context);
			}
			for (const EGridDamageType Type : Modifier.DamageTypes)
			{
				AddDetailTextLine(Out, NAME_None, DamageTypeTextId(Type), EGridTalentDetailLineSemantic::Context);
			}
			for (const EGridCombatTargetCondition Condition : Modifier.RequiredTargetConditions)
			{
				AddDetailTextLine(Out, NAME_None, TargetConditionTextId(Condition), EGridTalentDetailLineSemantic::Context);
			}
			for (const EGridCombatTargetCondition Condition : Modifier.AnyTargetConditions)
			{
				AddDetailTextLine(Out, NAME_None, TargetConditionTextId(Condition), EGridTalentDetailLineSemantic::Context);
			}
			for (const FName StatusId : Modifier.RequiredTargetStatusEffectIds)
			{
				if (const UGridStatusEffectDefinitionAsset* Definition =
					FGridStatusEffectPersistence::ResolveDefinitionByEffectId(StatusId))
				{
					if (!Definition->DisplayName.IsEmpty())
					{
						AddDetailLine(Out, TEXT("Talent.Context.TargetStatus"), Definition->DisplayName.ToString(), EGridTalentDetailLineSemantic::Context);
					}
				}
			}
			for (const FName CategoryId : Modifier.AllowedTargetMonsterCategoryIds)
			{
				const FText CategoryName = MonsterCategoryDisplayName(CategoryId);
				if (!CategoryName.IsEmpty())
				{
					AddDetailLine(
						Out,
						TEXT("Talent.Context.TargetCategory"),
						CategoryName.ToString(),
						EGridTalentDetailLineSemantic::Context);
				}
			}
			for (const FName Tag : Modifier.RequiredSourceTags)
			{
				AddDetailTextLine(
					Out,
					TEXT("Talent.Context.SourceTag"),
					FName(*(FString(TEXT("Talent.Context.Tag.")) + Tag.ToString())),
					EGridTalentDetailLineSemantic::Context);
			}
			if (Modifier.bRequirePartyStationarySincePreviousActivation)
			{
				AddDetailTextLine(Out, NAME_None, TEXT("Talent.Context.PartyStationary"), EGridTalentDetailLineSemantic::Context);
			}
			if (Modifier.bExcludeAreaActions)
			{
				AddDetailTextLine(Out, NAME_None, TEXT("Talent.Context.ExcludeAreaActions"), EGridTalentDetailLineSemantic::Context);
			}

			AddSignedValueLine(Out, TEXT("Talent.Effect.Accuracy"), Modifier.AccuracyModifier);
			AddSignedValueLine(Out, TEXT("Talent.Effect.Evasion"), Modifier.EvasionModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.OutgoingDamage"), Modifier.OutgoingDamagePercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.IncomingDamage"), Modifier.IncomingDamagePercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.CriticalChance"), Modifier.CriticalChancePercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.CriticalDamage"), Modifier.CriticalDamagePercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.WeaponDamage"), Modifier.WeaponDamagePercentModifier);
			AddSignedValueLine(Out, TEXT("Talent.Effect.ActionPointCost"), Modifier.ActionPointCostModifier);
			AddSignedValueLine(Out, TEXT("Talent.Effect.ManaCost"), Modifier.ManaCostModifier);
			AddSignedValueLine(Out, TEXT("Talent.Effect.RangeCells"), Modifier.RangeCellsModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.PositiveEffects"), Modifier.PositiveEffectPercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.OutgoingHealing"), Modifier.OutgoingHealingPercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.FriendlyDirectDamage"), Modifier.FriendlyDirectDamagePercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.SelfDirectDamage"), Modifier.SelfDirectDamagePercentModifier);
			AddSignedValueLine(Out, TEXT("Talent.Effect.QuickItemSecondaryTargets"), Modifier.QuickItemSecondaryTargetCount);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.SecondaryMagnitude"), Modifier.QuickItemSecondaryMagnitudePercent);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.SecondaryDuration"), Modifier.QuickItemSecondaryDurationPercent);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.PhysicalArmorReference"), Modifier.PhysicalArmorReferencePercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.MagicalArmorReference"), Modifier.MagicalArmorReferencePercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.PhysicalArmorRestoration"), Modifier.PhysicalArmorRestorationPercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.MagicalArmorRestoration"), Modifier.MagicalArmorRestorationPercentModifier);
			AddSignedValueLine(Out, TEXT("Talent.Effect.SurfaceDurationRounds"), Modifier.SurfaceDurationRoundsModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.SurfacePeriodicDamage"), Modifier.SurfacePeriodicDamagePercentModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.SurfaceReactionDamage"), Modifier.SurfaceReactionDamagePercentModifier);
			AddSignedValueLine(Out, TEXT("Talent.Effect.SurfaceReactionRadius"), Modifier.SurfaceReactionAreaRadiusModifier);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.Resistance.Physical"), Modifier.ResistanceModifiers.PhysicalResistance);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.Resistance.Fire"), Modifier.ResistanceModifiers.FireResistance);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.Resistance.Ice"), Modifier.ResistanceModifiers.IceResistance);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.Resistance.Lightning"), Modifier.ResistanceModifiers.LightningResistance);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.Resistance.Poison"), Modifier.ResistanceModifiers.PoisonResistance);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.Resistance.Holy"), Modifier.ResistanceModifiers.HolyResistance);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.Resistance.Necrotic"), Modifier.ResistanceModifiers.NecroticResistance);
			AddSignedPercentLine(Out, TEXT("Talent.Effect.Resistance.Arcane"), Modifier.ResistanceModifiers.ArcaneResistance);
			if (Modifier.MinimumManaCost > 0)
			{
				AddDetailNumberLine(Out, TEXT("Talent.Effect.MinimumManaCost"), Modifier.MinimumManaCost);
			}
		}
	}


	FText ConceptualChoiceDisplayName(const URPGClassAsset& ClassDefinition, FName RequirementId)
	{
		for (const FRPGClassProgressionChoiceDefinition& Candidate : ClassDefinition.ProgressionChoices)
		{
			if (Candidate.ChoiceId != RequirementId && Candidate.TalentNodeId != RequirementId)
			{
				continue;
			}
			FString Label = Candidate.DisplayName.ToString();
			if (Candidate.TalentNodeId == RequirementId && Candidate.ChoiceId != RequirementId)
			{
				for (const FString Separator : { FString(TEXT(" — ")), FString(TEXT(" – ")), FString(TEXT(" - ")), FString(TEXT(": ")) })
				{
					const int32 Index = Label.Find(Separator, ESearchCase::CaseSensitive);
					if (Index > 0)
					{
						Label = Label.Left(Index);
						break;
					}
				}
			}
			Label.TrimStartAndEndInline();
			return FText::FromString(Label);
		}
		return FText::GetEmpty();
	}

	FName RecipeTextId(FName RecipeId)
	{
		if (RecipeId.IsNone() ||
			!RecipeId.ToString().StartsWith(TEXT("Recipe_"), ESearchCase::CaseSensitive))
		{
			return NAME_None;
		}
		return FName(*(FString(TEXT("Talent.Recipe.")) + RecipeId.ToString()));
	}

	void BuildAcquisitionView(
		const URPGClassAsset& ClassDefinition,
		const FRPGClassProgressionChoiceDefinition& Choice,
		bool bExclusiveVariant,
		FGridTalentAcquisitionView& Out)
	{
		Out = FGridTalentAcquisitionView();
		Out.MinimumLevel = Choice.MinimumLevel;
		Out.PointCost = Choice.PointCost;
		for (const FName Id : Choice.PrerequisiteChoiceIds)
		{
			const FText Name = ConceptualChoiceDisplayName(ClassDefinition, Id);
			if (!Name.IsEmpty()) Out.PrerequisiteTalentNames.Add(Name);
		}
		for (const FName Id : Choice.PrerequisiteRequirementIds)
		{
			const FText Name = ConceptualChoiceDisplayName(ClassDefinition, Id);
			if (!Name.IsEmpty() && !Out.PrerequisiteTalentNames.ContainsByPredicate(
				[&Name](const FText& Existing) { return Existing.EqualTo(Name); })) Out.PrerequisiteTalentNames.Add(Name);
		}
		for (const FName Id : Choice.GrantedRequirementIds)
		{
			const FName TextId = RecipeTextId(Id);
			if (!TextId.IsNone())
			{
				Out.GrantedRecipeTextIds.AddUnique(TextId);
			}
		}
		if (bExclusiveVariant)
		{
			Out.ExclusivityTextId = TEXT("Talent.Acquisition.Exclusivity.SingleVariant");
		}
	}

	void BuildStructuredUsage(
		const URPGClassAsset& ClassDefinition,
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<FGridTalentDetailLineView>& Out)
	{
		Out.Reset();
		TArray<const FGridCombatActionDefinition*> Actions;
		CollectUnlockedActions(ClassDefinition, Choice, Actions);
		for (const FGridCombatActionDefinition* ActionPtr : Actions)
		{
			if (!ActionPtr) continue;
			const FGridCombatActionDefinition& Action = *ActionPtr;
			if (Actions.Num() > 1 && !Action.DisplayName.IsEmpty())
			{
				AddDetailLine(Out, TEXT("Talent.Detail.Action"), Action.DisplayName.ToString(), EGridTalentDetailLineSemantic::Action);
			}
			if (Action.ActionPointCost > 0)
			{
				AddDetailNumberLine(
					Out, TEXT("Talent.Detail.ActionPointCost"), Action.ActionPointCost,
					EGridTalentDetailLineSemantic::ActionPointCost);
			}
			if (Action.ResourceCosts.ManaCost > 0)
			{
				AddDetailNumberLine(
					Out, TEXT("Talent.Detail.ManaCost"), Action.ResourceCosts.ManaCost,
					EGridTalentDetailLineSemantic::ManaCost);
			}
			if (Action.ResourceCosts.SourceItemQuantityCost > 0)
			{
				AddDetailNumberLine(
					Out, TEXT("Talent.Detail.ItemCost"), Action.ResourceCosts.SourceItemQuantityCost,
					EGridTalentDetailLineSemantic::ItemCost);
			}

			AddDetailTextLine(
				Out,
				TEXT("Talent.Detail.Target"),
				TargetingPolicyTextId(Action.TargetingPolicy),
				EGridTalentDetailLineSemantic::Target);

			if (Action.RangeCells > 0)
			{
				AddDetailNumberLine(
					Out, TEXT("Talent.Detail.RangeCells"), Action.RangeCells,
					EGridTalentDetailLineSemantic::Range);
			}
			if (Action.AreaRadiusCells > 0)
			{
				AddDetailNumberLine(
					Out, TEXT("Talent.Detail.AreaRadiusCells"), Action.AreaRadiusCells,
					EGridTalentDetailLineSemantic::Area);
			}
			if (Action.bRequiresLineOfSight)
			{
				AddDetailTextLine(
					Out, TEXT("Talent.Detail.LineOfSight"), TEXT("Talent.Value.Required"),
					EGridTalentDetailLineSemantic::LineOfSight);
			}
			if (Action.ResolutionCount > 1)
			{
				AddDetailNumberLine(
					Out, TEXT("Talent.Detail.ResolutionCount"), Action.ResolutionCount,
					EGridTalentDetailLineSemantic::ResolutionCount);
				if (Action.SubsequentResolutionAccuracyModifier != 0)
				{
					AddSignedValueLine(
						Out,
						TEXT("Talent.Detail.SubsequentResolutionAccuracy"),
						Action.SubsequentResolutionAccuracyModifier);
				}
			}
			if (Action.CooldownRounds > 0)
			{
				AddDetailNumberLine(
					Out, TEXT("Talent.Detail.CooldownRounds"), Action.CooldownRounds,
					EGridTalentDetailLineSemantic::Cooldown);
			}
		}
	}
	FText ResolveStatusReferences(const FText& Source)
	{
		FString Text = Source.ToString();
		int32 SearchFrom = 0;
		while (SearchFrom < Text.Len())
		{
			const int32 Start = Text.Find(TEXT("Status_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
			if (Start == INDEX_NONE)
			{
				break;
			}
			int32 End = Start;
			while (End < Text.Len() && (FChar::IsAlnum(Text[End]) || Text[End] == TCHAR('_')))
			{
				++End;
			}
			const FName EffectId(*Text.Mid(Start, End - Start));
			UGridStatusEffectDefinitionAsset* Definition =
				FGridStatusEffectPersistence::ResolveDefinitionByEffectId(EffectId);
			if (!IsValid(Definition) || Definition->DisplayName.IsEmpty())
			{
				SearchFrom = End;
				continue;
			}
			const FString Replacement = Definition->DisplayName.ToString();
			Text = Text.Left(Start) + Replacement + Text.Mid(End);
			SearchFrom = Start + Replacement.Len();
		}
		return FText::FromString(Text);
	}

	void AddStatusApplicationEffect(
		const FGridCombatStatusApplicationProfile& Profile,
		TArray<FGridTalentDetailLineView>& Out)
	{
		UGridStatusEffectDefinitionAsset* Definition =
			FGridStatusEffectPersistence::ResolveDefinitionByEffectId(Profile.StatusEffectId);
		if (!IsValid(Definition) || Definition->DisplayName.IsEmpty())
		{
			return;
		}

		FName LabelTextId = TEXT("Talent.Detail.Status");
		if (Profile.ArmorGate == EGridCombatStatusArmorGate::PhysicalArmorDepleted)
		{
			LabelTextId = TEXT("Talent.Detail.Status.PhysicalArmorDepleted");
		}
		else if (Profile.ArmorGate == EGridCombatStatusArmorGate::MagicalArmorDepleted)
		{
			LabelTextId = TEXT("Talent.Detail.Status.MagicalArmorDepleted");
		}
		else if (Profile.Trigger == EGridCombatStatusApplicationTrigger::AfterSuccessfulHit)
		{
			LabelTextId = TEXT("Talent.Detail.Status.AfterSuccessfulHit");
		}

		AddDetailLine(Out, LabelTextId, Definition->DisplayName.ToString());
		AddStatusDurationLine(
			Out,
			*Definition,
			Profile.DurationOverride,
			TEXT("Talent.Detail.StatusDurationTurns"),
			TEXT("Talent.Detail.StatusDurationRounds"),
			TEXT("Talent.Detail.StatusDuration"));

		if (Profile.InitialStackCount > 1)
		{
			AddDetailNumberLine(
				Out, TEXT("Talent.Detail.StatusCharges"), Profile.InitialStackCount);
		}
		if (!Definition->Description.IsEmpty())
		{
			AddDetailLine(
				Out, TEXT("Talent.Detail.StatusDescription"), Definition->Description.ToString());
		}
	}

	void BuildStructuredEffects(
		const URPGClassAsset& ClassDefinition,
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<FGridTalentDetailLineView>& Out)
	{
		Out.Reset();
		BuildStructuredCombatModifierEffects(Choice, Out);
		BuildStructuredReactionEffects(Choice, Out);
		BuildStructuredSkillPartyEffects(Choice, Out);

		TArray<const FGridCombatActionDefinition*> Actions;
		CollectUnlockedActions(ClassDefinition, Choice, Actions);
		for (const FGridCombatActionDefinition* ActionPtr : Actions)
		{
			if (!ActionPtr) continue;
			for (const FGridCombatStatusApplicationProfile& Profile : ActionPtr->StatusApplications)
			{
				AddStatusApplicationEffect(Profile, Out);
			}
			if (ActionPtr->bAffectsAlliesInArea)
			{
				AddDetailTextLine(Out, TEXT("Talent.Detail.Area"), TEXT("Talent.Value.AreaCanAffectAllies"));
			}
		}
	}

	bool ResolveConceptPresentation(
		FName ClassId, FName BranchId, FName NodeId, FText& OutName, FText& OutPrinciple)
	{
		OutName = FText::GetEmpty();
		OutPrinciple = FText::GetEmpty();
		const URPGTalentPresentationAsset* Catalog = LoadObject<URPGTalentPresentationAsset>(
			nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.DA_RPGTalentPresentation"));
		if (!Catalog) return false;
		const FRPGClassPresentationDefinition* ClassPresentation = Catalog->FindClass(ClassId);
		if (!ClassPresentation) return false;
		const FRPGTalentBranchPresentationDefinition* Branch = ClassPresentation->Branches.FindByPredicate(
			[BranchId](const FRPGTalentBranchPresentationDefinition& Candidate)
			{
				return Candidate.TalentBranchId == BranchId;
			});
		if (!Branch) return false;
		const FRPGTalentNodePresentationDefinition* Override = Branch->FindNodeOverride(NodeId);
		if (!Override) return false;
		OutName = Override->DisplayName;
		OutPrinciple = Override->Description;
		return !OutName.IsEmpty();
	}

	bool BuildCommonSatisfiedIds(const FChoiceArray& Choices, TSet<FName>& OutCommonIds)
	{
		OutCommonIds.Reset();
		if (Choices.IsEmpty() || !Choices[0]) return false;
		BuildSatisfiedIdsForChoice(*Choices[0], OutCommonIds);
		for (int32 Index = 1; Index < Choices.Num(); ++Index)
		{
			if (!Choices[Index]) return false;
			TSet<FName> CurrentIds;
			BuildSatisfiedIdsForChoice(*Choices[Index], CurrentIds);
			TArray<FName> ToRemove;
			for (const FName ExistingId : OutCommonIds)
			{
				if (!CurrentIds.Contains(ExistingId)) ToRemove.Add(ExistingId);
			}
			for (const FName Id : ToRemove) OutCommonIds.Remove(Id);
		}
		return !OutCommonIds.IsEmpty();
	}

	bool NodeDependsOnPrevious(const FChoiceArray& CurrentChoices, const FChoiceArray& PreviousChoices)
	{
		TSet<FName> CommonPreviousIds;
		if (!BuildCommonSatisfiedIds(PreviousChoices, CommonPreviousIds)) return false;
		for (const FRPGClassProgressionChoiceDefinition* Choice : CurrentChoices)
		{
			if (!Choice) return false;
			bool bDependsOnPrevious = false;
			for (const FName PrerequisiteId : Choice->PrerequisiteChoiceIds)
			{
				if (CommonPreviousIds.Contains(PrerequisiteId)) { bDependsOnPrevious = true; break; }
			}
			if (!bDependsOnPrevious)
			{
				for (const FName RequirementId : Choice->PrerequisiteRequirementIds)
				{
					if (CommonPreviousIds.Contains(RequirementId)) { bDependsOnPrevious = true; break; }
				}
			}
			if (!bDependsOnPrevious) return false;
		}
		return true;
	}

	bool BuildTalentNode(const URPGClassAsset& ClassDefinition, int32 CharacterLevel, const TSet<FName>& SelectedChoiceIds,
		FName BranchId, FName NodeId, FChoiceArray Choices, FGridTalentNodeView& OutNode)
	{
		OutNode = FGridTalentNodeView();
		if (Choices.IsEmpty() || BranchId.IsNone() || NodeId.IsNone()) return false;
		Choices.Sort(
			[](const FRPGClassProgressionChoiceDefinition& Left, const FRPGClassProgressionChoiceDefinition& Right)
			{
				return Left.ChoiceId.ToString().Compare(Right.ChoiceId.ToString(), ESearchCase::CaseSensitive) < 0;
			});
		const FRPGClassProgressionChoiceDefinition* First = Choices[0];
		if (!First) return false;
		const int32 Tier = ResolveTalentTier(First->MinimumLevel);
		if (Tier == 0) return false;

		const bool bVariantNode = Choices.Num() > 1;
		const FName VariantGroup = First->ExclusiveChoiceGroupId;
		if (!bVariantNode && First->ChoiceId != NodeId) return false;
		if (bVariantNode && VariantGroup.IsNone()) return false;

		for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
		{
			if (!Choice || Choice->TalentBranchId != BranchId || Choice->TalentNodeId != NodeId ||
				Choice->MinimumLevel != First->MinimumLevel || Choice->PointCost != First->PointCost) return false;
			if (bVariantNode &&
				(Choice->ExclusiveChoiceGroupId != VariantGroup || !Choice->GrantedRequirementIds.Contains(NodeId) ||
					!AreNameArraysEquivalent(Choice->PrerequisiteChoiceIds, First->PrerequisiteChoiceIds) ||
					!AreNameArraysEquivalent(Choice->PrerequisiteRequirementIds, First->PrerequisiteRequirementIds))) return false;
		}

		OutNode.TalentNodeId = NodeId;
		OutNode.TalentBranchId = BranchId;
		OutNode.Tier = Tier;
		OutNode.MinimumLevel = First->MinimumLevel;
		OutNode.PointCost = First->PointCost;
		OutNode.bHasExclusiveVariants = bVariantNode;
		OutNode.Variants.Reserve(Choices.Num());

		int32 SelectedVariantCount = 0;
		bool bHaveUnselectedState = false;
		EGridTalentNodeState CommonUnselectedState = EGridTalentNodeState::LockedPrerequisite;
		for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
		{
			const bool bSelected = SelectedChoiceIds.Contains(Choice->ChoiceId);
			const ERPGClassProgressionChoiceAvailabilityReason Availability =
				FRPGClassProgressionService::GetChoiceAvailability(&ClassDefinition, CharacterLevel, SelectedChoiceIds, Choice->ChoiceId);
			FGridTalentVariantView Variant;
			Variant.ChoiceId = Choice->ChoiceId;
			Variant.DisplayName = Choice->DisplayName;
			Variant.Principle = ResolveStatusReferences(Choice->Description);
			Variant.Type = Choice->PresentationType;
			BuildStructuredEffects(ClassDefinition, *Choice, Variant.Effects);
			BuildStructuredUsage(ClassDefinition, *Choice, Variant.Usage);
			Variant.bAcquired = bSelected;
			if (!TryMapChoiceState(bSelected, Availability, Variant.State)) return false;
			Variant.bCanChoose =
				bVariantNode && Variant.State == EGridTalentNodeState::Available && !Variant.bAcquired;


			OutNode.Variants.Add(MoveTemp(Variant));

			if (bSelected)
			{
				++SelectedVariantCount;
				OutNode.SelectedChoiceId = Choice->ChoiceId;
			}
			else if (!bHaveUnselectedState)
			{
				CommonUnselectedState = OutNode.Variants.Last().State;
				bHaveUnselectedState = true;
			}
			else if (SelectedVariantCount == 0 && CommonUnselectedState != OutNode.Variants.Last().State)
			{
				return false;
			}
		}
		if (SelectedVariantCount > 1) return false;
		OutNode.State = SelectedVariantCount == 1 ? EGridTalentNodeState::Acquired : CommonUnselectedState;

		OutNode.Type = OutNode.Variants[0].Type;
		for (const FGridTalentVariantView& Variant : OutNode.Variants)
		{
			if (Variant.Type != OutNode.Type) return false;
		}
		BuildAcquisitionView(ClassDefinition, *First, bVariantNode, OutNode.Acquisition);

		if (bVariantNode)
		{
			FText OverrideName;
			FText OverridePrinciple;
			if (!ResolveConceptPresentation(
				ClassDefinition.ClassId, BranchId, NodeId, OverrideName, OverridePrinciple) ||
				OverrideName.IsEmpty())
			{
				return false;
			}
			OutNode.DisplayName = OverrideName;
			OutNode.Principle = ResolveStatusReferences(OverridePrinciple);
		}
		else
		{
			if (First->DisplayName.IsEmpty())
			{
				return false;
			}
			OutNode.DisplayName = First->DisplayName;
			OutNode.Principle = ResolveStatusReferences(First->Description);
			OutNode.Effects = OutNode.Variants[0].Effects;
			OutNode.Usage = OutNode.Variants[0].Usage;
			OutNode.SimpleChoiceId = First->ChoiceId;
			OutNode.bCanAcquireSimple =
				OutNode.State == EGridTalentNodeState::Available && !SelectedChoiceIds.Contains(First->ChoiceId);
			// Simple Talents are not one-option variant nodes. Keep Variants reserved for
			// the four true exclusive-choice concepts.
			OutNode.Variants.Reset();
		}
		return SelectedVariantCount == 1 || bHaveUnselectedState;
	}

	bool BuildTalentTree(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex,
		const FGridCharacterInventoryState& Character, URPGClassAsset& ClassDefinition, FGridTalentTreeView& OutTree)
	{
		OutTree = FGridTalentTreeView();
		OutTree.ClassId = ClassDefinition.ClassId;
		if (ClassDefinition.ProgressionChoices.IsEmpty()) return true;

		bool bAnyStructuralMetadata = false;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition.ProgressionChoices)
		{
			const bool bHasBranch = !Choice.TalentBranchId.IsNone();
			const bool bHasNode = !Choice.TalentNodeId.IsNone();
			if (bHasBranch != bHasNode) return false;
			bAnyStructuralMetadata |= bHasBranch;
		}
		if (!bAnyStructuralMetadata) return true;

		TSet<FName> SelectedChoiceIds;
		if (!BuildSelectedChoiceSet(PartyInventoryComponent, CharacterIndex, SelectedChoiceIds)) return false;

		TMap<FName, TMap<FName, FChoiceArray>> ChoicesByBranch;
		TMap<FName, FName> BranchByNode;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition.ProgressionChoices)
		{
			if (Choice.TalentBranchId.IsNone() || Choice.TalentNodeId.IsNone() || ResolveTalentTier(Choice.MinimumLevel) == 0) return false;
			if (const FName* ExistingBranch = BranchByNode.Find(Choice.TalentNodeId); ExistingBranch && *ExistingBranch != Choice.TalentBranchId) return false;
			BranchByNode.FindOrAdd(Choice.TalentNodeId) = Choice.TalentBranchId;
			ChoicesByBranch.FindOrAdd(Choice.TalentBranchId).FindOrAdd(Choice.TalentNodeId).Add(&Choice);
		}

		TArray<FName> BranchIds;
		ChoicesByBranch.GetKeys(BranchIds);
		BranchIds.Sort([](const FName Left, const FName Right)
		{
			return Left.ToString().Compare(Right.ToString(), ESearchCase::CaseSensitive) < 0;
		});

		for (const FName BranchId : BranchIds)
		{
			const TMap<FName, FChoiceArray>* NodeGroups = ChoicesByBranch.Find(BranchId);
			if (!NodeGroups) return false;
			FGridTalentBranchView Branch;
			Branch.TalentBranchId = BranchId;
			TArray<FName> NodeIds;
			NodeGroups->GetKeys(NodeIds);
			for (const FName NodeId : NodeIds)
			{
				const FChoiceArray* NodeChoices = NodeGroups->Find(NodeId);
				FGridTalentNodeView Node;
				if (!NodeChoices || !BuildTalentNode(ClassDefinition, Character.Level, SelectedChoiceIds, BranchId, NodeId, *NodeChoices, Node)) return false;
				Branch.Nodes.Add(MoveTemp(Node));
			}
			Branch.Nodes.Sort([](const FGridTalentNodeView& Left, const FGridTalentNodeView& Right)
			{
				if (Left.MinimumLevel != Right.MinimumLevel) return Left.MinimumLevel < Right.MinimumLevel;
				return Left.TalentNodeId.ToString().Compare(Right.TalentNodeId.ToString(), ESearchCase::CaseSensitive) < 0;
			});
			for (int32 NodeIndex = 0; NodeIndex < Branch.Nodes.Num(); ++NodeIndex)
			{
				FGridTalentNodeView& Node = Branch.Nodes[NodeIndex];
				if (NodeIndex > 0)
				{
					const FGridTalentNodeView& Previous = Branch.Nodes[NodeIndex - 1];
					if (Previous.MinimumLevel == Node.MinimumLevel) return false;
					const FChoiceArray* CurrentChoices = NodeGroups->Find(Node.TalentNodeId);
					const FChoiceArray* PreviousChoices = NodeGroups->Find(Previous.TalentNodeId);
					if (!CurrentChoices || !PreviousChoices || !NodeDependsOnPrevious(*CurrentChoices, *PreviousChoices)) return false;
					if (Previous.DisplayName.IsEmpty()) return false;
					Node.PreviousNodeId = Previous.TalentNodeId;
					Node.PreviousNodeDisplayName = Previous.DisplayName;
				}
				if (Node.State == EGridTalentNodeState::Acquired) ++Branch.AcquiredNodeCount;
			}
			OutTree.Branches.Add(MoveTemp(Branch));
		}
		return true;
	}
}

bool FGridSkillsPageService::TryBuildCharacterView(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex,
	const TArray<const URPGSkillAsset*>& SkillDefinitions, FGridSkillsPageView& OutView)
{
	OutView = FGridSkillsPageView();
	if (!IsValid(PartyInventoryComponent) || !PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex)) return false;

	const FGridCharacterInventoryState& Character = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	URPGClassAsset* ClassDefinition = ResolveClassDefinition(Character);
	if (!Character.CharacterId.IsValid() || !ClassDefinition || !FRPGSkillService::ValidateSkillState(Character)) return false;

	TArray<const URPGSkillAsset*> SortedDefinitions;
	if (!ValidateAndSortDefinitions(SkillDefinitions, SortedDefinitions)) return false;

	TSet<FName> DefinitionSkillIds;
	for (const URPGSkillAsset* Definition : SortedDefinitions) DefinitionSkillIds.Add(Definition->SkillId);
	for (const FRPGSkillRank& SkillRank : Character.SkillRanks)
	{
		if (!DefinitionSkillIds.Contains(SkillRank.SkillId)) return false;
	}

	FGridSkillsPageView Candidate;
	Candidate.CharacterIndex = CharacterIndex;
	Candidate.CharacterId = Character.CharacterId;
	Candidate.CharacterName = Character.DisplayName;
	Candidate.CharacterLevel = Character.Level;
	Candidate.ClassId = ClassDefinition->ClassId;
	Candidate.ClassDisplayName = ClassDefinition->DisplayName;

	FRPGSkillPointBalance SkillPointBalance;
	if (!FRPGSkillPointService::TryGetBalance(Character, SkillPointBalance)) return false;
	Candidate.GrantedSkillPoints = SkillPointBalance.GrantedPoints;
	Candidate.SpentSkillPoints = SkillPointBalance.SpentPoints;
	Candidate.RemainingSkillPoints = SkillPointBalance.RemainingPoints;
	Candidate.SkillRankCap = SkillPointBalance.RankCap;

	Candidate.Skills.Reserve(SortedDefinitions.Num());
	for (const URPGSkillAsset* Definition : SortedDefinitions)
	{
		const int32 Rank = FRPGSkillService::GetSkillRank(Character, Definition->SkillId);
		if (Rank < 0 || Rank > Definition->MaxRank) return false;
		FGridSkillEntryView SkillView;
		SkillView.SkillId = Definition->SkillId;
		SkillView.DisplayName = Definition->DisplayName;
		SkillView.Description = Definition->Description;
		SkillView.GoverningAttribute = Definition->GoverningAttribute;
		SkillView.Rank = Rank;
		SkillView.MaxRank = Definition->MaxRank;
		SkillView.bAllowUntrainedChecks = Definition->bAllowUntrainedChecks;
		SkillView.bTrained = Rank > 0;
		SkillView.CurrentRankCap = FMath::Min(Definition->MaxRank, SkillPointBalance.RankCap);
		SkillView.bCanIncreaseRank =
			FRPGSkillPointService::GetNextRankPurchaseAvailability(Character, Definition) ==
			ERPGSkillPointMutationRejectReason::None;
		Candidate.Skills.Add(MoveTemp(SkillView));
	}

	if (!FRPGClassProgressionTransactionService::TryGetChoicePointBalance(
			PartyInventoryComponent,
			CharacterIndex,
			Candidate.GrantedTalentPoints,
			Candidate.SpentTalentPoints,
			Candidate.RemainingTalentPoints))
	{
		return false;
	}

	if (!BuildTalentTree(PartyInventoryComponent, CharacterIndex, Character, *ClassDefinition, Candidate.TalentTree)) return false;
	OutView = MoveTemp(Candidate);
	return true;
}

bool FGridSkillsPageService::TryBuildSelectedCharacterView(
	UGridPartyInventoryComponent* PartyInventoryComponent, const TArray<const URPGSkillAsset*>& SkillDefinitions, FGridSkillsPageView& OutView)
{
	OutView = FGridSkillsPageView();
	if (!IsValid(PartyInventoryComponent)) return false;
	return TryBuildCharacterView(PartyInventoryComponent, PartyInventoryComponent->GetSelectedCharacterIndex(), SkillDefinitions, OutView);
}

void FGridSkillsPageService::ResolveCanonicalSkillDefinitions(TArray<const URPGSkillAsset*>& OutDefinitions)
{
	OutDefinitions.Reset();
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FString> SearchPaths;
	SearchPaths.Add(TEXT("/Game"));
	AssetManager.ScanPathsForPrimaryAssets(RPGSkillPrimaryAssetType, SearchPaths, URPGSkillAsset::StaticClass(), false, false, true);

	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(RPGSkillPrimaryAssetType, AssetIds);
	AssetIds.Sort([](const FPrimaryAssetId& Left, const FPrimaryAssetId& Right)
	{
		return Left.PrimaryAssetName.ToString().Compare(Right.PrimaryAssetName.ToString(), ESearchCase::CaseSensitive) < 0;
	});

	TSet<FName> SeenSkillIds;
	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		URPGSkillAsset* Definition = AssetManager.GetPrimaryAssetObject<URPGSkillAsset>(AssetId);
		if (!IsValid(Definition))
		{
			const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
			if (AssetPath.IsValid()) Definition = Cast<URPGSkillAsset>(AssetPath.TryLoad());
		}
		if (!IsValid(Definition) || !Definition->IsValidDefinition() || Definition->GetPrimaryAssetId() != AssetId || SeenSkillIds.Contains(Definition->SkillId)) continue;
		SeenSkillIds.Add(Definition->SkillId);
		OutDefinitions.Add(Definition);
	}
}
