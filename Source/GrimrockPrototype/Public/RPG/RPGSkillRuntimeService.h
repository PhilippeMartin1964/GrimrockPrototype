#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillService.h"
#include "RPG/RPGSkillTypes.h"

class UGridPartyInventoryComponent;
class URPGSkillAsset;

/**
 * Stateless runtime bridge between the authoritative party component and the
 * pure MON20.6 skill services. It never owns or mirrors character state.
 */
struct GRIMROCKPROTOTYPE_API FRPGSkillRuntimeService
{
	static bool TryGetCharacterSkillRank(const UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex, FName SkillId, int32& OutRank);

	static bool TryGetSelectedCharacterSkillRank(const UGridPartyInventoryComponent* PartyInventory, FName SkillId, int32& OutRank);

	static bool TrySetCharacterSkillRank(UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex, const URPGSkillAsset* SkillDefinition,
		int32 NewRank, FRPGSkillMutationResult& OutResult);

	static bool TrySetSelectedCharacterSkillRank(
		UGridPartyInventoryComponent* PartyInventory, const URPGSkillAsset* SkillDefinition, int32 NewRank, FRPGSkillMutationResult& OutResult);

	static bool TryIncreaseCharacterSkillRank(UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex, const URPGSkillAsset* SkillDefinition,
		int32 Delta, FRPGSkillMutationResult& OutResult);

	static bool TryIncreaseSelectedCharacterSkillRank(
		UGridPartyInventoryComponent* PartyInventory, const URPGSkillAsset* SkillDefinition, int32 Delta, FRPGSkillMutationResult& OutResult);

	static bool TryResolveCharacterSkillCheck(const UGridPartyInventoryComponent* PartyInventory, int32 CharacterIndex, const URPGSkillAsset* SkillDefinition,
		int32 Difficulty, FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult);

	static bool TryResolveSelectedCharacterSkillCheck(const UGridPartyInventoryComponent* PartyInventory, const URPGSkillAsset* SkillDefinition,
		int32 Difficulty, FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult);
};
