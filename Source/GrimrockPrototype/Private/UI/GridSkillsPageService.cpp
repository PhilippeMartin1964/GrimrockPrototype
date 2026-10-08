#include "UI/GridSkillsPageService.h"

#include "UI/RPGTalentPresentationAsset.h"

#include "Engine/AssetManager.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGSkillService.h"
#include "RPG/RPGTalentRuntimeService.h"
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

	FString HumanizeId(FName Id)
	{
		FString Value = Id.ToString();
		for (const TCHAR* Prefix : { TEXT("Status_"), TEXT("Skill_"), TEXT("Recipe_"), TEXT("Action_"), TEXT("Talent_") })
		{
			if (Value.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				Value.RightChopInline(FCString::Strlen(Prefix), EAllowShrinking::No);
				break;
			}
		}
		Value.ReplaceInline(TEXT("_"), TEXT(" "));
		Value.ReplaceInline(TEXT("."), TEXT(" "));
		return Value;
	}

	FString DamageTypeLabel(EGridDamageType Type)
	{
		switch (Type)
		{
			case EGridDamageType::Physical: return TEXT("physiques");
			case EGridDamageType::Fire: return TEXT("de feu");
			case EGridDamageType::Ice: return TEXT("de glace");
			case EGridDamageType::Lightning: return TEXT("de foudre");
			case EGridDamageType::Poison: return TEXT("de poison");
			case EGridDamageType::Holy: return TEXT("sacrés");
			case EGridDamageType::Necrotic: return TEXT("nécrotiques");
			case EGridDamageType::Arcane: return TEXT("arcaniques");
			default: return TEXT("inconnus");
		}
	}

	FString PhysicalSubtypeLabel(EGridPhysicalDamageSubtype Type)
	{
		switch (Type)
		{
			case EGridPhysicalDamageSubtype::Slashing: return TEXT("tranchantes");
			case EGridPhysicalDamageSubtype::Piercing: return TEXT("perforantes");
			case EGridPhysicalDamageSubtype::Bludgeoning: return TEXT("contondantes");
			default: return TEXT("physiques");
		}
	}

	FString SourcePolicyLabel(EGridCombatActionSourcePolicy Policy)
	{
		switch (Policy)
		{
			case EGridCombatActionSourcePolicy::Universal: return TEXT("actions universelles");
			case EGridCombatActionSourcePolicy::Equipment: return TEXT("actions d'équipement");
			case EGridCombatActionSourcePolicy::Ability: return TEXT("capacités");
			case EGridCombatActionSourcePolicy::Spell: return TEXT("sorts");
			case EGridCombatActionSourcePolicy::QuickItem: return TEXT("objets rapides");
			default: return TEXT("actions");
		}
	}

	FString TargetConditionLabel(EGridCombatTargetCondition Condition)
	{
		switch (Condition)
		{
			case EGridCombatTargetCondition::RearArc: return TEXT("cible attaquée par l'arrière");
			case EGridCombatTargetCondition::HasActedThisRound: return TEXT("cible ayant déjà agi ce round");
			case EGridCombatTargetCondition::HasNotActedThisRound: return TEXT("cible n'ayant pas encore agi ce round");
			case EGridCombatTargetCondition::PhysicalControl: return TEXT("cible sous contrôle physique");
			default: return FString();
		}
	}

	FString ReactionTriggerLabel(EGridCombatReactionTrigger Trigger)
	{
		switch (Trigger)
		{
			case EGridCombatReactionTrigger::ActionResolved: return TEXT("après résolution d'une action");
			case EGridCombatReactionTrigger::AttackHit: return TEXT("après une attaque réussie");
			case EGridCombatReactionTrigger::AttackMiss: return TEXT("après une attaque manquée");
			case EGridCombatReactionTrigger::TargetDefeated: return TEXT("lorsqu'une cible est vaincue");
			case EGridCombatReactionTrigger::DirectDamageReceived: return TEXT("après avoir subi des dégâts directs");
			case EGridCombatReactionTrigger::SurfaceReaction: return TEXT("lors d'une réaction de surface");
			case EGridCombatReactionTrigger::IncomingAttackHit: return TEXT("lorsqu'une attaque entrante touche");
			case EGridCombatReactionTrigger::OwnedStatusTargetDefeated: return TEXT("lorsqu'une cible affectée par votre statut est vaincue");
			default: return TEXT("réaction");
		}
	}

	FString ReactionLimitLabel(EGridCombatReactionLimit Limit)
	{
		switch (Limit)
		{
			case EGridCombatReactionLimit::OncePerRound: return TEXT("une fois par round");
			case EGridCombatReactionLimit::OncePerAction: return TEXT("une fois par action");
			default: return TEXT("sans limite spéciale");
		}
	}

	void AppendSignedPercent(TArray<FString>& Out, const TCHAR* Label, int32 Value)
	{
		if (Value != 0) Out.Add(FString::Printf(TEXT("%s : %+d %%"), Label, Value));
	}

	void AppendSignedValue(TArray<FString>& Out, const TCHAR* Label, int32 Value)
	{
		if (Value != 0) Out.Add(FString::Printf(TEXT("%s : %+d"), Label, Value));
	}

	FString BuildModifierContext(const FGridCombatModifierProfile& Modifier)
	{
		TArray<FString> Parts;
		for (const EGridCombatActionSourcePolicy Policy : Modifier.SourcePolicies) Parts.Add(SourcePolicyLabel(Policy));
		for (const EGridPhysicalDamageSubtype Subtype : Modifier.PhysicalSubtypes) Parts.Add(TEXT("armes ") + PhysicalSubtypeLabel(Subtype));
		for (const EGridDamageType Type : Modifier.DamageTypes) Parts.Add(TEXT("dégâts ") + DamageTypeLabel(Type));
		for (const EGridCombatTargetCondition Condition : Modifier.RequiredTargetConditions)
		{
			const FString Label = TargetConditionLabel(Condition);
			if (!Label.IsEmpty()) Parts.Add(Label);
		}
		for (const EGridCombatTargetCondition Condition : Modifier.AnyTargetConditions)
		{
			const FString Label = TargetConditionLabel(Condition);
			if (!Label.IsEmpty()) Parts.Add(Label);
		}
		for (const FName StatusId : Modifier.RequiredTargetStatusEffectIds) Parts.Add(TEXT("cible avec ") + HumanizeId(StatusId));
		for (const FName CategoryId : Modifier.AllowedTargetMonsterCategoryIds) Parts.Add(TEXT("cible : ") + HumanizeId(CategoryId));
		for (const FName Tag : Modifier.RequiredSourceTags) Parts.Add(HumanizeId(Tag));
		if (Modifier.bRequirePartyStationarySincePreviousActivation) Parts.Add(TEXT("si le groupe n'a pas bougé depuis l'activation précédente"));
		if (Modifier.bExcludeAreaActions) Parts.Add(TEXT("hors actions de zone"));
		return FString::Join(Parts, TEXT(", "));
	}

	FText BuildMechanicsSummary(const FRPGClassProgressionChoiceDefinition& Choice)
	{
		TArray<FString> Lines;

		for (const FGridCombatModifierProfile& Modifier : Choice.CombatModifiers)
		{
			TArray<FString> Effects;
			AppendSignedValue(Effects, TEXT("Précision"), Modifier.AccuracyModifier);
			AppendSignedValue(Effects, TEXT("Esquive"), Modifier.EvasionModifier);
			AppendSignedPercent(Effects, TEXT("Dégâts infligés"), Modifier.OutgoingDamagePercentModifier);
			AppendSignedPercent(Effects, TEXT("Dégâts reçus"), Modifier.IncomingDamagePercentModifier);
			AppendSignedValue(Effects, TEXT("Chance de critique (points)"), Modifier.CriticalChancePercentModifier);
			AppendSignedValue(Effects, TEXT("Dégâts critiques (points)"), Modifier.CriticalDamagePercentModifier);
			AppendSignedPercent(Effects, TEXT("Dégâts de l'arme"), Modifier.WeaponDamagePercentModifier);
			AppendSignedValue(Effects, TEXT("Coût en points d'action"), Modifier.ActionPointCostModifier);
			AppendSignedValue(Effects, TEXT("Coût en mana"), Modifier.ManaCostModifier);
			AppendSignedValue(Effects, TEXT("Portée (cases)"), Modifier.RangeCellsModifier);
			AppendSignedPercent(Effects, TEXT("Effets positifs"), Modifier.PositiveEffectPercentModifier);
			AppendSignedPercent(Effects, TEXT("Soins prodigués"), Modifier.OutgoingHealingPercentModifier);
			AppendSignedPercent(Effects, TEXT("Dégâts directs aux alliés"), Modifier.FriendlyDirectDamagePercentModifier);
			AppendSignedPercent(Effects, TEXT("Dégâts directs sur soi"), Modifier.SelfDirectDamagePercentModifier);
			AppendSignedValue(Effects, TEXT("Cibles secondaires d'objet rapide"), Modifier.QuickItemSecondaryTargetCount);
			AppendSignedPercent(Effects, TEXT("Magnitude secondaire"), Modifier.QuickItemSecondaryMagnitudePercent);
			AppendSignedPercent(Effects, TEXT("Durée secondaire"), Modifier.QuickItemSecondaryDurationPercent);
			AppendSignedPercent(Effects, TEXT("Armure physique de référence"), Modifier.PhysicalArmorReferencePercentModifier);
			AppendSignedPercent(Effects, TEXT("Armure magique de référence"), Modifier.MagicalArmorReferencePercentModifier);
			AppendSignedPercent(Effects, TEXT("Restauration d'armure physique"), Modifier.PhysicalArmorRestorationPercentModifier);
			AppendSignedPercent(Effects, TEXT("Restauration d'armure magique"), Modifier.MagicalArmorRestorationPercentModifier);
			AppendSignedValue(Effects, TEXT("Durée des surfaces (rounds)"), Modifier.SurfaceDurationRoundsModifier);
			AppendSignedPercent(Effects, TEXT("Dégâts périodiques des surfaces"), Modifier.SurfacePeriodicDamagePercentModifier);
			AppendSignedPercent(Effects, TEXT("Dégâts des réactions de surface"), Modifier.SurfaceReactionDamagePercentModifier);
			AppendSignedValue(Effects, TEXT("Rayon des réactions de surface"), Modifier.SurfaceReactionAreaRadiusModifier);
			AppendSignedPercent(Effects, TEXT("Résistance physique"), Modifier.ResistanceModifiers.PhysicalResistance);
			AppendSignedPercent(Effects, TEXT("Résistance au feu"), Modifier.ResistanceModifiers.FireResistance);
			AppendSignedPercent(Effects, TEXT("Résistance à la glace"), Modifier.ResistanceModifiers.IceResistance);
			AppendSignedPercent(Effects, TEXT("Résistance à la foudre"), Modifier.ResistanceModifiers.LightningResistance);
			AppendSignedPercent(Effects, TEXT("Résistance au poison"), Modifier.ResistanceModifiers.PoisonResistance);
			AppendSignedPercent(Effects, TEXT("Résistance sacrée"), Modifier.ResistanceModifiers.HolyResistance);
			AppendSignedPercent(Effects, TEXT("Résistance nécrotique"), Modifier.ResistanceModifiers.NecroticResistance);
			AppendSignedPercent(Effects, TEXT("Résistance arcanique"), Modifier.ResistanceModifiers.ArcaneResistance);
			if (Modifier.MinimumManaCost > 0) Effects.Add(FString::Printf(TEXT("Coût minimum en mana : %d"), Modifier.MinimumManaCost));

			if (!Effects.IsEmpty())
			{
				const FString Context = BuildModifierContext(Modifier);
				Lines.Add(Context.IsEmpty()
					? TEXT("• ") + FString::Join(Effects, TEXT(" ; "))
					: TEXT("• ") + Context + TEXT(" → ") + FString::Join(Effects, TEXT(" ; ")));
			}
		}

		for (const FGridCombatReactionProfile& Reaction : Choice.CombatReactions)
		{
			TArray<FString> Response;
			if (Reaction.CounterAttackWeaponProfile.bUseEquippedWeapon)
			{
				Response.Add(FString::Printf(TEXT("contre-attaque à %d %% des dégâts de l'arme, portée %d"),
					Reaction.CounterAttackWeaponProfile.WeaponDamagePercent, Reaction.CounterAttackRangeCells));
			}
			if (Reaction.InterceptFinalDamagePercent > 0)
				Response.Add(FString::Printf(TEXT("redirige %d %% des dégâts finaux"), Reaction.InterceptFinalDamagePercent));
			AppendSignedPercent(Response, TEXT("dégâts de réaction de surface"), Reaction.SurfaceReactionDamagePercentModifier);
			AppendSignedValue(Response, TEXT("rayon de réaction de surface"), Reaction.SurfaceReactionAreaRadiusModifier);
			if (Reaction.SecondaryDirectDamage > 0)
				Response.Add(FString::Printf(TEXT("%d dégâts directs %s"), Reaction.SecondaryDirectDamage, *DamageTypeLabel(Reaction.SecondaryDirectDamageType)));
			if (!Reaction.ApplyOwnerStatusEffectId.IsNone())
			{
				FString Status = TEXT("applique ") + HumanizeId(Reaction.ApplyOwnerStatusEffectId);
				if (Reaction.ApplyOwnerStatusDurationOverride >= 0)
					Status += FString::Printf(TEXT(" pendant %d tour%s"), Reaction.ApplyOwnerStatusDurationOverride, Reaction.ApplyOwnerStatusDurationOverride > 1 ? TEXT("s") : TEXT(""));
				Response.Add(Status);
			}
			if (!Reaction.TransferOwnedTargetStatusEffectId.IsNone())
				Response.Add(TEXT("transfère ") + HumanizeId(Reaction.TransferOwnedTargetStatusEffectId) +
					FString::Printf(TEXT(" vers une cible à %d case%s"), Reaction.TransferTargetRangeCells, Reaction.TransferTargetRangeCells > 1 ? TEXT("s") : TEXT("")));
			if (Reaction.bConsumeOwningStatus) Response.Add(TEXT("consomme le statut déclencheur"));
			FString Line = TEXT("• ") + ReactionTriggerLabel(Reaction.Trigger) + TEXT(", ") + ReactionLimitLabel(Reaction.Limit);
			if (!Response.IsEmpty()) Line += TEXT(" → ") + FString::Join(Response, TEXT(" ; "));
			Lines.Add(Line);
		}

		for (const FRPGSkillProgressionModifier& Skill : Choice.SkillModifiers)
		{
			TArray<FString> Effects;
			AppendSignedValue(Effects, TEXT("jets"), Skill.CheckModifier);
			if (Skill.RequirementGrantRankModifier > 0) Effects.Add(FString::Printf(TEXT("rang effectif pour prérequis : +%d"), Skill.RequirementGrantRankModifier));
			if (Skill.SafeFailureMargin > 0) Effects.Add(FString::Printf(TEXT("échec sûr jusqu'à %d point%s"), Skill.SafeFailureMargin, Skill.SafeFailureMargin > 1 ? TEXT("s") : TEXT("")));
			if (Skill.bRequireRangedContext) Effects.Add(TEXT("uniquement à distance"));
			if (!Skill.RelatedMonsterCategoryIds.IsEmpty())
			{
				TArray<FString> Categories;
				for (const FName Id : Skill.RelatedMonsterCategoryIds) Categories.Add(HumanizeId(Id));
				Effects.Add(TEXT("contre : ") + FString::Join(Categories, TEXT(", ")));
			}
			Lines.Add(TEXT("• Compétence ") + HumanizeId(Skill.SkillId) + TEXT(" → ") + FString::Join(Effects, TEXT(" ; ")));
		}

		for (const FRPGPartyProgressionModifier& Party : Choice.PartyModifiers)
		{
			TArray<FString> Effects;
			if (Party.GroupSkillCheckModifier != 0)
			{
				TArray<FString> Skills;
				for (const FName Id : Party.GroupSkillIds) Skills.Add(HumanizeId(Id));
				Effects.Add(FString::Printf(TEXT("jets de groupe %+d (%s)"), Party.GroupSkillCheckModifier, *FString::Join(Skills, TEXT(", "))));
			}
			AppendSignedValue(Effects, TEXT("points d'action de mobilité maximum"), Party.MaximumMobilityActionPointsModifier);
			if (!Effects.IsEmpty()) Lines.Add(TEXT("• Groupe → ") + FString::Join(Effects, TEXT(" ; ")));
		}

		if (Choice.FirstRoundInitiativeModifier != 0)
			Lines.Add(FString::Printf(TEXT("• Initiative au premier round : %+d"), Choice.FirstRoundInitiativeModifier));

		for (const FName RequirementId : Choice.GrantedRequirementIds)
		{
			const FString Raw = RequirementId.ToString();
			if (Raw.StartsWith(TEXT("Recipe_"), ESearchCase::CaseSensitive))
				Lines.Add(TEXT("• Recette débloquée : ") + HumanizeId(RequirementId));
		}

		return Lines.IsEmpty()
			? FText::GetEmpty()
			: FText::FromString(FString::Join(Lines, TEXT("\n")));
	}

	FText TargetingSummary(EGridCombatTargetingPolicy Policy)
	{
		switch (Policy)
		{
			case EGridCombatTargetingPolicy::Self: return FText::FromString(TEXT("soi-même"));
			case EGridCombatTargetingPolicy::Ally: return FText::FromString(TEXT("soi-même ou un allié vivant"));
			case EGridCombatTargetingPolicy::FirstAxialTarget: return FText::FromString(TEXT("première cible dans l'axe"));
			case EGridCombatTargetingPolicy::Cell: return FText::FromString(TEXT("une cellule"));
			case EGridCombatTargetingPolicy::Area: return FText::FromString(TEXT("une zone"));
			case EGridCombatTargetingPolicy::Hostile: return FText::FromString(TEXT("un ennemi"));
			case EGridCombatTargetingPolicy::Party: return FText::FromString(TEXT("tous les membres vivants du groupe"));
			case EGridCombatTargetingPolicy::FrontRowParty: return FText::FromString(TEXT("membres vivants du rang avant"));
			case EGridCombatTargetingPolicy::AllyOrHostile: return FText::FromString(TEXT("un allié ou un ennemi"));
			default: return FText::GetEmpty();
		}
	}

	FText ConceptualNodeDisplayName(const FGridTalentNodeView& Node)
	{
		if (Node.Variants.IsEmpty()) return FText::GetEmpty();
		FString Label = Node.Variants[0].DisplayName.ToString();
		if (Node.Variants.Num() > 1)
		{
			for (const FString Separator : { FString(TEXT(" — ")), FString(TEXT(" – ")), FString(TEXT(" - ")), FString(TEXT(": ")) })
			{
				const int32 SeparatorIndex = Label.Find(Separator, ESearchCase::CaseSensitive);
				if (SeparatorIndex > 0)
				{
					Label = Label.Left(SeparatorIndex);
					break;
				}
			}
		}
		Label.TrimStartAndEndInline();
		return FText::FromString(Label);
	}

	void BuildUnlockedActionViews(
		const URPGClassAsset& ClassDefinition,
		const FRPGClassProgressionChoiceDefinition& Choice,
		TArray<FGridTalentUnlockedActionView>& OutActions)
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
			if (!bUnlockedByChoice)
			{
				continue;
			}

			FGridTalentUnlockedActionView ActionView;
			ActionView.ActionId = Action.ActionId;
			ActionView.SourcePolicy = Action.SourcePolicy;
			ActionView.DisplayName = Action.DisplayName;
			ActionView.Description = Action.Description;
			ActionView.ActionPointCost = Action.ActionPointCost;
			ActionView.ManaCost = Action.ResourceCosts.ManaCost;
			ActionView.RangeCells = Action.RangeCells;
			ActionView.CooldownRounds = Action.CooldownRounds;
			ActionView.SourceItemQuantityCost = Action.ResourceCosts.SourceItemQuantityCost;
			ActionView.TargetSummary = TargetingSummary(Action.TargetingPolicy);
			ActionView.AreaRadiusCells = Action.AreaRadiusCells;
			ActionView.MaximumResolvedTargets = Action.MaximumResolvedTargets;
			ActionView.ChainJumpRangeCells = Action.ChainJumpRangeCells;
			ActionView.ResolutionCount = Action.ResolutionCount;
			ActionView.SubsequentResolutionAccuracyModifier = Action.SubsequentResolutionAccuracyModifier;
			ActionView.bRequiresLineOfSight = Action.bRequiresLineOfSight;
			ActionView.bAreaCenteredOnParty = Action.bAreaCenteredOnParty;
			ActionView.bAffectsAlliesInArea = Action.bAffectsAlliesInArea;
			OutActions.Add(MoveTemp(ActionView));
		}

		OutActions.Sort(
			[](const FGridTalentUnlockedActionView& Left, const FGridTalentUnlockedActionView& Right)
			{
				return Left.ActionId.ToString().Compare(Right.ActionId.ToString(), ESearchCase::CaseSensitive) < 0;
			});
	}


	FText TalentTypeText(ERPGTalentPresentationType Type)
	{
		switch (Type)
		{
			case ERPGTalentPresentationType::Active: return FText::FromString(TEXT("ACTIF"));
			case ERPGTalentPresentationType::ActiveSpell: return FText::FromString(TEXT("SORT ACTIF"));
			case ERPGTalentPresentationType::Passive: return FText::FromString(TEXT("PASSIF"));
			case ERPGTalentPresentationType::AutomaticReaction: return FText::FromString(TEXT("RÉACTION AUTOMATIQUE"));
			case ERPGTalentPresentationType::RecipeQuickItem: return FText::FromString(TEXT("RECETTE + OBJET RAPIDE"));
			case ERPGTalentPresentationType::RecipeActive: return FText::FromString(TEXT("RECETTE + ACTIF"));
			default: return FText::GetEmpty();
		}
	}


	void AddDetailLine(TArray<FGridTalentDetailLineView>& Lines, const TCHAR* Label, const FString& Value)
	{
		if (Value.IsEmpty())
		{
			return;
		}
		FGridTalentDetailLineView Line;
		Line.Label = FText::FromString(Label);
		Line.Value = FText::FromString(Value);
		Lines.Add(MoveTemp(Line));
	}

	FString Plural(int32 Value, const TCHAR* Singular, const TCHAR* PluralForm)
	{
		return FString::Printf(TEXT("%d %s"), Value, Value > 1 ? PluralForm : Singular);
	}

	FText TalentStatusText(
		EGridTalentNodeState State, int32 MinimumLevel, int32 PointCost, const TArray<FText>& PrerequisiteNames)
	{
		switch (State)
		{
			case EGridTalentNodeState::Acquired:
				return FText::FromString(TEXT("ACQUIS"));
			case EGridTalentNodeState::Available:
				return FText::FromString(TEXT("DISPONIBLE"));
			case EGridTalentNodeState::LockedLevel:
				return FText::FromString(FString::Printf(TEXT("VERROUILLÉ — niveau %d requis"), MinimumLevel));
			case EGridTalentNodeState::LockedPrerequisite:
				if (!PrerequisiteNames.IsEmpty() && !PrerequisiteNames[0].IsEmpty())
				{
					return FText::Format(NSLOCTEXT("GridSkillsPage", "TalentLockedPrerequisite",
						"VERROUILLÉ — nécessite « {0} »"), PrerequisiteNames[0]);
				}
				return FText::FromString(TEXT("VERROUILLÉ — prérequis manquant"));
			case EGridTalentNodeState::LockedPoints:
				return FText::FromString(FString::Printf(
					TEXT("VERROUILLÉ — nécessite %d point%s de Talent"), PointCost, PointCost > 1 ? TEXT("s") : TEXT("")));
			case EGridTalentNodeState::LockedExclusive:
				return FText::FromString(TEXT("INDISPONIBLE — autre variante déjà choisie"));
			default:
				return FText::GetEmpty();
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
		if (bExclusiveVariant)
		{
			Out.ExclusivityText = FText::FromString(TEXT("Une seule variante peut être acquise."));
		}
	}

	void BuildStructuredUsage(
		const TArray<FGridTalentUnlockedActionView>& Actions,
		TArray<FGridTalentDetailLineView>& Out)
	{
		Out.Reset();
		for (const FGridTalentUnlockedActionView& Action : Actions)
		{
			if (Actions.Num() > 1 && !Action.DisplayName.IsEmpty())
			{
				AddDetailLine(Out, TEXT("Action"), Action.DisplayName.ToString());
			}
			if (Action.ActionPointCost > 0)
				AddDetailLine(Out, TEXT("Coût"), Plural(Action.ActionPointCost, TEXT("point d'action"), TEXT("points d'action")));
			if (Action.ManaCost > 0)
				AddDetailLine(Out, TEXT("Mana"), FString::FromInt(Action.ManaCost));
			if (Action.SourceItemQuantityCost > 0)
				AddDetailLine(Out, TEXT("Objet consommé"), Plural(Action.SourceItemQuantityCost, TEXT("objet"), TEXT("objets")));
			if (!Action.TargetSummary.IsEmpty())
				AddDetailLine(Out, TEXT("Cible"), Action.TargetSummary.ToString());
			if (Action.RangeCells > 0)
				AddDetailLine(Out, TEXT("Portée"), Plural(Action.RangeCells, TEXT("case"), TEXT("cases")));
			if (Action.AreaRadiusCells > 0)
				AddDetailLine(Out, TEXT("Zone"), FString::Printf(TEXT("rayon %s"), *Plural(Action.AreaRadiusCells, TEXT("case"), TEXT("cases"))));
			if (Action.bRequiresLineOfSight)
				AddDetailLine(Out, TEXT("Ligne de vue"), TEXT("requise"));
			if (Action.ResolutionCount > 1)
			{
				FString Value = Plural(Action.ResolutionCount, TEXT("résolution"), TEXT("résolutions"));
				if (Action.SubsequentResolutionAccuracyModifier != 0)
					Value += FString::Printf(TEXT(" ; Précision %+d à partir de la 2e"), Action.SubsequentResolutionAccuracyModifier);
				AddDetailLine(Out, TEXT("Résolutions"), Value);
			}
			if (Action.CooldownRounds > 0)
				AddDetailLine(Out, TEXT("Recharge"), Plural(Action.CooldownRounds, TEXT("round"), TEXT("rounds")));
		}
	}

	void BuildStructuredEffects(
		const FRPGClassProgressionChoiceDefinition& Choice,
		const TArray<FGridTalentUnlockedActionView>& Actions,
		TArray<FGridTalentDetailLineView>& Out)
	{
		Out.Reset();
		const FString Mechanics = BuildMechanicsSummary(Choice).ToString();
		TArray<FString> Lines;
		Mechanics.ParseIntoArrayLines(Lines, true);
		for (FString Line : Lines)
		{
			Line.RemoveFromStart(TEXT("• "));
			Line.TrimStartAndEndInline();
			AddDetailLine(Out, TEXT("Effet"), Line);
		}
		for (const FGridTalentUnlockedActionView& Action : Actions)
		{
			if (Action.bAffectsAlliesInArea)
				AddDetailLine(Out, TEXT("Zone"), TEXT("peut également affecter les alliés"));
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
			Variant.Description = Choice->Description;
			Variant.Principle = Choice->Description;
			BuildUnlockedActionViews(ClassDefinition, *Choice, Variant.UnlockedActions);
			Variant.Type = Choice->PresentationType;
			Variant.TypeText = TalentTypeText(Variant.Type);
			Variant.EffectCategory = Variant.TypeText; // DESC01.14.1 compatibility only.
			Variant.MechanicsSummary = BuildMechanicsSummary(*Choice); // compatibility only.
			BuildStructuredEffects(*Choice, Variant.UnlockedActions, Variant.Effects);
			BuildStructuredUsage(Variant.UnlockedActions, Variant.Usage);
			Variant.bSelected = bSelected;
			Variant.bAcquired = bSelected;
			if (!TryMapChoiceState(bSelected, Availability, Variant.State)) return false;
			Variant.bAvailable = Variant.State == EGridTalentNodeState::Available;
			Variant.bCanChoose = bVariantNode && Variant.bAvailable && !Variant.bAcquired;

			FGridTalentAcquisitionView VariantAcquisition;
			BuildAcquisitionView(ClassDefinition, *Choice, bVariantNode, VariantAcquisition);
			Variant.StatusText = TalentStatusText(
				Variant.State, Choice->MinimumLevel, Choice->PointCost, VariantAcquisition.PrerequisiteTalentNames);

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
		OutNode.TypeText = TalentTypeText(OutNode.Type);
		BuildAcquisitionView(ClassDefinition, *First, bVariantNode, OutNode.Acquisition);
		OutNode.StatusText = TalentStatusText(
			OutNode.State, First->MinimumLevel, First->PointCost, OutNode.Acquisition.PrerequisiteTalentNames);

		if (bVariantNode)
		{
			FText OverrideName;
			FText OverridePrinciple;
			if (ResolveConceptPresentation(ClassDefinition.ClassId, BranchId, NodeId, OverrideName, OverridePrinciple))
			{
				OutNode.DisplayName = OverrideName;
				OutNode.Principle = OverridePrinciple;
			}
			else
			{
				OutNode.DisplayName = ConceptualNodeDisplayName(OutNode);
			}
		}
		else
		{
			OutNode.DisplayName = First->DisplayName;
			OutNode.Principle = First->Description;
			OutNode.Effects = OutNode.Variants[0].Effects;
			OutNode.Usage = OutNode.Variants[0].Usage;
			OutNode.SimpleChoiceId = First->ChoiceId;
			OutNode.bCanAcquireSimple =
				OutNode.State == EGridTalentNodeState::Available && !SelectedChoiceIds.Contains(First->ChoiceId);
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
					Node.PreviousNodeId = Previous.TalentNodeId;
					Node.PreviousNodeDisplayName = Previous.DisplayName.IsEmpty() ? ConceptualNodeDisplayName(Previous) : Previous.DisplayName;
					if (Node.State == EGridTalentNodeState::LockedPrerequisite && !Node.PreviousNodeDisplayName.IsEmpty())
					{
						Node.StatusText = FText::Format(
							NSLOCTEXT("GridSkillsPage", "TalentLockedPrevious",
								"VERROUILLÉ — nécessite « {0} »"), Node.PreviousNodeDisplayName);
					}
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

	TArray<FRPGTalentRuntimeView> TalentViews;
	if (!FRPGTalentRuntimeService::TryGetSelectedTalents(PartyInventoryComponent, CharacterIndex, TalentViews)) return false;
	TalentViews.Sort([](const FRPGTalentRuntimeView& Left, const FRPGTalentRuntimeView& Right)
	{
		return Left.ChoiceId.ToString().Compare(Right.ChoiceId.ToString(), ESearchCase::CaseSensitive) < 0;
	});
	Candidate.Talents.Reserve(TalentViews.Num());
	for (const FRPGTalentRuntimeView& Talent : TalentViews)
	{
		FGridTalentEntryView TalentView;
		TalentView.ChoiceId = Talent.ChoiceId;
		TalentView.DisplayName = Talent.DisplayName;
		TalentView.Description = Talent.Description;
		TalentView.MinimumLevel = Talent.MinimumLevel;
		TalentView.PointCost = Talent.PointCost;
		TalentView.bSelected = Talent.bSelected;
		Candidate.Talents.Add(MoveTemp(TalentView));
	}

	FRPGTalentPointBalance Balance;
	if (!FRPGTalentRuntimeService::TryGetTalentPointBalance(PartyInventoryComponent, CharacterIndex, Balance)) return false;
	Candidate.GrantedTalentPoints = Balance.GrantedPoints;
	Candidate.SpentTalentPoints = Balance.SpentPoints;
	Candidate.RemainingTalentPoints = Balance.RemainingPoints;

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
