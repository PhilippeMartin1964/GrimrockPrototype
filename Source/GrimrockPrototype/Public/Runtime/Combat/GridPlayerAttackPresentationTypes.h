#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridPlayerAttackPresentationTypes.generated.h"

class AGridMonsterActor;
class UNiagaraSystem;
class USceneComponent;
class USoundBase;

UENUM (BlueprintType)
enum class EGridPlayerAttackPresentationEvent : uint8
{
    Attack,
    ImpactHit,
    ImpactMiss
};

UENUM (BlueprintType)
enum class EGridPlayerAttackMotionStyle : uint8
{
    None,
    Swing,
    Thrust,
    Throw,
    Cast
};

USTRUCT (BlueprintType)
struct FGridPlayerAttackAudioDefinition
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    TArray<TSoftObjectPtr<USoundBase>> Sounds;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    float VolumeMultiplier = 1.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    float PitchMin = 1.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    float PitchMax = 1.0f;

    bool IsValid () const;
    bool HasConfiguredSound () const { return Sounds.Num () > 0; }
};

USTRUCT (BlueprintType)
struct FGridPlayerAttackVFXDefinition
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    TArray<TSoftObjectPtr<UNiagaraSystem>> Systems;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    bool bAttachToHeldItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    FName SocketName = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    FVector LocationOffset = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    FRotator RotationOffset = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    FVector Scale = FVector::OneVector;

    bool IsValid () const;
    bool HasConfiguredSystem () const { return Systems.Num () > 0; }
};

USTRUCT (BlueprintType)
struct FGridPlayerAttackPresentationProfile
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Motion")
    EGridPlayerAttackMotionStyle MotionStyle =
        EGridPlayerAttackMotionStyle::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Motion")
    bool bAnimateHeldItem = false;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|Player Attack|Motion",
        meta = (ClampMin = "0.01", ClampMax = "2.0", EditCondition = "bAnimateHeldItem"))
    float MotionDurationSeconds = 0.18f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Motion")
    FVector PeakLocationOffset = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Motion")
    FRotator PeakRotationOffset = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    FGridPlayerAttackAudioDefinition AttackAudio;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    FGridPlayerAttackAudioDefinition ImpactHitAudio;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    FGridPlayerAttackAudioDefinition ImpactMissAudio;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    FGridPlayerAttackVFXDefinition AttackVFX;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    FGridPlayerAttackVFXDefinition ImpactHitVFX;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    FGridPlayerAttackVFXDefinition ImpactMissVFX;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat|Player Attack|Feedback",
        meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float FeedbackDurationSeconds = 1.25f;

    bool IsValid () const;
};

USTRUCT (BlueprintType)
struct FGridPlayerAttackPresentationRequest
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    int32 SequenceNumber = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    EGridPlayerAttackPresentationEvent Event =
        EGridPlayerAttackPresentationEvent::Attack;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    FGridPlayerAttackRequest AttackRequest;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    FGridAttackResult AttackResult;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    bool bHasAttackResult = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    TObjectPtr<AGridMonsterActor> TargetMonster = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    TObjectPtr<USoundBase> ResolvedSound = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    float VolumeMultiplier = 1.0f;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    float PitchMultiplier = 1.0f;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    TObjectPtr<UNiagaraSystem> ResolvedSystem = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    FTransform VFXWorldTransform = FTransform::Identity;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    bool bAttachVFXToHeldItem = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    TObjectPtr<USceneComponent> VFXAttachComponent = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    FName SocketName = NAME_None;
};

UENUM (BlueprintType)
enum class EGridPlayerAttackFeedbackOutcome : uint8
{
    Rejected,
    Miss,
    Hit,
    CriticalHit,
    TargetDefeated
};

USTRUCT (BlueprintType)
struct FGridPlayerAttackFeedbackRequest
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    int32 SequenceNumber = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    bool bAccepted = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    FGridPlayerAttackRequest AttackRequest;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    FGridAttackResult AttackResult;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    EGridPlayerAttackFeedbackOutcome Outcome =
        EGridPlayerAttackFeedbackOutcome::Rejected;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    FText SourceDisplayName;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    FText TargetDisplayName;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    FText PrimaryText;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    FText DetailText;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    FLinearColor SuggestedColor = FLinearColor::White;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    float DurationSeconds = 1.25f;
};

struct GRIMROCKPROTOTYPE_API FGridPlayerAttackFeedbackFormatter
{
    static FText FormatRejectReason (
        EGridPlayerAttackRejectReason RejectReason);
    static FGridPlayerAttackFeedbackRequest FormatResolved (
        const FGridPlayerAttackRequest& Request,
        const FGridAttackResult& Result,
        const FText& SourceDisplayName,
        const FText& TargetDisplayName,
        float DurationSeconds);
    static FText FormatDamageDetail (const FGridAttackResult& Result);
};
