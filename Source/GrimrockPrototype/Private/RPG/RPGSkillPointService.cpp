#include "RPG/RPGSkillPointService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridPartyInventoryComponent.h"

int32 FRPGSkillPointService::GetTotalPointsGranted(int32 CharacterLevel)
{
	const int32 MinimumLevel = URPGCharacterRulesLibrary::GetMinimumLevel();
	const int32 MaximumLevel = URPGCharacterRulesLibrary::GetMaximumLevel();
	if (CharacterLevel < MinimumLevel || CharacterLevel > MaximumLevel)
	{
		return 0;
	}
	return CharacterLevel + 3;
}

int32 FRPGSkillPointService::GetRankCapForLevel(int32 CharacterLevel)
{
	const int32 MinimumLevel = URPGCharacterRulesLibrary::GetMinimumLevel();
	const int32 MaximumLevel = URPGCharacterRulesLibrary::GetMaximumLevel();
	if (CharacterLevel < MinimumLevel || CharacterLevel > MaximumLevel)
	{
		return 0;
	}
	if (CharacterLevel <= 4) return 2;
	if (CharacterLevel <= 9) return 3;
	if (CharacterLevel <= 14) return 4;
	return 5;
}

bool FRPGSkillPointService::TryGetBalance(
	const FGridCharacterInventoryState& CharacterState,
	FRPGSkillPointBalance& OutBalance)
{
	OutBalance = FRPGSkillPointBalance();

	const int32 GrantedPoints = GetTotalPointsGranted(CharacterState.Level);
	const int32 RankCap = GetRankCapForLevel(CharacterState.Level);
	if (GrantedPoints <= 0 || RankCap <= 0 || !FRPGSkillService::ValidateSkillState(CharacterState))
	{
		return false;
	}

	int32 SpentPoints = 0;
	for (const FRPGSkillRank& Rank : CharacterState.SkillRanks)
	{
		SpentPoints += Rank.Rank;
	}

	if (SpentPoints < 0 || SpentPoints > GrantedPoints)
	{
		return false;
	}

	OutBalance.GrantedPoints = GrantedPoints;
	OutBalance.SpentPoints = SpentPoints;
	OutBalance.RemainingPoints = GrantedPoints - SpentPoints;
	OutBalance.RankCap = RankCap;
	return true;
}

ERPGSkillPointMutationRejectReason FRPGSkillPointService::GetNextRankPurchaseAvailability(
	const FGridCharacterInventoryState& CharacterState,
	const URPGSkillAsset* SkillDefinition)
{
	if (!IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition())
	{
		return ERPGSkillPointMutationRejectReason::InvalidDefinition;
	}

	if (GetTotalPointsGranted(CharacterState.Level) <= 0 || GetRankCapForLevel(CharacterState.Level) <= 0)
	{
		return ERPGSkillPointMutationRejectReason::InvalidLevel;
	}

	if (!FRPGSkillService::ValidateSkillState(CharacterState))
	{
		return ERPGSkillPointMutationRejectReason::InvalidSkillState;
	}

	FRPGSkillPointBalance Balance;
	if (!TryGetBalance(CharacterState, Balance))
	{
		return ERPGSkillPointMutationRejectReason::InvalidPointBalance;
	}

	const int32 CurrentRank = FRPGSkillService::GetSkillRank(CharacterState, SkillDefinition->SkillId);
	if (CurrentRank >= SkillDefinition->MaxRank)
	{
		return ERPGSkillPointMutationRejectReason::SkillMaxRankReached;
	}
	if (CurrentRank >= Balance.RankCap)
	{
		return ERPGSkillPointMutationRejectReason::LevelRankCapReached;
	}
	if (Balance.RemainingPoints <= 0)
	{
		return ERPGSkillPointMutationRejectReason::NoSkillPoints;
	}

	return ERPGSkillPointMutationRejectReason::None;
}

