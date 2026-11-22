#pragma once

#include "CoreMinimal.h"

namespace RPGSelectionRules
{
	inline bool BuildSelectionSet(const TArray<FName>& ChoiceIds, TSet<FName>& OutChoiceIds)
	{
		OutChoiceIds.Reset();
		for (const FName ChoiceId : ChoiceIds)
		{
			if (ChoiceId.IsNone() || OutChoiceIds.Contains(ChoiceId))
			{
				OutChoiceIds.Reset();
				return false;
			}
			OutChoiceIds.Add(ChoiceId);
		}
		return true;
	}
}
