#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"

class UGridPartyInventoryComponent;
struct FGridMonsterAttackDefinition;

/**
 * Equipment-driven reactive defense. No new hotbar actions, AP expenditure or
 * armor reserve. The shield branch only labels ACTUAL shield armor absorption.
 */
class GRIMROCKPROTOTYPE_API FGridEquippedGuardResolver
{
public:
	/** ARM013 offhand, physical melee, an ordinary landed noncritical attack. */
	static bool CanParry(const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex,
		const FGridMonsterAttackDefinition& Attack, const FGridAttackResult& Result);

	/** A deterministic roll supplied by the authoritative combat random stream.
	 * Called only after CanParry; never draws randomness in presentation/UI. */
	static bool IsSuccessfulParryRoll(int32 Roll1To100, int32 ParryChancePercent);

	/** Convert a hit into a miss BEFORE armor/status application. */
	static void ApplyParry(FGridAttackResult& Result);

	/** Only counts the protection really spent by the equipped OffHand shield.
	 * Does not increase evasion, resistances or armor in any way. */
	static int32 ResolveAbsorbedShieldDamage(const UGridPartyInventoryComponent* Inventory,
		int32 CharacterIndex, const FGridAttackResult& Result);
};
