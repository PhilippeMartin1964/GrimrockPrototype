#pragma once

#include "CoreMinimal.h"

class UGridPartyInventoryComponent;
class URPGSkillAsset;
struct FGridCharacterInventoryState;

enum class ERPGSkillPointPurchaseRejectReason : uint8
{
	None,
	InvalidInventory,
	InvalidCharacter,
	InvalidDefinition,
	InvalidLevel,
	InvalidSkillState,
	InvalidPointBalance,
	NoSkillPoints,
	LevelRankCapReached,
	SkillMaxRankReached,
	MutationRejected
};

struct FRPGSkillPointBalance
{
	int32 GrantedPoints = 0;
	int32 SpentPoints = 0;
	int32 RemainingPoints = 0;
	int32 RankCap = 0;
};

struct FRPGSkillPointPurchaseResult
{
	bool bCommitted = false;
	ERPGSkillPointPurchaseRejectReason RejectReason = ERPGSkillPointPurchaseRejectReason::None;
	FName SkillId = NAME_None;
	int32 PreviousRank = 0;
	int32 NewRank = 0;
	int32 GrantedPoints = 0;
	int32 SpentPoints = 0;
	int32 RemainingPoints = 0;
	int32 RankCap = 0;
};

/**
 * RPG-SKILL01 sole authority for the player-facing Skill Point economy.
 *
 * No Skill Point counter is persisted. Granted points derive from Level and
 * spent points derive from the durable sparse SkillRanks authority.
 */
struct GRIMROCKPROTOTYPE_API FRPGSkillPointService
{
	/** Level 1 grants 4 points; every later level grants +1. Invalid levels return zero. */
	static int32 GetTotalPointsGranted(int32 CharacterLevel);

	/** Current per-Skill rank cap: 2 at 1-4, 3 at 5-9, 4 at 10-14, 5 at 15-20. */
	static int32 GetRankCapForLevel(int32 CharacterLevel);

	/** Reconstructs the complete Skill Point balance from Level + SkillRanks. */
	static bool TryGetBalance(const FGridCharacterInventoryState& CharacterState, FRPGSkillPointBalance& OutBalance);

	/** Pure availability query used by read models and presentation. */
	static ERPGSkillPointPurchaseRejectReason GetNextRankPurchaseAvailability(
		const FGridCharacterInventoryState& CharacterState,
		const URPGSkillAsset* SkillDefinition);

	/** Spends one point and raises one Skill by one rank. */
	static bool TryPurchaseNextRank(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		const URPGSkillAsset* SkillDefinition,
		FRPGSkillPointPurchaseResult& OutResult);
};
