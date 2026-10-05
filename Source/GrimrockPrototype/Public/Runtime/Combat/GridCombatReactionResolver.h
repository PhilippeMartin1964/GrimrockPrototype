#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"

struct FGridCharacterInventoryState;
struct FGridStatusEffectCollection;

struct GRIMROCKPROTOTYPE_API FGridCombatReactionBinding
{
	FGridCombatReactionProfile Profile;
	FName OwningStatusEffectId = NAME_None;
};

struct GRIMROCKPROTOTYPE_API FGridCombatReactionUsageKey
{
	FGuid OwnerCombatantId;
	FName ReactionId = NAME_None;

	bool operator==(const FGridCombatReactionUsageKey& Other) const
	{
		return OwnerCombatantId == Other.OwnerCombatantId && ReactionId == Other.ReactionId;
	}

	friend uint32 GetTypeHash(const FGridCombatReactionUsageKey& Key)
	{
		return HashCombine(GetTypeHash(Key.OwnerCombatantId), GetTypeHash(Key.ReactionId));
	}
};

struct GRIMROCKPROTOTYPE_API FGridCombatReactionActionUsageKey
{
	FGridCombatReactionUsageKey Reaction;
	FGuid ActionInstanceId;

	bool operator==(const FGridCombatReactionActionUsageKey& Other) const
	{
		return Reaction == Other.Reaction && ActionInstanceId == Other.ActionInstanceId;
	}

	friend uint32 GetTypeHash(const FGridCombatReactionActionUsageKey& Key)
	{
		return HashCombine(GetTypeHash(Key.Reaction), GetTypeHash(Key.ActionInstanceId));
	}
};

class GRIMROCKPROTOTYPE_API FGridCombatReactionLedger
{
public:
	bool CanTrigger(const FGridCombatReactionProfile& Profile, const FGuid& OwnerCombatantId, const FGridCombatReactionEvent& Event) const;
	bool Commit(const FGridCombatReactionProfile& Profile, const FGuid& OwnerCombatantId, const FGridCombatReactionEvent& Event);
	void Reset();

private:
	TMap<FGridCombatReactionUsageKey, int32> LastRoundByReaction;
	TSet<FGridCombatReactionActionUsageKey> UsedActions;
};

class GRIMROCKPROTOTYPE_API FGridCombatReactionResolver
{
public:
	static bool Matches(const FGridCombatReactionProfile& Profile, const FGridCombatReactionEvent& Event);

	static bool CollectStatusBindings(const FGridStatusEffectCollection& StatusEffects, TArray<FGridCombatReactionBinding>& OutBindings);
	static bool CollectCharacterBindings(const FGridCharacterInventoryState& Character, TArray<FGridCombatReactionBinding>& OutBindings);

	static void ResolveMatches(const TArray<FGridCombatReactionBinding>& Bindings, const FGuid& OwnerCombatantId,
		const FGridCombatReactionEvent& Event, FGridCombatReactionLedger& Ledger, bool bCommit, TArray<FGridCombatReactionMatch>& OutMatches);
};
