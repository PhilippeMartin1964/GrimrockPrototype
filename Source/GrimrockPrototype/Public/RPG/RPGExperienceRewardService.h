#pragma once

#include "CoreMinimal.h"

class UGridPartyInventoryComponent;

/**
 * Runtime notification emitted after cumulative XP was actually applied to one
 * active character. The persistent state remains owned by
 * FGridCharacterInventoryState inside UGridPartyInventoryComponent.
 */
DECLARE_MULTICAST_DELEGATE_FourParams (
    FGridCharacterExperienceAwardedNativeSignature,
    int32 /* CharacterIndex */,
    int32 /* AwardedExperience */,
    int32 /* PreviousExperience */,
    int32 /* NewExperience */);

/**
 * Stateless MON15.2 transaction for sharing one monster XP reward between the
 * active party. It never changes Level, derived stats, inventory or hotbar.
 */
struct GRIMROCKPROTOTYPE_API FRPGExperienceRewardService
{
    static int32 AwardToActiveParty (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 TotalExperienceReward);

    static FGridCharacterExperienceAwardedNativeSignature&
        OnCharacterExperienceAwarded ();
};
