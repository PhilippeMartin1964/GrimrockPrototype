#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatLog.h"
#include "Runtime/GridEventFeedTypes.h"

/** EVENTHUD01.8 pure projection from resolved combat feedback to Event Feed data. */
class GRIMROCKPROTOTYPE_API FGridCombatEventFeedFormatter
{
public:
	/** Returns false for combat-log entries intentionally excluded from the Event Feed. */
	static bool BuildEntry(const FGridCombatLogEntry& CombatEntry, FGridHudEventEntry& OutEntry);

	/** One-to-one semantic mapping for every runtime damage type. */
	static EGridHudEventSemantic ResolveDamageSemantic(EGridDamageType DamageType);
};
