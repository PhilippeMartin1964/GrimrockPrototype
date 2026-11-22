#pragma once

#include "RPG/RPGCharacterTypes.h"

namespace RPGResourceRules
{
	// Preserves damage without resurrecting; mana preserves expenditure, including zero maxima.
	inline int32 PreserveHealthDeficit(const FRPGDerivedStats& PreviousStats, const FRPGCharacterResources& PreviousResources, int32 NewMaximumHealth)
	{
		const int32 SafeNewMaximum = FMath::Max(1, NewMaximumHealth);
		const int32 SafePreviousMaximum = FMath::Max(1, PreviousStats.MaxHealth);
		const int32 SafePreviousCurrent = FMath::Clamp(PreviousResources.CurrentHealth, 0, SafePreviousMaximum);
		if (SafePreviousCurrent <= 0)
		{
			return 0;
		}

		const int32 DamageTaken = SafePreviousMaximum - SafePreviousCurrent;
		return FMath::Clamp(SafeNewMaximum - DamageTaken, 0, SafeNewMaximum);
	}

	inline int32 PreserveManaDeficit(const FRPGDerivedStats& PreviousStats, const FRPGCharacterResources& PreviousResources, int32 NewMaximumMana)
	{
		const int32 SafeNewMaximum = FMath::Max(0, NewMaximumMana);
		const int32 SafePreviousMaximum = FMath::Max(0, PreviousStats.MaxMana);
		const int32 SafePreviousCurrent = FMath::Clamp(PreviousResources.CurrentMana, 0, SafePreviousMaximum);
		const int32 ManaSpent = SafePreviousMaximum - SafePreviousCurrent;
		return FMath::Clamp(SafeNewMaximum - ManaSpent, 0, SafeNewMaximum);
	}
}
