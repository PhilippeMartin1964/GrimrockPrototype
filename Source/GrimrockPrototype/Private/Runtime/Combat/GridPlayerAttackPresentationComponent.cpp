#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC (
    LogGridPlayerAttackPresentation,
    Log,
    All);

namespace
{
    constexpr uint32 MON114PresentationSeedSalt = 0x4D313134u;
    constexpr uint32 MON114PitchSeedSalt = 0x50495443u;

    int32 GetEventIndex (EGridPlayerAttackPresentationEvent Event)
    {
        const int32 Index = static_cast<int32> (Event);
        return Index >= 0 && Index < 3 ? Index : INDEX_NONE;
    }

    int32 BuildPresentationSeed (
        const FGridPlayerAttackRequest& Request,
        EGridPlayerAttackPresentationEvent Event,
        int32 OccurrenceNumber)
    {
        uint32 Seed = MON114PresentationSeedSalt;
        Seed = HashCombine (Seed, GetTypeHash (Request.AttackerCharacterId));
        Seed = HashCombine (Seed, GetTypeHash (Request.AttackId));
        Seed = HashCombine (
            Seed,
            GetTypeHash (static_cast<uint8> (Event)));
        Seed = HashCombine (
            Seed,
            GetTypeHash (FMath::Max (0, OccurrenceNumber)));
        return static_cast<int32> (Seed);
    }

    template <typename ObjectType>
    ObjectType* ResolveVariation (
        const TArray<TSoftObjectPtr<ObjectType>>& Variations,
        int32 Seed)
    {
        if (Variations.Num () == 0)
        {
            return nullptr;
        }
        FRandomStream Stream (Seed);
        const int32 Index = Stream.RandRange (
            0,
            Variations.Num () - 1);
        return Variations.IsValidIndex (Index)
            ? Variations[Index].LoadSynchronous ()
            : nullptr;
    }

    FGridPlayerAttackPresentationProfile MakeUnarmedProfile ()
    {
        FGridPlayerAttackPresentationProfile Profile;
        Profile.MotionStyle =
            EGridPlayerAttackMotionStyle::Thrust;
        Profile.bAnimateHeldItem = false;
        Profile.MotionDurationSeconds = 0.15f;
        Profile.FeedbackDurationSeconds = 1.25f;
        return Profile;
    }
}

UGridPlayerAttackPresentationComponent::
UGridPlayerAttackPresentationComponent ()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGridPlayerAttackPresentationComponent::BeginPlay ()
{
    Super::BeginPlay ();
    InitializePresentation ();
}

void UGridPlayerAttackPresentationComponent::EndPlay (
    const EEndPlayReason::Type EndPlayReason)
{
    if (TurnManager)
    {
        TurnManager->OnPlayerAttackRequested.RemoveDynamic (
            this,
            &UGridPlayerAttackPresentationComponent::HandlePlayerAttackRequested);
        TurnManager->OnPlayerAttackResolved.RemoveDynamic (
            this,
            &UGridPlayerAttackPresentationComponent::HandlePlayerAttackResolved);
        TurnManager->OnPlayerAttackRejected.RemoveDynamic (
            this,
            &UGridPlayerAttackPresentationComponent::HandlePlayerAttackRejected);
        TurnManager->OnCombatEnded.RemoveDynamic (
            this,
            &UGridPlayerAttackPresentationComponent::HandleCombatEnded);
    }
    RestoreHeldItemMotion ();
    StopActiveVFX ();
    PendingPresentations.Reset ();
    Super::EndPlay (EndPlayReason);
}

void UGridPlayerAttackPresentationComponent::TickComponent (
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent (DeltaTime, TickType, ThisTickFunction);
    if (!bMotionActive)
    {
        SetComponentTickEnabled (false);
        return;
    }

    AGridItemActor* HeldItem = AnimatedHeldItemActor.Get ();
    if (!IsValid (HeldItem) ||
        !PartyPawn ||
        PartyPawn->HeldItemActor.Get () != HeldItem)
    {
        RestoreHeldItemMotion ();
        return;
    }

    MotionElapsedSeconds += FMath::Max (0.0f, DeltaTime);
    const float Alpha = FMath::Clamp (
        MotionElapsedSeconds /
            FMath::Max (0.01f, MotionDurationSeconds),
        0.0f,
        1.0f);
    const float MotionAlpha = FMath::Sin (PI * Alpha);
    FTransform AnimatedTransform = MotionInitialRelativeTransform;
    AnimatedTransform.SetLocation (
        MotionInitialRelativeTransform.GetLocation () +
        MotionPeakLocationOffset * MotionAlpha);
    const FRotator InitialRotation =
        MotionInitialRelativeTransform.Rotator ();
    AnimatedTransform.SetRotation (
        FQuat ((InitialRotation +
            MotionPeakRotationOffset * MotionAlpha).GetNormalized ()));
    HeldItem->SetActorRelativeTransform (AnimatedTransform);

    if (Alpha >= 1.0f)
    {
        RestoreHeldItemMotion ();
    }
}

