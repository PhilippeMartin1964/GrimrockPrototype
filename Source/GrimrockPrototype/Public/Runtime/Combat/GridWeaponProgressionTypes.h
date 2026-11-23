#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridWeaponProgressionTypes.generated.h"

/**
 * ARMEMENT01.4 - one immutable authored profile per archetype and item level.
 * A resolved profile holds no character level, rarity or crafting state.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponLevelProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity")
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity", meta=(ClampMin="1", ClampMax="20"))
	int32 ItemLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity")
	FString Family;

	/** Informational; equipment slot restrictions continue to live in item definitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity")
	FString HandUsage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	EGridAttackScalingAttribute ScalingAttribute = EGridAttackScalingAttribute::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	int32 AccuracyBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	FGridDamageDiceProfile DamageDice;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Defense")
	int32 PhysicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Defense")
	int32 MagicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	int32 ActionPointCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	int32 RangeCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Inventory")
	float WeightKg = 0.0f;

	/** Rejects invalid authoring, including fake dice on passive shields/focuses. */
	bool IsValidProfile() const;

	/** Returns false for passive profiles. Does not create or resolve a combat action. */
	bool TryBuildAttackDefinition(FGridAttackDefinition& OutAttack) const;
};

/** Absolute correction to a level rule. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponGrowthException
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth", meta=(ClampMin="2", ClampMax="20"))
	int32 ItemLevel = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	int32 Value = 0;
};

/** Value(L) = N1 + floor((L-1) / StepEveryLevels)*StepAmount + sparse exceptions. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponIntegerGrowthRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth", meta=(ClampMin="1", ClampMax="20"))
	int32 StepEveryLevels = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	int32 StepAmount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	TArray<FGridWeaponGrowthException> Exceptions;

	int32 Resolve(int32 ItemLevel, int32 N1Value) const;
	bool IsValid() const;
};