bool FRPGSkillPointService::TryPurchaseNextRank(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	int32 CharacterIndex,
	const URPGSkillAsset* SkillDefinition,
	FRPGSkillPointMutationResult& OutResult)
{
	OutResult = FRPGSkillPointMutationResult();

	if (!IsValid(PartyInventoryComponent))
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidInventory;
		return false;
	}
	if (!PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidCharacter;
		return false;
	}

	FGridCharacterInventoryState& Character =
		PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];

	OutResult.SkillId = IsValid(SkillDefinition) ? SkillDefinition->SkillId : NAME_None;
	OutResult.RejectReason = GetNextRankPurchaseAvailability(Character, SkillDefinition);

	FRPGSkillPointBalance Balance;
	if (TryGetBalance(Character, Balance))
	{
		OutResult.GrantedPoints = Balance.GrantedPoints;
		OutResult.SpentPoints = Balance.SpentPoints;
		OutResult.RemainingPoints = Balance.RemainingPoints;
		OutResult.RankCap = Balance.RankCap;
	}

	if (OutResult.RejectReason != ERPGSkillPointMutationRejectReason::None)
	{
		return false;
	}

	OutResult.PreviousRank = FRPGSkillService::GetSkillRank(Character, SkillDefinition->SkillId);

	FRPGSkillMutationResult Mutation;
	if (!FRPGSkillService::TryIncreaseSkillRank(Character, SkillDefinition, 1, Mutation) ||
		!Mutation.bChanged ||
		Mutation.NewRank != OutResult.PreviousRank + 1)
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::MutationRejected;
		return false;
	}

	OutResult.bCommitted = true;
	OutResult.NewRank = Mutation.NewRank;
	OutResult.SpentPoints = Balance.SpentPoints + 1;
	OutResult.RemainingPoints = Balance.RemainingPoints - 1;
	OutResult.RejectReason = ERPGSkillPointMutationRejectReason::None;

	PartyInventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

bool FRPGSkillPointService::TryRefundPurchasedRank(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	int32 CharacterIndex,
	const URPGSkillAsset* SkillDefinition,
	int32 SessionFloorRank,
	FRPGSkillPointMutationResult& OutResult)
{
	OutResult = FRPGSkillPointMutationResult();

	if (!IsValid(PartyInventoryComponent))
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidInventory;
		return false;
	}
	if (!PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidCharacter;
		return false;
	}
	if (!IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition())
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidDefinition;
		return false;
	}

	FGridCharacterInventoryState& Character =
		PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];

	OutResult.SkillId = SkillDefinition->SkillId;

	if (GetTotalPointsGranted(Character.Level) <= 0 || GetRankCapForLevel(Character.Level) <= 0)
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidLevel;
		return false;
	}
	if (!FRPGSkillService::ValidateSkillState(Character))
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidSkillState;
		return false;
	}

	FRPGSkillPointBalance Balance;
	if (!TryGetBalance(Character, Balance))
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidPointBalance;
		return false;
	}

	OutResult.GrantedPoints = Balance.GrantedPoints;
	OutResult.SpentPoints = Balance.SpentPoints;
	OutResult.RemainingPoints = Balance.RemainingPoints;
	OutResult.RankCap = Balance.RankCap;
	OutResult.PreviousRank = FRPGSkillService::GetSkillRank(Character, SkillDefinition->SkillId);

	if (OutResult.PreviousRank < 0 || OutResult.PreviousRank > SkillDefinition->MaxRank ||
		SessionFloorRank < 0 || SessionFloorRank > OutResult.PreviousRank)
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::InvalidSessionFloor;
		return false;
	}
	if (OutResult.PreviousRank <= SessionFloorRank)
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::NoSessionPurchaseToUndo;
		return false;
	}

	FRPGSkillMutationResult Mutation;
	if (!FRPGSkillService::TrySetSkillRank(Character, SkillDefinition, OutResult.PreviousRank - 1, Mutation) ||
		!Mutation.bChanged ||
		Mutation.NewRank != OutResult.PreviousRank - 1)
	{
		OutResult.RejectReason = ERPGSkillPointMutationRejectReason::MutationRejected;
		return false;
	}

	OutResult.bCommitted = true;
	OutResult.NewRank = Mutation.NewRank;
	OutResult.SpentPoints = Balance.SpentPoints - 1;
	OutResult.RemainingPoints = Balance.RemainingPoints + 1;
	OutResult.RejectReason = ERPGSkillPointMutationRejectReason::None;

	PartyInventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}
