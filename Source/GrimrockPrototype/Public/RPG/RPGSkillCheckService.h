#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"

class URPGSkillAsset;
struct FGridCharacterInventoryState;
struct FGridPartyInventoryState;
struct FRPGAttributes;

/** Pure deterministic resolver for non-combat skill checks. */
struct GRIMROCKPROTOTYPE_API FRPGSkillCheckService
{
	/**
     * Resolves one d20 skill check using the caller-provided random stream.
     * Rejected requests do not consume the stream.
     */
	static bool TryResolveSkillCheck(const FGridCharacterInventoryState& CharacterState, const URPGSkillAsset* SkillDefinition, int32 Difficulty,
		FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult, const FRPGSkillCheckContext* Context = nullptr);

	/** Selects the best living eligible member by static bonus, then resolves one d20 and applies party modifiers once. */
	static bool TryResolveBestPartySkillCheck(const FGridPartyInventoryState& PartyState, const URPGSkillAsset* SkillDefinition, int32 Difficulty,
		FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult, int32& OutCharacterIndex,
		const FRPGSkillCheckContext* Context = nullptr);

	/** Returns the raw character attribute selected by the skill definition. */
	static int32 GetGoverningAttributeValue(const FRPGAttributes& Attributes, ERPGSkillGoverningAttribute GoverningAttribute);
};