bool UGridPlayerAttackPresentationComponent::InitializePresentation (
    UGridTurnManagerComponent* InTurnManager)
{
    RuntimeActor = Cast<AGridLevelRuntimeActor> (GetOwner ());
    TurnManager = InTurnManager
        ? InTurnManager
        : GetOwner ()
            ? GetOwner ()->FindComponentByClass<
                UGridTurnManagerComponent> ()
            : nullptr;
    PartyPawn = TurnManager ? TurnManager->PartyPawn : nullptr;
    if (!TurnManager)
    {
        return false;
    }

    TurnManager->OnPlayerAttackRequested.AddUniqueDynamic (
        this,
        &UGridPlayerAttackPresentationComponent::HandlePlayerAttackRequested);
    TurnManager->OnPlayerAttackResolved.AddUniqueDynamic (
        this,
        &UGridPlayerAttackPresentationComponent::HandlePlayerAttackResolved);
    TurnManager->OnPlayerAttackRejected.AddUniqueDynamic (
        this,
        &UGridPlayerAttackPresentationComponent::HandlePlayerAttackRejected);
    TurnManager->OnCombatEnded.AddUniqueDynamic (
        this,
        &UGridPlayerAttackPresentationComponent::HandleCombatEnded);
    return true;
}

void UGridPlayerAttackPresentationComponent::
ResetTransientPresentationState ()
{
    RestoreHeldItemMotion ();
    StopActiveVFX ();
    PendingPresentations.Reset ();
    NextPresentationSequenceNumber = 1;
    NextFeedbackSequenceNumber = 1;
    EventOccurrenceCounts[0] = 0;
    EventOccurrenceCounts[1] = 0;
    EventOccurrenceCounts[2] = 0;
    PresentationAttackCount = 0;
    PresentationImpactHitCount = 0;
    PresentationImpactMissCount = 0;
    FeedbackCount = 0;
    AudioPlaybackRequestCount = 0;
    AudioAttackRequestCount = 0;
    AudioImpactHitRequestCount = 0;
    AudioImpactMissRequestCount = 0;
    VFXSpawnRequestCount = 0;
    bHeldItemMotionStarted = false;
    ThrownItemLaunchRequestCount = 0;
    ThrownItemLaunchStartedCount = 0;
    bThrownItemLaunchStarted = false;
    LastPresentationRequest =
        FGridPlayerAttackPresentationRequest ();
    LastFeedbackRequest = FGridPlayerAttackFeedbackRequest ();
}

int32 UGridPlayerAttackPresentationComponent::
GetAudioPlaybackRequestCountForEvent (
    EGridPlayerAttackPresentationEvent Event) const
{
    switch (Event)
    {
    case EGridPlayerAttackPresentationEvent::Attack:
        return AudioAttackRequestCount;
    case EGridPlayerAttackPresentationEvent::ImpactHit:
        return AudioImpactHitRequestCount;
    case EGridPlayerAttackPresentationEvent::ImpactMiss:
        return AudioImpactMissRequestCount;
    default:
        return 0;
    }
}

int32 UGridPlayerAttackPresentationComponent::
GetPresentationRequestCountForEvent (
    EGridPlayerAttackPresentationEvent Event) const
{
    switch (Event)
    {
    case EGridPlayerAttackPresentationEvent::Attack:
        return PresentationAttackCount;
    case EGridPlayerAttackPresentationEvent::ImpactHit:
        return PresentationImpactHitCount;
    case EGridPlayerAttackPresentationEvent::ImpactMiss:
        return PresentationImpactMissCount;
    default:
        return 0;
    }
}

