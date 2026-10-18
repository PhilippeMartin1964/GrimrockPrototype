#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatLog.h"
#include "Runtime/GridEventFeedTypes.h"

/** EVENTHUD01.8/8P pure projection from resolved combat feedback to Event Feed data. */
class GRIMROCKPROTOTYPE_API FGridCombatEventFeedFormatter
{
public:
	/** Returns false for combat-log entries intentionally excluded from the Event Feed. */
	static bool BuildEntry(const FGridCombatLogEntry& CombatEntry, FGridHudEventEntry& OutEntry);

	/** One-to-one semantic mapping for every runtime damage type. */
	static EGridHudEventSemantic ResolveDamageSemantic(EGridDamageType DamageType);

	/**
	 * EVENTHUD01.8P exact dice projection of one resolved random roll.
	 * A uniform inclusive range Min..Max is represented as
	 * 1d(Max-Min+1) + (Min-1), including virtual dice such as d7.
	 */
	static FText FormatRollAsDice(const FGridCombatRollTrace& Roll);
};
