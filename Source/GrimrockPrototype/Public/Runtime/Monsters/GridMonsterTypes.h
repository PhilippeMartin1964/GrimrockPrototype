#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterAudioTypes.h"
#include "Runtime/Monsters/GridMonsterVFXTypes.h"
#include "GridMonsterTypes.generated.h"

class UAnimMontage;
class UGridItemDefinitionAsset;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

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

/** Generic delivery contract for monster attacks. Execution remains owned by combat/TurnManager systems. */
UENUM (BlueprintType)
enum class EGridMonsterAttackDelivery : uint8
{
    Contact    UMETA (DisplayName = "Contact"),
    Projectile UMETA (DisplayName = "Projectile"),
    Instant    UMETA (DisplayName = "Instant")
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

/** MON14.2 serialized route policy. Route execution belongs to MON14.3. */
UENUM (BlueprintType)
enum class EGridMonsterPatrolMode : uint8
{
    None     UMETA (DisplayName = "None"),
    Loop     UMETA (DisplayName = "Loop"),
    PingPong UMETA (DisplayName = "Ping Pong")
};

/** One authored patrol stop. Facing=None preserves the travel/arrival facing. */
USTRUCT (BlueprintType)
struct FGridMonsterPatrolWaypoint
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Patrol")
    FIntPoint Cell = FIntPoint::ZeroValue;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Patrol")
    EGridEdge Facing = EGridEdge::None;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Monster|Patrol",
        meta = (ClampMin = "0.0"))
    float WaitSeconds = 0.0f;
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

    /** Flat damage added after the random damage roll. Attack_Bite uses +1. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    int32 DamageBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    int32 AccuracyBonus = 0;

    /** Inclusive minimum legal grid distance for this attack. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "1"))
    int32 MinRangeCells = 1;

    /** Inclusive maximum legal grid distance. Kept as RangeCells for serialized asset compatibility. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "1", DisplayName = "Maximum Range Cells"))
    int32 RangeCells = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    EGridMonsterAttackDelivery Delivery = EGridMonsterAttackDelivery::Contact;

    /** Ranged planners/executors must reject the attack when the target is not visible. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    bool bRequiresLineOfSight = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "1"))
    int32 ActionPointCost = 1;

    /** Number of the monster's subsequent turns for which this attack remains unavailable. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "0"))
    int32 CooldownTurns = 0;

    /** Higher values win when more than one legal attack is available at the same distance. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    int32 Priority = 0;

    /** Total presentation duration used by the timer fallback and timeout. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation", meta = (ClampMin = "0.01"))
    float ExpectedDuration = 0.55f;

    /** Fallback impact time when no montage notify is available. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation", meta = (ClampMin = "0.0"))
    float ImpactTimeSeconds = 0.25f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
    TSoftObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
    FName ImpactNotifyName = TEXT ("Monster.AttackImpact");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
    FName CompleteNotifyName = TEXT ("Monster.ActionComplete");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Audio")
    TSoftObjectPtr<USoundBase> AttackSound;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Audio")
    FGridMonsterAudioEventDefinition AttackAudio;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Audio")
    FGridMonsterAudioEventDefinition ImpactHitAudio;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Audio")
    FGridMonsterAudioEventDefinition ImpactMissAudio;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    TSoftObjectPtr<UNiagaraSystem> ImpactVFX;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    FGridMonsterVFXEventDefinition AttackVFXDefinition;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    FGridMonsterVFXEventDefinition ImpactHitVFXDefinition;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    FGridMonsterVFXEventDefinition ImpactMissVFXDefinition;

    /** Optional presentation-only mesh for Delivery=Projectile. Gameplay impact remains grid-authoritative. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Projectile")
    TSoftObjectPtr<UStaticMesh> ProjectileVisualMesh;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Projectile")
    FVector ProjectileVisualScale = FVector::OneVector;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Projectile")
    FRotator ProjectileRotationOffset = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Projectile", meta = (ClampMin = "0.01"))
    float ProjectileTravelDuration = 0.20f;

    bool SupportsDistance (int32 DistanceCells) const
    {
        return DistanceCells >= MinRangeCells && DistanceCells <= RangeCells;
    }

    bool IsRangedAttack () const
    {
        return Delivery != EGridMonsterAttackDelivery::Contact || RangeCells > 1;
    }

    bool ValidateDefinition (FString& OutError) const
    {
        TArray<FString> Errors;
        if (AttackId.IsNone ())
        {
            Errors.Add (TEXT ("AttackId must not be None."));
        }
        if (MinDamage < 0)
        {
            Errors.Add (TEXT ("MinDamage must be non-negative."));
        }
        if (MaxDamage < MinDamage)
        {
            Errors.Add (TEXT ("MaxDamage must be at least MinDamage."));
        }
        if (MinRangeCells <= 0)
        {
            Errors.Add (TEXT ("MinRangeCells must be greater than zero."));
        }
        if (RangeCells < MinRangeCells)
        {
            Errors.Add (TEXT ("RangeCells must be at least MinRangeCells."));
        }
        if (ActionPointCost <= 0)
        {
            Errors.Add (TEXT ("ActionPointCost must be greater than zero."));
        }
        if (CooldownTurns < 0)
        {
            Errors.Add (TEXT ("CooldownTurns must not be negative."));
        }
        if (!FMath::IsFinite (ExpectedDuration) || ExpectedDuration <= 0.0f)
        {
            Errors.Add (TEXT ("ExpectedDuration must be finite and greater than zero."));
        }
        if (!FMath::IsFinite (ImpactTimeSeconds) ||
            ImpactTimeSeconds < 0.0f ||
            ImpactTimeSeconds > ExpectedDuration)
        {
            Errors.Add (TEXT ("ImpactTimeSeconds must be finite and between zero and ExpectedDuration."));
        }
        if (DamageType != EGridDamageType::Physical &&
            PhysicalSubtype != EGridPhysicalDamageSubtype::None)
        {
            Errors.Add (TEXT ("PhysicalSubtype must be None for non-physical damage."));
        }
        if (!FMath::IsFinite (ProjectileTravelDuration) || ProjectileTravelDuration <= 0.0f)
        {
            Errors.Add (TEXT ("ProjectileTravelDuration must be finite and greater than zero."));
        }
        if (!FMath::IsFinite (ProjectileVisualScale.X) ||
            !FMath::IsFinite (ProjectileVisualScale.Y) ||
            !FMath::IsFinite (ProjectileVisualScale.Z) ||
            ProjectileVisualScale.X <= 0.0f ||
            ProjectileVisualScale.Y <= 0.0f ||
            ProjectileVisualScale.Z <= 0.0f)
        {
            Errors.Add (TEXT ("ProjectileVisualScale components must be finite and greater than zero."));
        }

        const auto ValidateAudio = [&Errors] (
            const TCHAR* FieldName,
            const FGridMonsterAudioEventDefinition& Definition)
        {
            FString Error;
            if (!Definition.ValidateDefinition (Error))
            {
                Errors.Add (FString::Printf (
                    TEXT ("%s.%s"),
                    FieldName,
                    *Error));
            }
        };
        ValidateAudio (TEXT ("AttackAudio"), AttackAudio);
        ValidateAudio (TEXT ("ImpactHitAudio"), ImpactHitAudio);
        ValidateAudio (TEXT ("ImpactMissAudio"), ImpactMissAudio);

        const auto ValidateVFX = [&Errors] (
            const TCHAR* FieldName,
            const FGridMonsterVFXEventDefinition& Definition)
        {
            FString Error;
            if (!Definition.ValidateDefinition (Error))
            {
                Errors.Add (FString::Printf (
                    TEXT ("%s.%s"),
                    FieldName,
                    *Error));
            }
        };
        ValidateVFX (TEXT ("AttackVFXDefinition"), AttackVFXDefinition);
        ValidateVFX (TEXT ("ImpactHitVFXDefinition"), ImpactHitVFXDefinition);
        ValidateVFX (TEXT ("ImpactMissVFXDefinition"), ImpactMissVFXDefinition);

        OutError = FString::Join (Errors, TEXT (" "));
        return Errors.IsEmpty ();
    }

    bool IsValidDefinition () const
    {
        FString Error;
        return ValidateDefinition (Error);
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

    /** Optional direct definition used by MON8 world loot placement. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Loot")
    TObjectPtr<UGridItemDefinitionAsset> ItemDefinitionAsset = nullptr;

    FName GetResolvedItemDefinitionId () const;
    bool IsValidDefinition () const;
};