void UGridPlayerAttackPresentationComponent::
HandlePlayerAttackRequested (FGridPlayerAttackRequest Request)
{
    bHeldItemMotionStarted = false;
    bThrownItemLaunchStarted = false;
    if (!PartyPawn && TurnManager)
    {
        PartyPawn = TurnManager->PartyPawn;
    }
    const FGridPlayerAttackPresentationProfile Profile =
        ResolveProfile (Request);
    AGridMonsterActor* TargetMonster =
        ResolveTargetMonster (Request);
    FPendingPresentation Pending;
    Pending.Profile = Profile;
    Pending.TargetMonster = TargetMonster;
    PendingPresentations.Add (Request.RequestId, Pending);
    EmitPresentation (
        EGridPlayerAttackPresentationEvent::Attack,
        Request,
        nullptr,
        TargetMonster,
        Profile);
    if (Profile.MotionStyle ==
        EGridPlayerAttackMotionStyle::Throw)
    {
        if (FPendingPresentation* StoredPending =
            PendingPresentations.Find (Request.RequestId))
        {
            StoredPending->ThrownItemActor =
                StartThrownItemLaunch (
                    Request,
                    TargetMonster,
                    Profile);
        }
    }
    else
    {
        StartHeldItemMotion (Request, Profile);
    }
}

void UGridPlayerAttackPresentationComponent::
HandlePlayerAttackResolved (
    FGridPlayerAttackRequest Request,
    AGridMonsterActor* TargetMonster,
    FGridAttackResult Result)
{
    FGridPlayerAttackPresentationProfile Profile =
        MakeUnarmedProfile ();
    AGridThrownItemActor* ThrownItem = nullptr;
    if (const FPendingPresentation* Pending =
        PendingPresentations.Find (Request.RequestId))
    {
        Profile = Pending->Profile;
        ThrownItem = Pending->ThrownItemActor.Get ();
    }
    else
    {
        Profile = ResolveProfile (Request);
    }

    ConfigureThrownItemOutcome (
        ThrownItem,
        TargetMonster,
        Result);
    EmitPresentation (
        Result.bHit
            ? EGridPlayerAttackPresentationEvent::ImpactHit
            : EGridPlayerAttackPresentationEvent::ImpactMiss,
        Request,
        &Result,
        TargetMonster,
        Profile);

    FGridPlayerAttackFeedbackRequest Feedback =
        FGridPlayerAttackFeedbackFormatter::FormatResolved (
            Request,
            Result,
            ResolveCharacterDisplayName (
                Request.AttackerCharacterIndex),
            ResolveMonsterDisplayName (TargetMonster),
            Profile.FeedbackDurationSeconds);
    EmitFeedback (Feedback);
    PendingPresentations.Remove (Request.RequestId);
}

void UGridPlayerAttackPresentationComponent::
HandlePlayerAttackRejected (
    int32 AttackerCharacterIndex,
    EGridPlayerAttackRejectReason RejectReason)
{
    bHeldItemMotionStarted = false;
    bThrownItemLaunchStarted = false;
    FGridPlayerAttackFeedbackRequest Feedback;
    Feedback.bAccepted = false;
    Feedback.RejectReason = RejectReason;
    Feedback.Outcome =
        EGridPlayerAttackFeedbackOutcome::Rejected;
    Feedback.AttackRequest.AttackerCharacterIndex =
        AttackerCharacterIndex;
    Feedback.SourceDisplayName =
        ResolveCharacterDisplayName (AttackerCharacterIndex);
    Feedback.PrimaryText =
        FGridPlayerAttackFeedbackFormatter::FormatRejectReason (
            RejectReason);
    Feedback.SuggestedColor =
        FLinearColor (0.9f, 0.25f, 0.15f);
    Feedback.DurationSeconds = 1.25f;
    EmitFeedback (Feedback);
}

void UGridPlayerAttackPresentationComponent::HandleCombatEnded (
    EGridCombatPhase ResultPhase)
{
    RestoreHeldItemMotion ();
    StopActiveVFX ();
    PendingPresentations.Reset ();
}

