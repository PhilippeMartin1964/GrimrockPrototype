#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridMonsterTypes.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class USoundBase;

UENUM (BlueprintType)
enum class EGridMonsterAIProfile : uint8
{
    DirectMelee  UMETA (DisplayName = "Direct Melee"),
    FastHarasser UMETA (DisplayName = "Fast Harasser"),
    SlowPressure UMETA (DisplayName = "Slow Pressure"),
    RangedKeeper UMETA (DisplayName = "Ranged Keeper"),
    Ambush       UMETA (DisplayName = "Ambush"),
    PuzzleLinked UMETA (DisplayName = "Puzzle Linked")
};

UENUM (BlueprintType)
enum class EGridMonsterState : uint8
{
    Dormant       UMETA (DisplayName = "Dormant"),
    Idle          UMETA (DisplayName = "Idle"),
    Alert         UMETA (DisplayName = "Alert"),
    Pursuing      UMETA (DisplayName = "Pursuing"),
    Attacking     UMETA (DisplayName = "Attacking"),
    Repositioning UMETA (DisplayName = "Repositioning"),
    Hurt          UMETA (DisplayName = "Hurt"),
    Dead          UMETA (DisplayName = "Dead")
};

USTRUCT (BlueprintType)
struct FGridMonsterAttackDefinition
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    FName AttackId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "0"))
    int32 MinDamage = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "0"))
    int32 MaxDamage = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    int32 AccuracyBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "1"))
    int32 RangeCells = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "0"))
    int32 ActionPointCost = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
    TSoftObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
    FName ImpactNotifyName = TEXT ("Monster.AttackImpact");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
    FName CompleteNotifyName = TEXT ("Monster.ActionComplete");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Audio")
    TSoftObjectPtr<USoundBase> AttackSound;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    TSoftObjectPtr<UNiagaraSystem> ImpactVFX;

    bool IsValidDefinition () const
    {
        return !AttackId.IsNone () &&
            MinDamage >= 0 &&
            MaxDamage >= MinDamage &&
            RangeCells > 0 &&
            ActionPointCost >= 0 &&
            (DamageType == EGridDamageType::Physical || PhysicalSubtype == EGridPhysicalDamageSubtype::None);
    }
};

USTRUCT (BlueprintType)
struct FGridMonsterDamageModifier
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Damage Modifier")
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Damage Modifier")
    EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Damage Modifier", meta = (ClampMin = "0.0"))
    float DamageMultiplier = 1.0f;

    bool IsValidDefinition () const
    {
        return FMath::IsFinite (DamageMultiplier) &&
            DamageMultiplier >= 0.0f &&
            (DamageType == EGridDamageType::Physical || PhysicalSubtype == EGridPhysicalDamageSubtype::None);
    }

    bool Matches (EGridDamageType InDamageType, EGridPhysicalDamageSubtype InPhysicalSubtype) const
    {
        if (DamageType != InDamageType)
        {
            return false;
        }

        return PhysicalSubtype == EGridPhysicalDamageSubtype::None || PhysicalSubtype == InPhysicalSubtype;
    }
};

USTRUCT (BlueprintType)
struct FGridMonsterLootEntry
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Loot")
    FName ItemDefinitionId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Loot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DropChance = 0.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Loot", meta = (ClampMin = "1"))
    int32 MinQuantity = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Loot", meta = (ClampMin = "1"))
    int32 MaxQuantity = 1;

    bool IsValidDefinition () const
    {
        return !ItemDefinitionId.IsNone () &&
            FMath::IsFinite (DropChance) &&
            DropChance >= 0.0f &&
            DropChance <= 1.0f &&
            MinQuantity > 0 &&
            MaxQuantity >= MinQuantity;
    }
};
