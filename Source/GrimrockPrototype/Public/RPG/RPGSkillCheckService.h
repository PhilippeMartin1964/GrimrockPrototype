#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"

class URPGSkillAsset;
struct FGridCharacterInventoryState;
struct FRPGAttributes;

/** Pure deterministic resolver for non-combat skill checks. */
struct GRIMROCKPROTOTYPE_API FRPGSkillCheckService
{
    /**
     * Resolves one d20 skill check using the caller-provided random stream.
     * Rejected requests do not consume the stream.
     */
    static bool TryResolveSkillCheck (
        const FGridCharacterInventoryState& CharacterState,
        const URPGSkillAsset* SkillDefinition,
        int32 Difficulty,
        FRandomStream& RandomStream,
        FRPGSkillCheckResult& OutResult);

    /** Returns the raw character attribute selected by the skill definition. */
    static int32 GetGoverningAttributeValue (
        const FRPGAttributes& Attributes,
        ERPGSkillGoverningAttribute GoverningAttribute);
};
