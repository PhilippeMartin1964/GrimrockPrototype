#pragma once

#include "CoreMinimal.h"

class URPGSkillAsset;
struct FGridCharacterInventoryState;

enum class ERPGSkillMutationRejectReason : uint8
{
	None,
	InvalidDefinition,
	InvalidCurrentState,
	RankOutOfRange,
	InvalidDelta
};

struct FRPGSkillMutationResult
{
	bool bChanged = false;
	ERPGSkillMutationRejectReason RejectReason = ERPGSkillMutationRejectReason::None;
	int32 PreviousRank = 0;
	int32 NewRank = 0;
};

/** Pure MON20.6 runtime rules for sparse character skill ranks. */
struct GRIMROCKPROTOTYPE_API FRPGSkillService
{
	/** Structural validation: positive ranks, valid ids, no duplicates. */
	static bool ValidateSkillState(const FGridCharacterInventoryState& CharacterState);

	/** Returns zero for an absent skill or structurally invalid state. */
	static int32 GetSkillRank(const FGridCharacterInventoryState& CharacterState, FName SkillId);

	/**
     * Atomically assigns one rank. Setting zero removes the sparse entry.
     * Failed validation never mutates CharacterState.
     */
	static bool TrySetSkillRank(
		FGridCharacterInventoryState& CharacterState, const URPGSkillAsset* SkillDefinition, int32 NewRank, FRPGSkillMutationResult& OutResult);

	/** Positive-only convenience operation delegating to TrySetSkillRank. */
	static bool TryIncreaseSkillRank(
		FGridCharacterInventoryState& CharacterState, const URPGSkillAsset* SkillDefinition, int32 Delta, FRPGSkillMutationResult& OutResult);
};
