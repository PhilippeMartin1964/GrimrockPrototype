#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridInventoryTypes.h"

/** Runtime matching context for one action or incoming/outgoing attack. */
struct GRIMROCKPROTOTYPE_API FGridCombatModifierContext
{
	FName ActionId = NAME_None;
	FName SourceDefinitionId = NAME_None;
	EGridCombatActionSourcePolicy SourcePolicy = EGridCombatActionSourcePolicy::None;
	EGridCombatActionType ActionType = EGridCombatActionType::None;
	bool bHasDamageDescriptor = false;
	EGridDamageType DamageType = EGridDamageType::Physical;
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;
};

/** Deterministic aggregate of every matching modifier profile. */
struct GRIMROCKPROTOTYPE_API FGridResolvedCombatModifiers
{
	int32 AccuracyModifier = 0;
	int32 EvasionModifier = 0;
	int32 OutgoingDamagePercentModifier = 0;
	int32 IncomingDamagePercentModifier = 0;
	int32 CriticalChancePercentModifier = 0;
	int32 CriticalDamagePercentModifier = 0;
	FGridDamageResistanceSet ResistanceModifiers;
	int32 ActionPointCostModifier = 0;
	int32 ManaCostModifier = 0;
	int32 RangeCellsModifier = 0;
	int32 PhysicalArmorReferencePercentModifier = 0;
	int32 MagicalArmorReferencePercentModifier = 0;
	int32 PhysicalArmorRestorationPercentModifier = 0;
	int32 MagicalArmorRestorationPercentModifier = 0;
	int32 SurfaceDurationRoundsModifier = 0;
	int32 SurfacePeriodicDamagePercentModifier = 0;
	int32 SurfaceReactionDamagePercentModifier = 0;
	int32 SurfaceReactionAreaRadiusModifier = 0;

	void Reset()
	{
		*this = FGridResolvedCombatModifiers();
	}

	bool IsEmpty() const
	{
		return AccuracyModifier == 0 && EvasionModifier == 0 && OutgoingDamagePercentModifier == 0 && IncomingDamagePercentModifier == 0 &&
			CriticalChancePercentModifier == 0 && CriticalDamagePercentModifier == 0 && ResistanceModifiers.IsEmpty() && ActionPointCostModifier == 0 &&
			ManaCostModifier == 0 && RangeCellsModifier == 0 && PhysicalArmorReferencePercentModifier == 0 &&
			MagicalArmorReferencePercentModifier == 0 && PhysicalArmorRestorationPercentModifier == 0 &&
			MagicalArmorRestorationPercentModifier == 0 && SurfaceDurationRoundsModifier == 0 &&
			SurfacePeriodicDamagePercentModifier == 0 && SurfaceReactionDamagePercentModifier == 0 &&
			SurfaceReactionAreaRadiusModifier == 0;
	}
};

/**
 * Pure C2 resolver. It never mutates durable progression state or authored
 * definitions. Callers pass copies/projections when applying modifiers.
 */
class GRIMROCKPROTOTYPE_API FGridCombatModifierResolver
{
public:
	static FGridCombatModifierContext MakeActionContext(const FGridCombatActionDefinition& Definition, FName SourceDefinitionId = NAME_None);

	static FGridCombatModifierContext MakeAttackContext(FName ActionId, FName SourceDefinitionId, EGridCombatActionSourcePolicy SourcePolicy,
		EGridCombatActionType ActionType, EGridDamageType DamageType, EGridPhysicalDamageSubtype PhysicalSubtype);

	static bool Matches(const FGridCombatModifierProfile& Profile, const FGridCombatModifierContext& Context);

	static void Resolve(const TArray<FGridCombatModifierProfile>& Profiles, const FGridCombatModifierContext& Context,
		FGridResolvedCombatModifiers& OutModifiers);

	/** Resolve selected class-choice profiles from durable ChoiceIds. */
	static bool CollectCharacterChoiceModifiers(const FGridCharacterInventoryState& Character, TArray<FGridCombatModifierProfile>& OutProfiles);

	/** Resolve modifiers from active status definitions; AddStacks scales profiles by StackCount. */
	static bool CollectStatusModifiers(const FGridStatusEffectCollection& StatusEffects, TArray<FGridCombatModifierProfile>& OutProfiles);

	/** Character aggregate = selected choices + active status modifiers. */
	static bool CollectCharacterModifiers(const FGridCharacterInventoryState& Character, TArray<FGridCombatModifierProfile>& OutProfiles);

	/** Mutates only the runtime/action projection copy. */
	static void ApplyToActionDefinitionProjection(FGridCombatActionDefinition& Definition, const FGridResolvedCombatModifiers& Modifiers);

	static void ApplyOutgoingAttackModifiers(FGridAttackSourceStats& Source, const FGridResolvedCombatModifiers& Modifiers);

	static void ApplyIncomingAttackModifiers(
		FGridAttackTargetStats& Target, EGridDamageType DamageType, const FGridResolvedCombatModifiers& Modifiers);
};
