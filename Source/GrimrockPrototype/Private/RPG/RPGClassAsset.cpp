#include "RPG/RPGClassAsset.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"

FPrimaryAssetId URPGClassAsset::GetPrimaryAssetId() const
{
	if (ClassId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("RPGClass")), ClassId);
}

namespace
{
	bool AreMON154RequirementIdsValid(const TArray<FName>& RequirementIds)
	{
		TSet<FName> Seen;
		for (const FName RequirementId : RequirementIds)
		{
			if (RequirementId.IsNone() || Seen.Contains(RequirementId))
			{
				return false;
			}
			Seen.Add(RequirementId);
		}
		return true;
	}

	bool HasMON154ChoiceDependencyCycle(const TMap<FName, TArray<FName>>& Dependencies)
	{
		TSet<FName> Visiting;
		TSet<FName> Visited;

		TFunction<bool(FName)> Visit = [&](FName ChoiceId) -> bool
		{
			if (Visited.Contains(ChoiceId))
			{
				return false;
			}
			if (Visiting.Contains(ChoiceId))
			{
				return true;
			}

			Visiting.Add(ChoiceId);
			if (const TArray<FName>* Prerequisites = Dependencies.Find(ChoiceId))
			{
				for (const FName PrerequisiteId : *Prerequisites)
				{
					if (Visit(PrerequisiteId))
					{
						return true;
					}
				}
			}
			Visiting.Remove(ChoiceId);
			Visited.Add(ChoiceId);
			return false;
		};

		for (const TPair<FName, TArray<FName>>& Entry : Dependencies)
		{
			if (Visit(Entry.Key))
			{
				return true;
			}
		}
		return false;
	}

	bool AreMON154ChoiceDependenciesResolvable(
		const TArray<FRPGClassProgressionChoiceDefinition>& Choices,
		const TSet<FName>& AutomaticallyGrantedRequirementIds,
		FName ClassId)
	{
		TSet<FName> ResolvedChoiceIds;
		TSet<FName> SatisfiedRequirementIds = AutomaticallyGrantedRequirementIds;
		SatisfiedRequirementIds.Add(ClassId);

		bool bMadeProgress = true;
		while (ResolvedChoiceIds.Num() < Choices.Num() && bMadeProgress)
		{
			bMadeProgress = false;
			for (const FRPGClassProgressionChoiceDefinition& Choice : Choices)
			{
				if (ResolvedChoiceIds.Contains(Choice.ChoiceId))
				{
					continue;
				}

				bool bPrerequisitesSatisfied = true;
				for (const FName PrerequisiteChoiceId : Choice.PrerequisiteChoiceIds)
				{
					if (!ResolvedChoiceIds.Contains(PrerequisiteChoiceId))
					{
						bPrerequisitesSatisfied = false;
						break;
					}
				}
				if (!bPrerequisitesSatisfied)
				{
					continue;
				}

				for (const FName RequirementId : Choice.PrerequisiteRequirementIds)
				{
					if (!SatisfiedRequirementIds.Contains(RequirementId))
					{
						bPrerequisitesSatisfied = false;
						break;
					}
				}
				if (!bPrerequisitesSatisfied)
				{
					continue;
				}

				ResolvedChoiceIds.Add(Choice.ChoiceId);
				SatisfiedRequirementIds.Add(Choice.ChoiceId);
				for (const FName RequirementId : Choice.GrantedRequirementIds)
				{
					SatisfiedRequirementIds.Add(RequirementId);
				}
				bMadeProgress = true;
			}
		}

		return ResolvedChoiceIds.Num() == Choices.Num();
	}
}

