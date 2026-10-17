#pragma once

#include "CoreMinimal.h"
#include "GridCombatTraceTypes.generated.h"

/** How one authoritative random combat value was produced. */
UENUM(BlueprintType)
enum class EGridCombatRollTraceKind : uint8
{
	None UMETA(DisplayName = "None"),
	Die UMETA(DisplayName = "Die"),
	Range UMETA(DisplayName = "Range")
};

/**
 * One random value actually consumed by combat resolution.
 *
 * Die supports future multi-die rolls through DiceCount + IndividualRolls.
 * Range represents the current uniform Min..Max damage resolver without
 * pretending that an arbitrary authored interval is dice notation.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCombatRollTrace
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Roll")
	bool bUsed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Roll")
	EGridCombatRollTraceKind Kind = EGridCombatRollTraceKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Roll")
	int32 DiceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Roll")
	int32 DiceSides = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Roll")
	int32 Minimum = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Roll")
	int32 Maximum = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Roll")
	TArray<int32> IndividualRolls;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Roll")
	int32 FinalValue = 0;
};

/** Exact hit-test inputs consumed by FGridCombatResolver. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCombatAttackTrace
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	FGridCombatRollTrace Roll;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	int32 SourceAccuracy = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	int32 AttackAccuracyBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	int32 FinalValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	bool bAlwaysHits = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	bool bNaturalAutoMiss = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	bool bNaturalAutoHit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	bool bCanCriticalHit = true;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	int32 CriticalChancePercent = 0;

	/** Lowest natural d20 result that can crit; 21 means criticals are disabled. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Attack")
	int32 CriticalMinimumRoll = 21;
};

/** Exact defense inputs consumed by FGridCombatResolver. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCombatDefenseTrace
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Defense")
	int32 BaseValue = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Defense")
	int32 EvasionModifier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Defense")
	int32 FinalValue = 0;
};

/**
 * Exact damage pipeline consumed by FGridCombatResolver.
 * Values are snapshots only: presentation must never recompute them.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCombatDamageTrace
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	bool bResolved = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	bool bDirectDamage = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	FGridCombatRollTrace Roll;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 SourceDamageBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 BaseDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 RawDamagePercent = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 ScaledBaseDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 CriticalDamagePercent = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 RawDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	float SourceDamageMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	float TargetDamageMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	float CombinedDamageMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 DamageAfterMultiplier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 ResistancePercent = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 DamageAfterResistance = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 PhysicalArmorBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 MagicalArmorBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 PhysicalArmorDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 MagicalArmorDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 HealthBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 HealthDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace|Damage")
	int32 HealthAfter = 0;
};

/** EVENTHUD01.7 calculation snapshot embedded in every FGridAttackResult. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCombatResolutionTrace
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace")
	FGridCombatAttackTrace Attack;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace")
	FGridCombatDefenseTrace Defense;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Trace")
	FGridCombatDamageTrace Damage;
};
