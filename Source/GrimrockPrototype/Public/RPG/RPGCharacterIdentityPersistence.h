#pragma once

#include "CoreMinimal.h"

struct FGridCharacterInventoryState;
struct FGridPartyInventoryState;

/**
 * TD07.3.4.3 persistence boundary for character authoring identity caches.
 * Durable IDs remain on the character; class/race labels, definitions and visual references are rebuilt transiently.
 */
struct GRIMROCKPROTOTYPE_API FRPGCharacterIdentityPersistence
{
	/** Remembers already-loaded class definitions before transient fields are omitted from an archive. */
	static void RememberRuntimeCaches(const FGridPartyInventoryState& PartyState);

	/** Rehydrates class/race identity caches plus Portrait/ClassIcon atomically for Active + Pool. */
	static bool RehydratePartyIdentity(FGridPartyInventoryState& PartyState, FString& OutError);

	/** Validates that transient identity caches match their durable IDs when present. */
	static bool ValidateRuntimePartyIdentity(const FGridPartyInventoryState& PartyState, FString& OutError);
};
