#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "GridPlayerAttackPresentationComponent.generated.h"

class AGridItemActor;
class AGridLevelRuntimeActor;
class AGridMonsterActor;
class AGridThrownItemActor;
class AGrimrockPartyPawn;
class UNiagaraComponent;
class UGridTurnManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridPlayerAttackPresentationRequestedSignature,
    FGridPlayerAttackPresentationRequest, Request);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridPlayerAttackFeedbackRequestedSignature,
    FGridPlayerAttackFeedbackRequest, Request);

UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API
UGridPlayerAttackPresentationComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridPlayerAttackPresentationComponent ();

    virtual void BeginPlay () override;
    virtual void EndPlay (
        const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent (
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    bool bAudioEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Audio")
    bool bNativeAudioPlaybackEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    bool bVFXEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|VFX")
    bool bNativeVFXSpawnEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Feedback")
    bool bNativeFeedbackEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Combat|Player Attack|Throw")
    bool bNativeThrownItemLaunchEnabled = true;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    FGridPlayerAttackPresentationRequest LastPresentationRequest;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    FGridPlayerAttackFeedbackRequest LastFeedbackRequest;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Audio")
    int32 AudioPlaybackRequestCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Audio")
    int32 AudioAttackRequestCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Audio")
    int32 AudioImpactHitRequestCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Audio")
    int32 AudioImpactMissRequestCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|VFX")
    int32 VFXSpawnRequestCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    int32 PresentationAttackCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    int32 PresentationImpactHitCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Presentation")
    int32 PresentationImpactMissCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Feedback")
    int32 FeedbackCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Motion")
    bool bHeldItemMotionStarted = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Throw")
    int32 ThrownItemLaunchRequestCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Throw")
    int32 ThrownItemLaunchStartedCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Combat|Player Attack|Throw")
    bool bThrownItemLaunchStarted = false;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Player Attack|Presentation")
    FGridPlayerAttackPresentationRequestedSignature
        OnPresentationRequested;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Player Attack|Feedback")
    FGridPlayerAttackFeedbackRequestedSignature
        OnFeedbackRequested;

    UFUNCTION (BlueprintCallable, Category = "Combat|Player Attack|Presentation")
    bool InitializePresentation (
        UGridTurnManagerComponent* InTurnManager = nullptr);

    UFUNCTION (BlueprintCallable, Category = "Combat|Player Attack|Presentation")
    void ResetTransientPresentationState ();

    UFUNCTION (BlueprintPure, Category = "Combat|Player Attack|Presentation")
    int32 GetPresentationRequestCountForEvent (
        EGridPlayerAttackPresentationEvent Event) const;

    UFUNCTION (BlueprintPure, Category = "Combat|Player Attack|Audio")
    int32 GetAudioPlaybackRequestCountForEvent (
        EGridPlayerAttackPresentationEvent Event) const;

    UFUNCTION (BlueprintPure, Category = "Combat|Player Attack|Motion")
    bool IsHeldItemMotionActive () const { return bMotionActive; }

private:
    struct FPendingPresentation
    {
        FGridPlayerAttackPresentationProfile Profile;
        TWeakObjectPtr<AGridMonsterActor> TargetMonster;
        TWeakObjectPtr<AGridThrownItemActor> ThrownItemActor;
    };

    UPROPERTY (Transient)
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor = nullptr;

    UPROPERTY (Transient)
    TObjectPtr<UGridTurnManagerComponent> TurnManager = nullptr;

    UPROPERTY (Transient)
    TObjectPtr<AGrimrockPartyPawn> PartyPawn = nullptr;

    UPROPERTY (Transient)
    TArray<TWeakObjectPtr<UNiagaraComponent>>
        ActiveNiagaraComponents;

    TMap<FGuid, FPendingPresentation> PendingPresentations;
    int32 NextPresentationSequenceNumber = 1;
    int32 NextFeedbackSequenceNumber = 1;
    int32 EventOccurrenceCounts[3] = { 0, 0, 0 };

    bool bMotionActive = false;
    float MotionElapsedSeconds = 0.0f;
    float MotionDurationSeconds = 0.0f;
    FVector MotionPeakLocationOffset = FVector::ZeroVector;
    FRotator MotionPeakRotationOffset = FRotator::ZeroRotator;
    FTransform MotionInitialRelativeTransform = FTransform::Identity;
    TWeakObjectPtr<AGridItemActor> AnimatedHeldItemActor;

    UFUNCTION ()
    void HandlePlayerAttackRequested (
        FGridPlayerAttackRequest Request);

    UFUNCTION ()
    void HandlePlayerAttackResolved (
        FGridPlayerAttackRequest Request,
        AGridMonsterActor* TargetMonster,
        FGridAttackResult Result);

    UFUNCTION ()
    void HandlePlayerAttackRejected (
        int32 AttackerCharacterIndex,
        EGridPlayerAttackRejectReason RejectReason);

    UFUNCTION ()
    void HandleCombatEnded (EGridCombatPhase ResultPhase);

    FGridPlayerAttackPresentationProfile ResolveProfile (
        const FGridPlayerAttackRequest& Request) const;
    AGridMonsterActor* ResolveTargetMonster (
        const FGridPlayerAttackRequest& Request) const;
    void EmitPresentation (
        EGridPlayerAttackPresentationEvent Event,
        const FGridPlayerAttackRequest& Request,
        const FGridAttackResult* Result,
        AGridMonsterActor* TargetMonster,
        const FGridPlayerAttackPresentationProfile& Profile);
    void EmitFeedback (
        const FGridPlayerAttackFeedbackRequest& Feedback);
    void StartHeldItemMotion (
        const FGridPlayerAttackRequest& Request,
        const FGridPlayerAttackPresentationProfile& Profile);
    AGridThrownItemActor* StartThrownItemLaunch (
        const FGridPlayerAttackRequest& Request,
        AGridMonsterActor* TargetMonster,
        const FGridPlayerAttackPresentationProfile& Profile);
    void ConfigureThrownItemOutcome (
        AGridThrownItemActor* ThrownItem,
        AGridMonsterActor* TargetMonster,
        const FGridAttackResult& Result);
    void RestoreHeldItemMotion ();
    void StopActiveVFX ();
    FText ResolveCharacterDisplayName (int32 CharacterIndex) const;
    FText ResolveMonsterDisplayName (
        const AGridMonsterActor* Monster) const;
};
