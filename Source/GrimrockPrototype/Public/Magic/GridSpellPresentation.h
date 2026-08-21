#pragma once

#include "CoreMinimal.h"
#include "Magic/GridSpellTargeting.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "GridSpellPresentation.generated.h"

class AGridCombatProjectileActor;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

UENUM (BlueprintType)
enum class EGridSpellPresentationEvent : uint8
{
    CastStarted       UMETA (DisplayName = "Cast Started"),
    ProjectileLaunched UMETA (DisplayName = "Projectile Launched"),
    Impact            UMETA (DisplayName = "Impact"),
    Completed         UMETA (DisplayName = "Completed")
};

/** Optional presentation-only projectile profile. Visual mesh may remain unassigned. */
USTRUCT (BlueprintType)
struct FGridSpellProjectilePresentationProfile
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Projectile")
    bool bEnabled = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Projectile")
    TSoftObjectPtr<UStaticMesh> VisualMesh;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Spell|Presentation|Projectile",
        meta = (ClampMin = "0.01", ClampMax = "5.0"))
    float TravelDurationSeconds = 0.20f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Projectile")
    FVector VisualScale = FVector::OneVector;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Projectile")
    FRotator RotationOffset = FRotator::ZeroRotator;

    bool IsValid () const
    {
        if (!bEnabled)
        {
            return true;
        }
        return FMath::IsFinite (TravelDurationSeconds) &&
            TravelDurationSeconds > 0.0f &&
            !VisualScale.ContainsNaN () &&
            VisualScale.X > 0.0f &&
            VisualScale.Y > 0.0f &&
            VisualScale.Z > 0.0f;
    }
};

/**
 * Data-driven spell presentation profile.
 * Audio/VFX definitions are reused from the established player attack pipeline.
 */
USTRUCT (BlueprintType)
struct FGridSpellPresentationProfile
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Cast")
    FGridPlayerAttackAudioDefinition CastAudio;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Cast")
    FGridPlayerAttackVFXDefinition CastVFX;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Impact")
    FGridPlayerAttackAudioDefinition ImpactAudio;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Impact")
    FGridPlayerAttackVFXDefinition ImpactVFX;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation|Projectile")
    FGridSpellProjectilePresentationProfile Projectile;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Spell|Presentation|Feedback",
        meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float FeedbackDurationSeconds = 1.25f;

    bool IsValid () const
    {
        return CastAudio.IsValid () &&
            CastVFX.IsValid () &&
            ImpactAudio.IsValid () &&
            ImpactVFX.IsValid () &&
            Projectile.IsValid () &&
            FMath::IsFinite (FeedbackDurationSeconds) &&
            FeedbackDurationSeconds > 0.0f;
    }
};

/** Immutable presentation plan produced after an accepted/resolved cast. */
USTRUCT (BlueprintType)
struct FGridSpellPresentationPlan
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Spell|Presentation")
    FName SpellId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Spell|Presentation")
    FGridSpellResolvedTarget ResolvedTarget;

    UPROPERTY (BlueprintReadOnly, Category = "Spell|Presentation")
    FVector SourceWorldLocation = FVector::ZeroVector;

    UPROPERTY (BlueprintReadOnly, Category = "Spell|Presentation")
    FVector TargetWorldLocation = FVector::ZeroVector;

    UPROPERTY (BlueprintReadOnly, Category = "Spell|Presentation")
    bool bLaunchProjectile = false;

    UPROPERTY (BlueprintReadOnly, Category = "Spell|Presentation")
    float ProjectileTravelDurationSeconds = 0.0f;

    UPROPERTY (BlueprintReadOnly, Category = "Spell|Presentation")
    TArray<EGridSpellPresentationEvent> Events;

    bool IsValid () const
    {
        return !SpellId.IsNone () &&
            !SourceWorldLocation.ContainsNaN () &&
            !TargetWorldLocation.ContainsNaN () &&
            (!bLaunchProjectile || ProjectileTravelDurationSeconds > 0.0f) &&
            !Events.IsEmpty ();
    }
};

/**
 * Pure MON18.6 presentation planning. It owns no health, mana, AP or status state.
 * Projectile timing/trajectory delegate to the validated MON17 combat projectile.
 */
struct GRIMROCKPROTOTYPE_API FGridSpellPresentationService
{
    static bool BuildPlan (
        const FGridSpellDefinition& Definition,
        const FGridSpellResolvedTarget& ResolvedTarget,
        const FGridSpellPresentationProfile& Profile,
        const FVector& SourceWorldLocation,
        const FVector& TargetWorldLocation,
        FGridSpellPresentationPlan& OutPlan);

    static float CalculateProjectileLaunchDelay (
        float ImpactTimeSeconds,
        const FGridSpellPresentationProfile& Profile);

    static FVector EvaluateProjectileTrajectory (
        const FVector& Source,
        const FVector& Target,
        float Alpha);
};

/** Runtime event emitted by UGridSpellPresentationComponent. */
USTRUCT (BlueprintType)
struct FGridSpellPresentationRequest
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    int32 SequenceNumber = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    EGridSpellPresentationEvent Event = EGridSpellPresentationEvent::CastStarted;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    FName SpellId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    FGridSpellResolvedTarget ResolvedTarget;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    TObjectPtr<USoundBase> ResolvedSound = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    TObjectPtr<UNiagaraSystem> ResolvedSystem = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    TObjectPtr<AGridCombatProjectileActor> ProjectileActor = nullptr;
};
