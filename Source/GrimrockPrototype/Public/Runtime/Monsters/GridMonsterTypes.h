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

    /** Flat damage added after the random damage roll. Attack_Bite uses +1. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    int32 DamageBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack")
    int32 AccuracyBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "1"))
    int32 RangeCells = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|Attack", meta = (ClampMin = "1"))
    int32 ActionPointCost = 1;

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
        if (RangeCells <= 0)
        {
            Errors.Add (TEXT ("RangeCells must be greater than zero."));
        }
        if (ActionPointCost <= 0)
        {
            Errors.Add (TEXT ("ActionPointCost must be greater than zero."));
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
