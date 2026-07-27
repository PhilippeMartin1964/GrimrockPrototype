#pragma once

#include "CoreMinimal.h"
#include "GridMonsterAudioTypes.generated.h"

class USoundBase;

UENUM (BlueprintType)
enum class EGridMonsterAudioEvent : uint8
{
    Alert,
    Attack,
    ImpactHit,
    ImpactMiss,
    Hurt,
    Death,
    Idle
};

USTRUCT (BlueprintType)
struct FGridMonsterAudioEventDefinition
{
    GENERATED_BODY ()

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Monster|Audio")
    TArray<TSoftObjectPtr<USoundBase>> Sounds;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Monster|Audio",
        meta = (ClampMin = "0.0"))
    float VolumeMultiplier = 1.0f;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Monster|Audio",
        meta = (ClampMin = "0.01"))
    float PitchMin = 1.0f;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Monster|Audio",
        meta = (ClampMin = "0.01"))
    float PitchMax = 1.0f;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Monster|Audio",
        meta = (ClampMin = "0.0"))
    float CooldownSeconds = 0.0f;

    bool IsValidDefinition () const
    {
        if (!FMath::IsFinite (VolumeMultiplier) ||
            VolumeMultiplier < 0.0f ||
            !FMath::IsFinite (PitchMin) ||
            PitchMin <= 0.0f ||
            !FMath::IsFinite (PitchMax) ||
            PitchMax < PitchMin ||
            !FMath::IsFinite (CooldownSeconds) ||
            CooldownSeconds < 0.0f)
        {
            return false;
        }

        for (const TSoftObjectPtr<USoundBase>& Sound : Sounds)
        {
            if (Sound.IsNull ())
            {
                return false;
            }
        }
        return true;
    }

    bool HasConfiguredSound () const
    {
        return !Sounds.IsEmpty ();
    }
};

USTRUCT (BlueprintType)
struct FGridMonsterAudioPlaybackRequest
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Audio")
    int32 SequenceNumber = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Audio")
    EGridMonsterAudioEvent Event =
        EGridMonsterAudioEvent::Idle;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Audio")
    FName MonsterId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Audio")
    FName AttackId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Audio")
    TObjectPtr<USoundBase> Sound = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Audio")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Audio")
    float VolumeMultiplier = 1.0f;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|Audio")
    float PitchMultiplier = 1.0f;
};

/** Pure presentation-only selection helpers. They never read gameplay RNG. */
class GRIMROCKPROTOTYPE_API FGridMonsterAudioSelector
{
public:
    static int32 BuildPresentationSeed (
        const FGuid& PersistenceId,
        FName MonsterId,
        EGridMonsterAudioEvent Event,
        int32 OccurrenceNumber);

    static int32 SelectVariationIndex (
        int32 PresentationSeed,
        int32 VariationCount);

    static float SelectPitch (
        int32 PresentationSeed,
        float PitchMin,
        float PitchMax);
};
