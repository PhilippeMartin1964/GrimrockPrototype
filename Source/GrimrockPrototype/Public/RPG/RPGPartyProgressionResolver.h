#pragma once

#include "CoreMinimal.h"

struct FGridCharacterInventoryState;
struct FGridPartyInventoryState;

/** Generic resolver for party-wide class progression contributions. */
struct GRIMROCKPROTOTYPE_API FRPGPartyProgressionResolver
{
	static int32 ResolveGroupSkillCheckModifier(const FGridPartyInventoryState& PartyState, FName SkillId);
	static int32 ResolveMaximumMobilityActionPointsModifier(const FGridPartyInventoryState& PartyState);
	static int32 ResolveFirstRoundInitiativeModifier(const FGridCharacterInventoryState& CharacterState);
};
