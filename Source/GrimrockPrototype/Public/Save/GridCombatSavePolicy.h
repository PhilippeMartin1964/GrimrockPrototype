#pragma once

#include "CoreMinimal.h"

struct FGridPartyInventoryState;
class AGrimrockPartyPawn;
class UGridTurnManagerComponent;

/**
 * MON18.9.1 durable save policy around combat.
 *
 * Combat state itself is deliberately not serialised. A persistent game owns
 * a pre-combat checkpoint and regular saves are rejected from combat start
 * through defeat. Victory returns to a stable saveable state.
 */
class GRIMROCKPROTOTYPE_API FGridCombatSavePolicy
{
public:
	static FString BuildPreCombatCheckpointSlotName(const FString& BaseSlotName);

	static bool IsSaveBlockedByCombatState(const UGridTurnManagerComponent* TurnManager);

	/**
     * Serialization boundary guard. The saved party is matched to its live
     * TurnManager by stable CharacterId so unrelated game/test worlds do not
     * block each other's saves.
     */
	static bool IsSaveBlockedForParty(const FGridPartyInventoryState& PartyState);

	/**
     * Saves the current stable exploration state to <PartySaveSlot>_AutoCombat.
     * Non-persistent transient fixtures (no slot / no completed character) are
     * deliberately accepted as skipped so historical combat tests stay pure.
     */
	static bool PreparePreCombatCheckpoint(AGrimrockPartyPawn* PartyPawn, FText& OutError, bool& bOutSkipped);
};
