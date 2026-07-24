#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridCombatTypes.generated.h"

UENUM (BlueprintType)
enum class EGridPhysicalDamageSubtype : uint8
{
    None        UMETA (DisplayName = "None"),
    Slashing    UMETA (DisplayName = "Slashing"),
    Piercing    UMETA (DisplayName = "Piercing"),
    Bludgeoning UMETA (DisplayName = "Bludgeoning")
};

UENUM (BlueprintType)
enum class EGridCombatActionType : uint8
{
    None          UMETA (DisplayName = "None"),
    Move          UMETA (DisplayName = "Move"),
    Turn          UMETA (DisplayName = "Turn"),
    MeleeAttack   UMETA (DisplayName = "Melee Attack"),
    RangedAttack  UMETA (DisplayName = "Ranged Attack"),
    Ability       UMETA (DisplayName = "Ability"),
    Defend        UMETA (DisplayName = "Defend"),
    Wait          UMETA (DisplayName = "Wait"),
    Retreat       UMETA (DisplayName = "Retreat"),
    Die           UMETA (DisplayName = "Die")
};

UENUM (BlueprintType)
enum class EGridCombatPhase : uint8
{
    Exploration    UMETA (DisplayName = "Exploration"),
    StartingCombat UMETA (DisplayName = "Starting Combat"),
    PlayerPhase    UMETA (DisplayName = "Player Phase"),
    EnemyPhase     UMETA (DisplayName = "Enemy Phase"),
    EndingRound    UMETA (DisplayName = "Ending Round"),
    Victory        UMETA (DisplayName = "Victory"),
    Defeat         UMETA (DisplayName = "Defeat")
};

USTRUCT (BlueprintType)
struct FGridAttackSourceStats
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Source")
    int32 Accuracy = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Source")
    int32 DamageBonus = 0;
};

USTRUCT (BlueprintType)
struct FGridAttackTargetStats
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target")
    int32 Evasion = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
    int32 CurrentHealth = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
    int32 PhysicalArmor = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0"))
    int32 MagicalArmor = 0;

    /** Percentage mitigation. Negative values represent vulnerability. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "-100", ClampMax = "100"))
    int32 ResistancePercent = 0;

    /** Species, status or environmental multiplier applied before resistance. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Target", meta = (ClampMin = "0.0"))
    float DamageMultiplier = 1.0f;
};

USTRUCT (BlueprintType)
struct FGridAttackDefinition
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
    EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack", meta = (ClampMin = "0"))
    int32 MinDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack", meta = (ClampMin = "0"))
    int32 MaxDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
    int32 AccuracyBonus = 0;

    bool IsValid () const
    {
        return MinDamage >= 0 &&
            MaxDamage >= MinDamage &&
            (DamageType == EGridDamageType::Physical || PhysicalSubtype == EGridPhysicalDamageSubtype::None);
    }
};

USTRUCT (BlueprintType)
struct FGridCombatAction
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FGuid ActionId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    EGridCombatActionType Type = EGridCombatActionType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FGuid SourceActorId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FGuid TargetActorId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    int32 TargetCharacterIndex = INDEX_NONE;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FIntPoint TargetCell = FIntPoint::ZeroValue;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action")
    FName AttackId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Action", meta = (ClampMin = "0"))
    int32 ActionPointCost = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
    bool bHit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
    bool bCriticalHit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
    int32 RolledDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Outcome")
    bool bOutcomeCommitted = false;

    bool IsValid () const
    {
        return Type != EGridCombatActionType::None && ActionPointCost >= 0;
    }
};

USTRUCT (BlueprintType)
struct FGridAttackResult
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    bool bHit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    bool bCriticalHit = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 NaturalAttackRoll = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 AttackRoll = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 DefenseValue = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 RawDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 ResistancePercent = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    float DamageMultiplier = 1.0f;

    /** Damage remaining after multipliers and percentage resistance, before armor. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 DamageAfterModifiers = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 PhysicalArmorDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 MagicalArmorDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 HealthDamage = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 TargetHealthBefore = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    int32 TargetHealthAfter = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Result")
    EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

    int32 GetTotalAppliedDamage () const
    {
        return PhysicalArmorDamage + MagicalArmorDamage + HealthDamage;
    }
};