FGridPlayerAttackPresentationProfile
UGridPlayerAttackPresentationComponent::ResolveProfile (
    const FGridPlayerAttackRequest& Request) const
{
    if (Request.OffensiveItemDefinitionId.IsNone ())
    {
        return MakeUnarmedProfile ();
    }
    const UGridPartyInventoryComponent* Inventory =
        PartyPawn ? PartyPawn->PartyInventoryComponent.Get () : nullptr;
    const UGridItemDefinitionAsset* Definition =
        Inventory
            ? Inventory->FindItemDefinition (
                Request.OffensiveItemDefinitionId)
            : nullptr;
    return Definition &&
        Definition->HasValidPlayerAttackPresentation ()
            ? Definition->PlayerAttackPresentationProfile
            : FGridPlayerAttackPresentationProfile ();
}

AGridMonsterActor*
UGridPlayerAttackPresentationComponent::ResolveTargetMonster (
    const FGridPlayerAttackRequest& Request) const
{
    UWorld* World = GetWorld ();
    if (!World)
    {
        return nullptr;
    }
    for (TActorIterator<AGridMonsterActor> It (World); It; ++It)
    {
        if (It->ResolvePersistenceId () == Request.TargetMonsterId)
        {
            return *It;
        }
    }
    return nullptr;
}

