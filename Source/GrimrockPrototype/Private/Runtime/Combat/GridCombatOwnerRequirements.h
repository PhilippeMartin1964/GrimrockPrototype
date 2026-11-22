#pragma once

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "Runtime/GridInventoryTypes.h"

namespace GridCombatOwnerRequirements
{
	inline bool Collect(const FGridCharacterInventoryState& Character, TSet<FName>& OutRequirements)
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

		return FRPGClassProgressionService::CollectSatisfiedRequirements(ClassDefinition, Character.Level, SelectedChoiceIds, OutRequirements);
	}

	inline bool AreSatisfied(const TArray<FName>& RequiredIds, const TSet<FName>& OwnerRequirements)
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
}