bool URPGClassAsset::IsValidDefinition() const
{
	if (ClassId.IsNone() || HealthAtLevelOne <= 0)
	{
		return false;
	}

	TSet<FName> ActionIds;
	for (const FGridCombatActionDefinition& Action : CombatActions)
	{
		if (!Action.IsValid() ||
			(Action.SourcePolicy != EGridCombatActionSourcePolicy::Ability && Action.SourcePolicy != EGridCombatActionSourcePolicy::Spell &&
				Action.SourcePolicy != EGridCombatActionSourcePolicy::Universal) ||
			Action.ResourceCosts.SourceItemQuantityCost != 0 ||
			(Action.SourcePolicy == EGridCombatActionSourcePolicy::Universal && Action.ResourceCosts.ManaCost != 0) || ActionIds.Contains(Action.ActionId))
		{
			return false;
		}
		ActionIds.Add(Action.ActionId);
	}

	const int32 MinimumLevel = URPGCharacterRulesLibrary::GetMinimumLevel();
	const int32 MaximumLevel = URPGCharacterRulesLibrary::GetMaximumLevel();

	TSet<int32> GrantLevels;
	TSet<FName> AutomaticallyGrantedRequirementIds;
	for (const FRPGClassProgressionLevelGrant& Grant : ProgressionLevelGrants)
	{
		if (Grant.Level < MinimumLevel || Grant.Level > MaximumLevel || Grant.ChoicePointsGranted < 0 ||
			(Grant.ChoicePointsGranted == 0 && Grant.GrantedRequirementIds.IsEmpty()) || GrantLevels.Contains(Grant.Level) ||
			!AreMON154RequirementIdsValid(Grant.GrantedRequirementIds))
		{
			return false;
		}
		GrantLevels.Add(Grant.Level);

		for (const FName RequirementId : Grant.GrantedRequirementIds)
		{
			if (AutomaticallyGrantedRequirementIds.Contains(RequirementId))
			{
				return false;
			}
			AutomaticallyGrantedRequirementIds.Add(RequirementId);
		}
	}

	TSet<FName> ChoiceIds;
	TMap<FName, TArray<FName>> ChoiceDependencies;
	for (const FRPGClassProgressionChoiceDefinition& Choice : ProgressionChoices)
	{
		if (Choice.ChoiceId.IsNone() || Choice.MinimumLevel < MinimumLevel || Choice.MinimumLevel > MaximumLevel || Choice.PointCost <= 0 ||
			ChoiceIds.Contains(Choice.ChoiceId) || !AreMON154RequirementIdsValid(Choice.PrerequisiteChoiceIds) ||
			!AreMON154RequirementIdsValid(Choice.PrerequisiteRequirementIds) || !AreMON154RequirementIdsValid(Choice.GrantedRequirementIds) ||
			Choice.PrerequisiteChoiceIds.Contains(Choice.ChoiceId) || Choice.PrerequisiteRequirementIds.Contains(Choice.ChoiceId))
		{
			return false;
		}
		for (const FGridCombatModifierProfile& Modifier : Choice.CombatModifiers)
		{
			if (!Modifier.IsValid())
			{
				return false;
			}
		}
		for (const FGridCombatReactionProfile& Reaction : Choice.CombatReactions)
		{
			if (!Reaction.IsValid() || Reaction.bConsumeOwningStatus)
			{
				return false;
			}
		}
		for (const FRPGSkillProgressionModifier& SkillModifier : Choice.SkillModifiers)
		{
			if (!SkillModifier.IsValid())
			{
				return false;
			}
		}
		for (const FRPGPartyProgressionModifier& PartyModifier : Choice.PartyModifiers)
		{
			if (!PartyModifier.IsValid())
			{
				return false;
			}
		}
		if (Choice.FirstRoundInitiativeModifier < -20 || Choice.FirstRoundInitiativeModifier > 20)
		{
			return false;
		}
		ChoiceIds.Add(Choice.ChoiceId);
		ChoiceDependencies.Add(Choice.ChoiceId, Choice.PrerequisiteChoiceIds);
	}

	TSet<FName> AvailableRequirementIds = AutomaticallyGrantedRequirementIds;
	AvailableRequirementIds.Add(ClassId);
	for (const FName ChoiceId : ChoiceIds)
	{
		AvailableRequirementIds.Add(ChoiceId);
	}
	for (const FRPGClassProgressionChoiceDefinition& Choice : ProgressionChoices)
	{
		for (const FName RequirementId : Choice.GrantedRequirementIds)
		{
			AvailableRequirementIds.Add(RequirementId);
		}
	}
	for (const FRPGClassProgressionChoiceDefinition& Choice : ProgressionChoices)
	{
		for (const FName PrerequisiteId : Choice.PrerequisiteChoiceIds)
		{
			if (!ChoiceIds.Contains(PrerequisiteId))
			{
				return false;
			}
		}
		for (const FName RequirementId : Choice.PrerequisiteRequirementIds)
		{
			if (!AvailableRequirementIds.Contains(RequirementId))
			{
				return false;
			}
		}
	}

	if (HasMON154ChoiceDependencyCycle(ChoiceDependencies) ||
		!AreMON154ChoiceDependenciesResolvable(ProgressionChoices, AutomaticallyGrantedRequirementIds, ClassId))
	{
		return false;
	}

	// Opt-in starting kits: legacy classes with no kit stay valid and empty.
	TSet<FName> SeenKitIds;
	for (const FRPGStartingEquipmentKit& Kit : StartingEquipmentKits)
	{
		if (Kit.KitId.IsNone() || SeenKitIds.Contains(Kit.KitId) || Kit.Items.IsEmpty())
		{
			return false;
		}
		SeenKitIds.Add(Kit.KitId);
		for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
		{
			if (!IsValid(Entry.ItemDefinition.Get()) || Entry.ItemLevel < 0 || Entry.ItemLevel > 20 || Entry.Quantity < 1 ||
				(Entry.TargetSlot != EGridEquipmentSlot::None && Entry.Quantity != 1))
			{
				return false;
			}
		}
	}
	if (!DefaultStartingEquipmentKitId.IsNone() && !SeenKitIds.Contains(DefaultStartingEquipmentKitId))
	{
		return false;
	}

	return true;
}

const FRPGClassProgressionChoiceDefinition* URPGClassAsset::FindProgressionChoice(FName ChoiceId) const
{
	if (ChoiceId.IsNone())
	{
		return nullptr;
	}

	return ProgressionChoices.FindByPredicate(
		[ChoiceId](const FRPGClassProgressionChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == ChoiceId;
		});
}

const FRPGStartingEquipmentKit* URPGClassAsset::FindStartingEquipmentKit(FName KitId) const
{
	return KitId.IsNone() ? nullptr : StartingEquipmentKits.FindByPredicate(
		[KitId](const FRPGStartingEquipmentKit& Kit)
		{
			return Kit.KitId == KitId;
		});
}