void UGridPlayerAttackPresentationComponent::EmitPresentation (
    EGridPlayerAttackPresentationEvent Event,
    const FGridPlayerAttackRequest& Request,
    const FGridAttackResult* Result,
    AGridMonsterActor* TargetMonster,
    const FGridPlayerAttackPresentationProfile& Profile)
{
    const int32 EventIndex = GetEventIndex (Event);
    if (EventIndex == INDEX_NONE)
    {
        return;
    }
    const int32 Occurrence = ++EventOccurrenceCounts[EventIndex];
    const int32 Seed =
        BuildPresentationSeed (Request, Event, Occurrence);
    const FGridPlayerAttackAudioDefinition* Audio = nullptr;
    const FGridPlayerAttackVFXDefinition* VFX = nullptr;
    switch (Event)
    {
    case EGridPlayerAttackPresentationEvent::Attack:
        Audio = &Profile.AttackAudio;
        VFX = &Profile.AttackVFX;
        ++PresentationAttackCount;
        break;
    case EGridPlayerAttackPresentationEvent::ImpactHit:
        Audio = &Profile.ImpactHitAudio;
        VFX = &Profile.ImpactHitVFX;
        ++PresentationImpactHitCount;
        break;
    case EGridPlayerAttackPresentationEvent::ImpactMiss:
        Audio = &Profile.ImpactMissAudio;
        VFX = &Profile.ImpactMissVFX;
        ++PresentationImpactMissCount;
        break;
    }

    FGridPlayerAttackPresentationRequest Presentation;
    Presentation.SequenceNumber =
        NextPresentationSequenceNumber++;
    Presentation.Event = Event;
    Presentation.AttackRequest = Request;
    Presentation.TargetMonster = TargetMonster;
    if (Result)
    {
        Presentation.AttackResult = *Result;
        Presentation.bHasAttackResult = true;
    }

    const FVector SourceLocation =
        PartyPawn && PartyPawn->Camera
            ? PartyPawn->Camera->GetComponentLocation ()
            : PartyPawn
                ? PartyPawn->GetActorLocation ()
                : FVector::ZeroVector;
    const FVector EventLocation =
        Event == EGridPlayerAttackPresentationEvent::Attack
            ? SourceLocation
            : TargetMonster
                ? TargetMonster->GetActorLocation ()
                : SourceLocation;

    if (bAudioEnabled && Audio && Audio->IsValid ())
    {
        Presentation.ResolvedSound =
            ResolveVariation (Audio->Sounds, Seed);
        Presentation.VolumeMultiplier =
            Audio->VolumeMultiplier;
        FRandomStream PitchStream (
            static_cast<int32> (HashCombine (
                static_cast<uint32> (Seed),
                MON114PitchSeedSalt)));
        Presentation.PitchMultiplier =
            FMath::IsNearlyEqual (Audio->PitchMin, Audio->PitchMax)
                ? Audio->PitchMin
                : PitchStream.FRandRange (
                    Audio->PitchMin,
                    Audio->PitchMax);
        if (Presentation.ResolvedSound)
        {
            ++AudioPlaybackRequestCount;
            switch (Event)
            {
            case EGridPlayerAttackPresentationEvent::Attack:
                ++AudioAttackRequestCount;
                break;
            case EGridPlayerAttackPresentationEvent::ImpactHit:
                ++AudioImpactHitRequestCount;
                break;
            case EGridPlayerAttackPresentationEvent::ImpactMiss:
                ++AudioImpactMissRequestCount;
                break;
            }
            if (bNativeAudioPlaybackEnabled && GetWorld ())
            {
                UGameplayStatics::PlaySoundAtLocation (
                    this,
                    Presentation.ResolvedSound,
                    EventLocation,
                    Presentation.VolumeMultiplier,
                    Presentation.PitchMultiplier);
            }
        }
    }

    USceneComponent* AttachComponent = nullptr;
    if (Event == EGridPlayerAttackPresentationEvent::Attack)
    {
        if (VFX && VFX->bAttachToHeldItem &&
            PartyPawn &&
            PartyPawn->HeldItemActor &&
            PartyPawn->GetHeldItemDefinitionId () ==
                Request.OffensiveItemDefinitionId)
        {
            AttachComponent =
                PartyPawn->HeldItemActor->GetRootComponent ();
            Presentation.bAttachVFXToHeldItem =
                AttachComponent != nullptr;
        }
        if (!AttachComponent && PartyPawn)
        {
            AttachComponent = PartyPawn->Camera
                ? static_cast<USceneComponent*> (
                    PartyPawn->Camera)
                : PartyPawn->GetRootComponent ();
        }
    }

    if (VFX && VFX->IsValid ())
    {
        Presentation.ResolvedSystem =
            bVFXEnabled
                ? ResolveVariation (VFX->Systems, Seed)
                : nullptr;
        Presentation.VFXAttachComponent = AttachComponent;
        Presentation.SocketName = VFX->SocketName;
        if (Event == EGridPlayerAttackPresentationEvent::Attack &&
            AttachComponent)
        {
            Presentation.VFXWorldTransform =
                FTransform (
                    VFX->RotationOffset,
                    VFX->LocationOffset,
                    VFX->Scale) *
                AttachComponent->GetSocketTransform (
                    VFX->SocketName,
                    RTS_World);
        }
        else
        {
            const FRotator FacingRotation =
                PartyPawn && TargetMonster
                    ? (TargetMonster->GetActorLocation () -
                        PartyPawn->GetActorLocation ()).Rotation ()
                    : FRotator::ZeroRotator;
            Presentation.VFXWorldTransform = FTransform (
                (FacingRotation + VFX->RotationOffset)
                    .GetNormalized (),
                EventLocation + VFX->LocationOffset,
                VFX->Scale);
        }

        if (Presentation.ResolvedSystem)
        {
            ++VFXSpawnRequestCount;
            if (bNativeVFXSpawnEnabled && GetWorld ())
            {
                UNiagaraComponent* Spawned = nullptr;
                if (Event ==
                        EGridPlayerAttackPresentationEvent::Attack &&
                    AttachComponent)
                {
                    Spawned =
                        UNiagaraFunctionLibrary::
                            SpawnSystemAttached (
                                Presentation.ResolvedSystem,
                                AttachComponent,
                                VFX->SocketName,
                                VFX->LocationOffset,
                                VFX->RotationOffset,
                                VFX->Scale,
                                EAttachLocation::KeepRelativeOffset,
                                true,
                                ENCPoolMethod::None,
                                true,
                                true);
                }
                else
                {
                    Spawned =
                        UNiagaraFunctionLibrary::
                            SpawnSystemAtLocation (
                                this,
                                Presentation.ResolvedSystem,
                                Presentation.VFXWorldTransform
                                    .GetLocation (),
                                Presentation.VFXWorldTransform
                                    .Rotator (),
                                Presentation.VFXWorldTransform
                                    .GetScale3D (),
                                true,
                                true,
                                ENCPoolMethod::None,
                                true);
                }
                if (Spawned)
                {
                    ActiveNiagaraComponents.Add (Spawned);
                }
            }
        }
    }

    LastPresentationRequest = Presentation;
    OnPresentationRequested.Broadcast (Presentation);
    UE_LOG (
        LogGridPlayerAttackPresentation,
        Log,
        TEXT ("[GridPlayerAttackPresentation] Seq=%d Event=%s Attack=%s Sound=%s Niagara=%s"),
        Presentation.SequenceNumber,
        *UEnum::GetValueAsString (Event),
        *Request.AttackId.ToString (),
        *GetNameSafe (Presentation.ResolvedSound),
        *GetNameSafe (Presentation.ResolvedSystem));
}

