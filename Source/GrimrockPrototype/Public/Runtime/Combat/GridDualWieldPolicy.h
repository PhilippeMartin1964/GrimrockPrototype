#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridItemDefinitionAsset.h"

/** Resolved physical weapon: an individual roll uses its own N1 catalogue profile. */
struct FGridDualWieldSource
{
	FGridItemInstance Item;
	const UGridItemDefinitionAsset* Definition = nullptr;
	FGridCombatActionDefinition Action;
	FGridOffensiveEquipmentProfile Profile;
};

struct FGridDualWieldPair
{
	FGridDualWieldSource Main;
	FGridDualWieldSource Off;
};

class UGridPartyInventoryComponent;

/**
 * Authoritative data-only eligibility and source-resolution policy.
 * Does not create attacks, spend resources, mutate equipment or apply effects.
 */
class GRIMROCKPROTOTYPE_API FGridDualWieldPolicy
{
public:
	static FName ActionId();
	static constexpr int32 ActionPointCost = 3;
	static constexpr int32 OffHandDamagePercent = 50;

	static bool TryResolve(const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, FGridDualWieldPair& OutPair);
};
