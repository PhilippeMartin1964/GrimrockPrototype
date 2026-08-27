#pragma once

#include "CoreMinimal.h"

class URPGSkillAsset;
struct FGridPartyInventoryState;

/**
 * Current-schema validation boundary for durable character SkillRanks.
 * No capture or restore mirror exists: FGridCharacterInventoryState::SkillRanks is the authority.
 */
struct GRIMROCKPROTOTYPE_API FRPGSkillPersistence
{
	static bool ValidatePartySkills(const FGridPartyInventoryState& PartyState, FString& OutError);

	/** Test seam and deterministic validation with an injected canonical resolver. */
	static bool ValidatePartySkills(const FGridPartyInventoryState& PartyState,
		TFunctionRef<const URPGSkillAsset*(FName)> DefinitionResolver, FString& OutError);

	/** Resolves the canonical RPGSkill:<SkillId> definition used by production save validation. */
	static const URPGSkillAsset* ResolveDefinitionBySkillId(FName SkillId);
};
