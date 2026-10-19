#pragma once

#include "CoreMinimal.h"

/**
 * UI-PROGRESSIONFEED01: transient, deterministic reminder cadence.
 * The authoritative RPG balances are read by the caller; this tracks only
 * which characters still have points (and their levels), never persists state.
 */
class GRIMROCKPROTOTYPE_API FGridPartyProgressionReminderState
{
public:
	static constexpr float ReminderIntervalSeconds = 300.0f;

	/** Returns true on first pending state, changes in pending roster/level,
	 *  and once per five active gameplay minutes while pending remains. */
	bool Advance(const TMap<FGuid, int32>& PendingCharacterLevels, float ActiveDeltaSeconds);

	void Reset();

private:
	TMap<FGuid, int32> LastPendingCharacterLevels;
	float SecondsSinceNotice = 0.0f;
};