void UGridPlayerAttackPresentationComponent::EmitFeedback (
    const FGridPlayerAttackFeedbackRequest& InFeedback)
{
    FGridPlayerAttackFeedbackRequest Feedback = InFeedback;
    Feedback.SequenceNumber = NextFeedbackSequenceNumber++;
    LastFeedbackRequest = Feedback;
    ++FeedbackCount;
    OnFeedbackRequested.Broadcast (Feedback);
    if (bNativeFeedbackEnabled && RuntimeActor)
    {
        RuntimeActor->ShowCombatFeedback (Feedback);
    }
}

void UGridPlayerAttackPresentationComponent::StartHeldItemMotion (
    const FGridPlayerAttackRequest& Request,
    const FGridPlayerAttackPresentationProfile& Profile)
{
    RestoreHeldItemMotion ();
    if (Profile.MotionStyle ==
            EGridPlayerAttackMotionStyle::Throw ||
        !Profile.bAnimateHeldItem ||
        Request.OffensiveItemDefinitionId.IsNone () ||
        !PartyPawn ||
        !PartyPawn->HeldItemActor ||
        PartyPawn->GetHeldItemDefinitionId () !=
            Request.OffensiveItemDefinitionId ||
        !PartyPawn->PartyInventoryComponent ||
        PartyPawn->PartyInventoryComponent->
            GetSelectedCharacterIndex () !=
            Request.AttackerCharacterIndex)
    {
        return;
    }

    AnimatedHeldItemActor = PartyPawn->HeldItemActor;
    MotionInitialRelativeTransform =
        PartyPawn->HeldItemActor->GetRootComponent ()
            ? PartyPawn->HeldItemActor->GetRootComponent ()->
                GetRelativeTransform ()
            : FTransform::Identity;
    MotionDurationSeconds = Profile.MotionDurationSeconds;
    MotionPeakLocationOffset = Profile.PeakLocationOffset;
    MotionPeakRotationOffset = Profile.PeakRotationOffset;
    MotionElapsedSeconds = 0.0f;
    bMotionActive = true;
    bHeldItemMotionStarted = true;
    SetComponentTickEnabled (true);
}

AGridThrownItemActor*
UGridPlayerAttackPresentationComponent::StartThrownItemLaunch (
    const FGridPlayerAttackRequest& Request,
    AGridMonsterActor* TargetMonster,
    const FGridPlayerAttackPresentationProfile& Profile)
{
    RestoreHeldItemMotion ();
    const bool bEquippedSource =
        Request.OffensiveEquipmentSlot ==
            EGridEquipmentSlot::MainHand ||
        Request.OffensiveEquipmentSlot ==
            EGridEquipmentSlot::OffHand;
    const bool bInventorySource =
        Request.OffensiveEquipmentSlot ==
            EGridEquipmentSlot::None;
    if (Profile.MotionStyle !=
            EGridPlayerAttackMotionStyle::Throw ||
        Request.OffensiveItemDefinitionId.IsNone () ||
        (!bEquippedSource && !bInventorySource))
    {
        return nullptr;
    }

    ++ThrownItemLaunchRequestCount;
    if (IsValid (Request.PreparedThrownItemActor.Get ()))
    {
        ++ThrownItemLaunchStartedCount;
        bThrownItemLaunchStarted = true;
        return Request.PreparedThrownItemActor.Get ();
    }

    // Equipped projectiles are gameplay state committed by the TurnManager.
    // Presentation must never extract or consume an equipped item as a
    // side-effect of receiving a visual event.
    if (bEquippedSource)
    {
        UE_LOG (
            LogGridPlayerAttackPresentation,
            Warning,
            TEXT ("[GridPlayerAttackPresentation] MissingPreparedThrow Attack=%s Item=%s Character=%d Slot=%s"),
            *Request.AttackId.ToString (),
            *Request.OffensiveItemDefinitionId.ToString (),
            Request.AttackerCharacterIndex,
            *UEnum::GetValueAsString (
                Request.OffensiveEquipmentSlot));
        return nullptr;
    }

    if (!bNativeThrownItemLaunchEnabled ||
        !Profile.bAnimateHeldItem ||
        !PartyPawn ||
        !IsValid (TargetMonster))
    {
        return nullptr;
    }

    const FVector TargetLocation =
        TargetMonster->CollisionComponent
            ? TargetMonster->CollisionComponent->Bounds.Origin
            : TargetMonster->GetActorLocation ();
    AGridThrownItemActor* ThrownItem =
        PartyPawn->TryLaunchInventoryItemForAttack (
            Request.AttackerCharacterIndex,
            Request.OffensiveItemDefinitionId,
            TargetLocation,
            Request.PartyCell);
    if (!ThrownItem)
    {
        UE_LOG (
            LogGridPlayerAttackPresentation,
            Warning,
            TEXT ("[GridPlayerAttackPresentation] ThrownLaunch=false Attack=%s Item=%s Character=%d Slot=%s"),
            *Request.AttackId.ToString (),
            *Request.OffensiveItemDefinitionId.ToString (),
            Request.AttackerCharacterIndex,
            *UEnum::GetValueAsString (
                Request.OffensiveEquipmentSlot));
        return nullptr;
    }

    ++ThrownItemLaunchStartedCount;
    bThrownItemLaunchStarted = true;
    UE_LOG (
        LogGridPlayerAttackPresentation,
        Log,
        TEXT ("[GridPlayerAttackPresentation] ThrownLaunch=true Attack=%s Item=%s RuntimeId=%s"),
        *Request.AttackId.ToString (),
        *Request.OffensiveItemDefinitionId.ToString (),
        *ThrownItem->ThrownItemInstance.RuntimeObjectId.ToString ());
    return ThrownItem;
}

