#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"

/** Mechanical surface-QuickItem summary shared by inventory and hotbar. */
struct GRIMROCKPROTOTYPE_API FGridQuickItemSurfaceTooltipText
{
	static FText Build(const FGridCombatActionDefinition& Action);
};
