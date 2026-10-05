#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"

class UGridStatusEffectDefinitionAsset;

/**
 * Pure C1 eligibility + definition lookup for combat-driven status effects.
 * Actual mutation remains owned by UGridStatusEffectLifecycleSubsystem.
 */
class GRIMROCKPROTOTYPE_API FGridCombatStatusApplicationResolver
{
public:
	static int32 GetPostPhysicalArmor(const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult);
	static int32 GetPostMagicalArmor(const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult);
	static int32 GetPostHealth(const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult);

	static bool IsEligible(const FGridCombatStatusApplicationProfile& Profile, const FGridAttackTargetStats& TargetBefore,
		const FGridAttackResult* AttackResult);

	/** Canonical GridStatusEffect primary-asset lookup with loaded-object fallback for tests/editor runtime. */
	static UGridStatusEffectDefinitionAsset* ResolveDefinition(FName StatusEffectId);

	/**
	 * Pure preflight against a copied collection. Returns true only when at
	 * least one eligible profile would actually mutate the collection.
	 */
	static bool WouldAnyMutate(const TArray<FGridCombatStatusApplicationProfile>& Profiles, const FGuid& SourceId,
		const FGridAttackTargetStats& TargetBefore, const FGridAttackResult* AttackResult, const FGridStatusEffectCollection& CurrentEffects);
};
