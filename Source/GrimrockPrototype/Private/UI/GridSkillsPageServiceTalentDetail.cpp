#include "UI/GridSkillsPageService.h"

#include "UI/RPGTalentPresentationAsset.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "Runtime/Monsters/GridMonsterCategoryAsset.h"

#include "Engine/AssetManager.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGRecipeCatalogAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillService.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Runtime/GridPartyInventoryComponent.h"

#include "GridSkillsPageServiceInternal.h"

namespace GridSkillsPageServicePrivate
{
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
				AddDetailTextLine(Lines, PermanentLabelTextId, TEXT("Status.Duration.Permanent"));
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

	bool IsRecipeId(FName RecipeId)
	{
		return !RecipeId.IsNone() &&
			RecipeId.ToString().StartsWith(TEXT("Recipe_"), ESearchCase::CaseSensitive);
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
			if (!IsRecipeId(Id))
			{
				continue;
			}

			if (Out.GrantedRecipes.ContainsByPredicate(
				[Id](const FGridRecipeDisplayView& Existing)
				{
					return Existing.RecipeId == Id;
				}))
			{
				continue;
			}

			FGridRecipeDisplayView Recipe;
			Recipe.RecipeId = Id;
			Recipe.DisplayName = URPGRecipeCatalogAsset::ResolveProductionDisplayName(Id);
			Out.GrantedRecipes.Add(MoveTemp(Recipe));
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
		const URPGTalentPresentationAsset* Catalog = URPGTalentPresentationAsset::ResolveCanonical();
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

}
