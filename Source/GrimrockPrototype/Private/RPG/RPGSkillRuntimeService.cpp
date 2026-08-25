#include "RPG/RPGSkillRuntimeService.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGSkillRuntimeServicePrivate
{
	const FGridCharacterInventoryState* GetCharacterState(const UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex)
	{
		if (!IsValid(PartyInventory) || !PartyInventory->IsValidCharacterIndex(CharacterIndex))
		{
			return nullptr;
		}

		return &PartyInventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	}

	FGridCharacterInventoryState* GetMutableCharacterState(UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex)
	{
		if (!IsValid(PartyInventory) || !PartyInventory->IsValidCharacterIndex(CharacterIndex))
		{
			return nullptr;
		}

		return &PartyInventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	}

	int32 GetSelectedCharacterIndex(const UGridPartyInventoryComponent* PartyInventory)
	{
		return IsValid(PartyInventory) ? PartyInventory->GetSelectedCharacterIndex() : INDEX_NONE;
	}

	void RejectMutationForInvalidCharacter(FRPGSkillMutationResult& OutResult)
	{
		OutResult = FRPGSkillMutationResult();
		OutResult.RejectReason = ERPGSkillMutationRejectReason::InvalidCurrentState;
	}

	void RejectCheckForInvalidCharacter(FRPGSkillCheckResult& OutResult)
	{
		OutResult = FRPGSkillCheckResult();
		OutResult.RejectReason = ERPGSkillCheckRejectReason::InvalidCharacterState;
	}
}

bool FRPGSkillRuntimeService::TryGetCharacterSkillRank(const UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex, FName SkillId, int32& OutRank)
{
	OutRank = 0;

	const FGridCharacterInventoryState* CharacterState = RPGSkillRuntimeServicePrivate::GetCharacterState(PartyInventory, CharacterIndex);
	if (!CharacterState || SkillId.IsNone() || !FRPGSkillService::ValidateSkillState(*CharacterState))
	{
		return false;
	}

	OutRank = FRPGSkillService::GetSkillRank(*CharacterState, SkillId);
	return true;
}

bool FRPGSkillRuntimeService::TryGetSelectedCharacterSkillRank(const UGridPartyInventoryComponent* PartyInventory, FName SkillId, int32& OutRank)
{
	return TryGetCharacterSkillRank(PartyInventory, RPGSkillRuntimeServicePrivate::GetSelectedCharacterIndex(PartyInventory), SkillId, OutRank);
}

bool FRPGSkillRuntimeService::TrySetCharacterSkillRank(UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex,
	const URPGSkillAsset* SkillDefinition, int32 NewRank, FRPGSkillMutationResult& OutResult)
{
	FGridCharacterInventoryState* CharacterState = RPGSkillRuntimeServicePrivate::GetMutableCharacterState(PartyInventory, CharacterIndex);
	if (!CharacterState)
	{
		RPGSkillRuntimeServicePrivate::RejectMutationForInvalidCharacter(OutResult);
		return false;
	}

	const bool bSuccess = FRPGSkillService::TrySetSkillRank(*CharacterState, SkillDefinition, NewRank, OutResult);
	if (bSuccess && OutResult.bChanged)
	{
		PartyInventory->NotifyPartyInventoryChanged(CharacterIndex);
	}
	return bSuccess;
}

bool FRPGSkillRuntimeService::TrySetSelectedCharacterSkillRank(
	UGridPartyInventoryComponent* PartyInventory, const URPGSkillAsset* SkillDefinition, int32 NewRank, FRPGSkillMutationResult& OutResult)
{
	return TrySetCharacterSkillRank(
		PartyInventory, RPGSkillRuntimeServicePrivate::GetSelectedCharacterIndex(PartyInventory), SkillDefinition, NewRank, OutResult);
}

bool FRPGSkillRuntimeService::TryIncreaseCharacterSkillRank(
	UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex, const URPGSkillAsset* SkillDefinition, int32 Delta, FRPGSkillMutationResult& OutResult)
{
	FGridCharacterInventoryState* CharacterState = RPGSkillRuntimeServicePrivate::GetMutableCharacterState(PartyInventory, CharacterIndex);
	if (!CharacterState)
	{
		RPGSkillRuntimeServicePrivate::RejectMutationForInvalidCharacter(OutResult);
		return false;
	}

	const bool bSuccess = FRPGSkillService::TryIncreaseSkillRank(*CharacterState, SkillDefinition, Delta, OutResult);
	if (bSuccess && OutResult.bChanged)
	{
		PartyInventory->NotifyPartyInventoryChanged(CharacterIndex);
	}
	return bSuccess;
}

bool FRPGSkillRuntimeService::TryIncreaseSelectedCharacterSkillRank(
	UGridPartyInventoryComponent* PartyInventory, const URPGSkillAsset* SkillDefinition, int32 Delta, FRPGSkillMutationResult& OutResult)
{
	return TryIncreaseCharacterSkillRank(
		PartyInventory, RPGSkillRuntimeServicePrivate::GetSelectedCharacterIndex(PartyInventory), SkillDefinition, Delta, OutResult);
}

bool FRPGSkillRuntimeService::TryResolveCharacterSkillCheck(const UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex,
	const URPGSkillAsset* SkillDefinition, int32 Difficulty, FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult)
{
	const FGridCharacterInventoryState* CharacterState = RPGSkillRuntimeServicePrivate::GetCharacterState(PartyInventory, CharacterIndex);
	if (!CharacterState)
	{
		RPGSkillRuntimeServicePrivate::RejectCheckForInvalidCharacter(OutResult);
		return false;
	}

	return FRPGSkillCheckService::TryResolveSkillCheck(*CharacterState, SkillDefinition, Difficulty, RandomStream, OutResult);
}

bool FRPGSkillRuntimeService::TryResolveSelectedCharacterSkillCheck(const UGridPartyInventoryComponent* PartyInventory, const URPGSkillAsset* SkillDefinition,
	int32 Difficulty, FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult)
{
	return TryResolveCharacterSkillCheck(
		PartyInventory, RPGSkillRuntimeServicePrivate::GetSelectedCharacterIndex(PartyInventory), SkillDefinition, Difficulty, RandomStream, OutResult);
}
