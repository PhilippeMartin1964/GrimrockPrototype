#pragma once

#include "Runtime/GridInventoryTypes.h"

namespace GridPartySaveProjections
{
	inline int32 CountCharactersWithKnownSpells(const FGridPartyInventoryState& PartyState)
	{
		int32 Count = 0;
		for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			Count += Character.KnownSpellIds.IsEmpty() ? 0 : 1;
		}
		for (const FGridCharacterInventoryState& Character : PartyState.CharacterPool)
		{
			Count += Character.KnownSpellIds.IsEmpty() ? 0 : 1;
		}
		return Count;
	}
}
