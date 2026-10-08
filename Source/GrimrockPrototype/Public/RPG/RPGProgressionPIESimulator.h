#pragma once

#include "CoreMinimal.h"

class UGridPartyInventoryComponent;

/**
 * RPG-DEV01 development-only progression transaction used by the PIE console
 * command. It owns no persistent state and delegates the real level mutation
 * to FRPGLevelUpService.
 */
struct GRIMROCKPROTOTYPE_API FRPGProgressionPIESimulator
{
	/**
	 * Moves one active character upward to TargetLevel by setting the exact
	 * cumulative XP threshold, then invoking the canonical level-up service.
	 *
	 * The transaction rejects demotion, an already reached target, incoherent
	 * Level/Experience state, and an unacknowledged previous level-up.
	 */
	static bool TrySetCharacterLevel(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		int32 TargetLevel,
		FText& OutFeedback);

	/** Convenience wrapper for the authoritative selected character. */
	static bool TrySetSelectedCharacterLevel(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 TargetLevel,
		FText& OutFeedback);
};
