#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"

/** Immutable inputs used to evaluate contributions without mutating gameplay. */
struct GRIMROCKPROTOTYPE_API FGridCombatActionCatalogContext
{
	int32 CharacterIndex = INDEX_NONE;
	FGuid CharacterId;
	bool bCombatActive = false;
	bool bCharacterDefeated = false;
	bool bActiveCombatant = false;
	bool bPartyBusy = false;
	bool bEnableQuickItemExecutors = false;
	bool bEnableClassActionExecutors = false;
	int32 RemainingActionPoints = 0;
	int32 RemainingMobilityActionPoints = 0;
	int32 CurrentHealth = 0;
	int32 MaximumHealth = 0;
	int32 CurrentMana = 0;
	int32 MaximumMana = 0;
	int32 CurrentPhysicalArmor = 0;
	int32 CurrentMagicalArmor = 0;
	int32 ReferencePhysicalArmor = 0;
	int32 ReferenceMagicalArmor = 0;
	FGridCombatArmorEffectSourceContext ArmorEffectSource;
	FGridStatusEffectCollection CurrentStatusEffects;
	TSet<FName> SatisfiedRequirements;
	/** One tag set per currently equipped hand item that can actually provide an attack. */
	TArray<TArray<FName>> EquippedOffensiveSourceTagSets;
	/** Parallel to EquippedOffensiveSourceTagSets; None represents a non-physical descriptor. */
	TArray<EGridPhysicalDamageSubtype> EquippedOffensivePhysicalSubtypes;
	TMap<FName, int32> RemainingCooldownRounds;
	TArray<FGridCombatModifierProfile> CombatModifiers;
	TArray<FRPGSkillRank> SkillRanks;
};

/**
 * Pure catalogue service. It evaluates definitions and current resources but
 * never resolves an effect and never consumes PA, mana or source items.
 */
class GRIMROCKPROTOTYPE_API FGridCombatActionCatalog
{
public:
	static void Build(const FGridCombatActionCatalogContext& Context, const TArray<FGridCombatActionContribution>& Contributions,
		TArray<FGridAvailableCombatAction>& OutActions);

	static FGridCombatActionDefinition MakeUnarmedAttackDefinition(int32 ActionPointCost);

	static FText GetAvailabilityReasonText(EGridCombatActionAvailabilityReason Reason);
};
