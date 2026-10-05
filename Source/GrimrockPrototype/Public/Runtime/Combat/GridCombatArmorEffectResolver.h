#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"

/**
 * Pure C3 resolver for direct PhysicalArmor / MagicalArmor mutations.
 * Attack damage effects are folded into FGridAttackResult before the target
 * mutation so combat log, ArmorGate C1 and broadcasts observe one result.
 */
class GRIMROCKPROTOTYPE_API FGridCombatArmorEffectResolver
{
public:
	static FGridCombatArmorEffectSourceContext MakeSourceContext(const FGridCharacterInventoryState& Character);
	static FGridCombatArmorEffectSourceContext MakeSourceContext(
		const FGridCharacterInventoryState& Character, const FRPGAttributes& EffectiveAttributes);

	static void ApplyReferenceModifiers(FGridCombatArmorPoolSnapshot& InOutSnapshot, const FGridResolvedCombatModifiers& Modifiers);

	static bool ResolveOne(const FGridCombatArmorEffectProfile& Profile, const FGridCombatArmorPoolSnapshot& Snapshot,
		const FGridResolvedCombatModifiers& Modifiers, const FGridCombatArmorEffectSourceContext* SourceContext,
		const FGridAttackResult* AttackResult, FGridCombatArmorEffectResult& OutResult);

	static bool WouldAnyRestore(const TArray<FGridCombatArmorEffectProfile>& Profiles, const FGridCombatArmorPoolSnapshot& Snapshot,
		const FGridResolvedCombatModifiers& Modifiers, const FGridCombatArmorEffectSourceContext* SourceContext = nullptr);

	/** Applies only Restore profiles and updates the snapshot sequentially. */
	static int32 ApplyRestoreEffects(const TArray<FGridCombatArmorEffectProfile>& Profiles, FGridCombatArmorPoolSnapshot& InOutSnapshot,
		const FGridResolvedCombatModifiers& Modifiers, const FGridCombatArmorEffectSourceContext* SourceContext = nullptr,
		TArray<FGridCombatArmorEffectResult>* OutResults = nullptr);

	/**
	 * Applies only successful-hit Damage profiles. Additional armor damage is
	 * clamped to armor remaining after the primary attack and never reaches HP.
	 */
	static int32 ApplyAttackDamageEffects(const TArray<FGridCombatArmorEffectProfile>& Profiles, const FGridCombatArmorPoolSnapshot& ReferenceSnapshot,
		const FGridResolvedCombatModifiers& Modifiers, FGridAttackResult& InOutAttackResult,
		const FGridCombatArmorEffectSourceContext* SourceContext = nullptr, TArray<FGridCombatArmorEffectResult>* OutResults = nullptr);

	static int32 GetRestorationPercentModifier(EGridCombatArmorPool Pool, const FGridResolvedCombatModifiers& Modifiers);
};
