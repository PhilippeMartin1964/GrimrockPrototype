#pragma once

#include "CoreMinimal.h"

struct FGridPartyInventoryState;

/** Current-schema validation boundary for durable character KnownSpellIds. */
struct GRIMROCKPROTOTYPE_API FGridSpellbookPersistence
{
	static bool ValidatePartySpellbooks(const FGridPartyInventoryState& PartyState, FString& OutError);
};
