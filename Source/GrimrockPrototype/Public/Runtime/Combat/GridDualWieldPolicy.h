#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridItemDefinitionAsset.h"

class UGridItemSubsystem;

/** Physical dual source: every roll uses its own immutable N1-N20 item level. */
struct FGridDualWieldSource
{
	FGridItemInstance Item;
	/** Optional, for identities not yet migrated to DA_ItemSystem. */
	const UGridItemDefinitionAsset* Definition = nullptr;
	bool bCanonicalIdentity = false;
	TArray<FName> SourceTags;
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

	static bool TryResolve(const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex,
		FGridDualWieldPair& OutPair, const UGridItemSubsystem* RegistryOverride = nullptr);
};