void UGridPlayerAttackPresentationComponent::
ConfigureThrownItemOutcome (
    AGridThrownItemActor* ThrownItem,
    AGridMonsterActor* TargetMonster,
    const FGridAttackResult& Result)
{
    if (!IsValid (ThrownItem))
    {
        return;
    }

    FVector TargetLocation =
        ThrownItem->GetActorLocation ();
    float AcceptanceRadius = 24.0f;
    if (TargetMonster)
    {
        TargetLocation =
            TargetMonster->CollisionComponent
                ? TargetMonster->CollisionComponent->Bounds.Origin
                : TargetMonster->GetActorLocation ();
        if (TargetMonster->CollisionComponent)
        {
            AcceptanceRadius = FMath::Clamp (
                TargetMonster->CollisionComponent->
                    GetScaledBoxExtent ().GetMin () * 0.75f,
                12.0f,
                75.0f);
        }
    }
    ThrownItem->ConfigureCombatPresentationTarget (
        Result.bHit && IsValid (TargetMonster),
        TargetLocation,
        AcceptanceRadius);
}

void UGridPlayerAttackPresentationComponent::
RestoreHeldItemMotion ()
{
    if (AGridItemActor* Item = AnimatedHeldItemActor.Get ())
    {
        Item->SetActorRelativeTransform (
            MotionInitialRelativeTransform);
    }
    AnimatedHeldItemActor.Reset ();
    bMotionActive = false;
    MotionElapsedSeconds = 0.0f;
    SetComponentTickEnabled (false);
}

void UGridPlayerAttackPresentationComponent::StopActiveVFX ()
{
    for (TWeakObjectPtr<UNiagaraComponent>& Component :
        ActiveNiagaraComponents)
    {
        if (Component.IsValid ())
        {
            Component->DeactivateImmediate ();
            Component->DestroyComponent ();
        }
    }
    ActiveNiagaraComponents.Reset ();
}

FText UGridPlayerAttackPresentationComponent::
ResolveCharacterDisplayName (int32 CharacterIndex) const
{
    FGridInventoryCharacterSummary Summary;
    if (PartyPawn &&
        PartyPawn->PartyInventoryComponent &&
        PartyPawn->PartyInventoryComponent->GetCharacterSummary (
            CharacterIndex,
            Summary) &&
        !Summary.DisplayName.IsEmpty ())
    {
        return Summary.DisplayName;
    }
    return NSLOCTEXT (
        "GridPlayerAttackPresentation",
        "UnknownCharacter",
        "Le personnage");
}

FText UGridPlayerAttackPresentationComponent::
ResolveMonsterDisplayName (
    const AGridMonsterActor* Monster) const
{
    return Monster &&
        Monster->MonsterDefinition &&
        !Monster->MonsterDefinition->DisplayName.IsEmpty ()
            ? Monster->MonsterDefinition->DisplayName
            : NSLOCTEXT (
                "GridPlayerAttackPresentation",
                "UnknownMonster",
                "le monstre");
}
