#pragma once

#include "CoreMinimal.h"

class UGridPartyInventoryComponent;

DECLARE_MULTICAST_DELEGATE_FourParams(FGridCharacterLevelUpAppliedNativeSignature, int32, int32, int32, int32);

/** Source-aware MON15.5 notification without breaking MON15.3 observers. */
DECLARE_MULTICAST_DELEGATE_FiveParams(FGridCharacterLevelUpAppliedWithSourceNativeSignature, UGridPartyInventoryComponent*, int32, int32, int32, int32);

/**
 * MON15.3 runtime transaction for applying level changes already justified by
 * cumulative Experience. The service owns no persistent state.
 */
struct GRIMROCKPROTOTYPE_API FRPGLevelUpService
{
	/**
     * Applies every pending level for one active character in one transaction.
     * Returns false when no level is pending or when validation fails.
     */
	static bool ApplyPendingLevelUp(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, bool bNotifyPartyInventoryChanged = true);

	/** Applies pending levels to every active character and returns levels gained. */
	static int32 ApplyPendingLevelUps(UGridPartyInventoryComponent* PartyInventoryComponent, bool bNotifyPartyInventoryChanged = true);

	/**
     * CharacterIndex, PreviousLevel, NewLevel, LevelsGained.
     * Observers should read the final character state from the party component.
     */
	static FGridCharacterLevelUpAppliedNativeSignature& OnCharacterLevelUpApplied();

	/**
     * PartyInventoryComponent, CharacterIndex, PreviousLevel, NewLevel,
     * LevelsGained. Added by MON15.5 for runtime notification/UI routing.
     */
	static FGridCharacterLevelUpAppliedWithSourceNativeSignature& OnCharacterLevelUpAppliedWithSource();
};
