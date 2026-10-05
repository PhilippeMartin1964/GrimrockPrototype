#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridInventoryTypes.h"

/** Pure C8 target/filter helpers shared by combat, statuses and batch resolution. */
class GRIMROCKPROTOTYPE_API FGridCombatTargetingResolver
{
public:
	static bool IsDirectHostileTargetable(const FGridStatusEffectCollection& StatusEffects);

	static bool MatchesTargetFilter(const FGridCombatTargetFilterProfile& Filter, FName MonsterCategoryId,
		const FGridStatusEffectCollection& StatusEffects, const FGuid& ActingSourceId);

	static bool MatchesStatusRemoval(const FGridStatusEffectRuntimeState& State, const FGridCombatStatusRemovalProfile& Profile);

	static void CollectStatusRemovalIds(const FGridStatusEffectCollection& StatusEffects,
		const TArray<FGridCombatStatusRemovalProfile>& Profiles, TArray<FName>& OutEffectIds);

	static void CollectPartyTargets(const FGridPartyInventoryState& PartyState, EGridCombatTargetingPolicy Policy,
		int32 SourceCharacterIndex, int32 ExplicitTargetCharacterIndex, int32 FrontLineSlotCount, TArray<int32>& OutCharacterIndices);

	static void CollectPartyTargets(const FGridPartyInventoryState& PartyState, EGridCombatTargetingPolicy Policy,
		int32 SourceCharacterIndex, const TArray<int32>& ExplicitTargetCharacterIndices, int32 FrontLineSlotCount, TArray<int32>& OutCharacterIndices);

	static bool ShouldApplyStatusApplication(const FGridCombatStatusApplicationProfile& Profile, int32 TargetOrdinal);
};
