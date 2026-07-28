#include "Runtime/Monsters/GridMonsterVFXComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

DEFINE_LOG_CATEGORY (LogGridMonsterVFX);

namespace
{
    constexpr uint32 MON10VFXSeedSalt = 0x56313033u;

    FString GetVFXEventText (EGridMonsterVFXEvent Event)
    {
        if (const UEnum* Enum = StaticEnum<EGridMonsterVFXEvent> ())
        {
            return Enum->GetNameStringByValue (
                static_cast<int64> (Event));
        }
        return TEXT ("Unknown");
    }
}

int32 FGridMonsterVFXSelector::BuildPresentationSeed (
    const FGuid& PersistenceId,
    FName MonsterId,
    EGridMonsterVFXEvent Event,
    int32 OccurrenceNumber)
{
    uint32 Seed = MON10VFXSeedSalt;
    Seed = HashCombine (Seed, GetTypeHash (PersistenceId));
    Seed = HashCombine (Seed, GetTypeHash (MonsterId));
    Seed = HashCombine (
        Seed,
        GetTypeHash (static_cast<uint8> (Event)));
    Seed = HashCombine (
        Seed,
        GetTypeHash (FMath::Max (0, OccurrenceNumber)));
    return static_cast<int32> (Seed);
}

int32 FGridMonsterVFXSelector::SelectVariationIndex (
    int32 PresentationSeed,
    int32 VariationCount)
{
    if (VariationCount <= 0)
    {
        return INDEX_NONE;
    }

    FRandomStream Stream (PresentationSeed);
    return Stream.RandRange (0, VariationCount - 1);
}

UGridMonsterVFXComponent::UGridMonsterVFXComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    EventOccurrenceCounts.Init (0, VFXEventCount);
    EventSpawnRequestCounts.Init (0, VFXEventCount);
    LastAcceptedEventTimes.Init (
        -TNumericLimits<double>::Max (),
        VFXEventCount);
}

void UGridMonsterVFXComponent::BeginPlay ()
{
    Super::BeginPlay ();
    InitializeMonsterVFX ();
}

void UGridMonsterVFXComponent::EndPlay (
    const EEndPlayReason::Type EndPlayReason)
{
    StopAllMonsterVFX ();
    Super::EndPlay (EndPlayReason);
}

bool UGridMonsterVFXComponent::InitializeMonsterVFX ()
{
    OwnerMonster = Cast<AGridMonsterActor> (GetOwner ());
    bInitialized =
        IsValid (OwnerMonster) &&
        IsValid (OwnerMonster->MonsterDefinition) &&
        OwnerMonster->MonsterDefinition->IsValidDefinition ();
    return bInitialized;
}

bool UGridMonsterVFXComponent::PlayAlertVFX ()
{
    const FGridMonsterVFXEventDefinition* Definition =
        GetMonsterDefinition (EGridMonsterVFXEvent::Alert);
    return Definition && PlayDefinition (
        EGridMonsterVFXEvent::Alert,
        *Definition,
        NAME_None,
        nullptr,
        FVector::ZeroVector,
        INDEX_NONE);
}

bool UGridMonsterVFXComponent::PlayAttackVFX (
    const FGridMonsterAttackDefinition& Attack)
{
    return PlayDefinition (
        EGridMonsterVFXEvent::Attack,
        Attack.AttackVFXDefinition,
        Attack.AttackId,
        nullptr,
        FVector::ZeroVector,
        INDEX_NONE);
}

bool UGridMonsterVFXComponent::PlayAttackImpactVFX (
    const FGridMonsterAttackDefinition& Attack,
    const FGridAttackResult& Result,
    FVector ImpactWorldLocation,
    int32 TargetCharacterIndex)
{
    const EGridMonsterVFXEvent Event = Result.bHit
        ? EGridMonsterVFXEvent::ImpactHit
        : EGridMonsterVFXEvent::ImpactMiss;
    const FGridMonsterVFXEventDefinition* Definition =
        Result.bHit
            ? &Attack.ImpactHitVFXDefinition
            : &Attack.ImpactMissVFXDefinition;
    FGridMonsterVFXEventDefinition LegacyDefinition;
    if (Result.bHit &&
        !Definition->HasConfiguredSystem () &&
        !Attack.ImpactVFX.IsNull ())
    {
        LegacyDefinition.Systems.Add (Attack.ImpactVFX);
        Definition = &LegacyDefinition;
    }

    if (ImpactWorldLocation.ContainsNaN () && OwnerMonster)
    {
        ImpactWorldLocation = OwnerMonster->GetActorLocation ();
    }
    return PlayDefinition (
        Event,
        *Definition,
        Attack.AttackId,
        &Result,
        ImpactWorldLocation,
        TargetCharacterIndex);
}

bool UGridMonsterVFXComponent::PlayHurtVFX (
    const FGridAttackResult& Result)
{
    const FGridMonsterVFXEventDefinition* Definition =
        GetMonsterDefinition (EGridMonsterVFXEvent::Hurt);
    return Definition && PlayDefinition (
        EGridMonsterVFXEvent::Hurt,
        *Definition,
        NAME_None,
        &Result,
        FVector::ZeroVector,
        INDEX_NONE);
}

bool UGridMonsterVFXComponent::PlayDeathVFX ()
{
    const FGridMonsterVFXEventDefinition* Definition =
        GetMonsterDefinition (EGridMonsterVFXEvent::Death);
    return Definition && PlayDefinition (
        EGridMonsterVFXEvent::Death,
        *Definition,
        NAME_None,
        nullptr,
        FVector::ZeroVector,
        INDEX_NONE);
}

void UGridMonsterVFXComponent::StopAllMonsterVFX ()
{
    CompactActiveNiagaraComponents ();
    for (const TWeakObjectPtr<UNiagaraComponent>& WeakComponent :
        ActiveNiagaraComponents)
    {
        if (UNiagaraComponent* Component = WeakComponent.Get ())
        {
            Component->DeactivateImmediate ();
            Component->DestroyComponent ();
        }
    }
    ActiveNiagaraComponents.Reset ();
}

void UGridMonsterVFXComponent::ResetTransientVFXState ()
{
    StopAllMonsterVFX ();
    NextSequenceNumber = 1;
    SpawnRequestCount = 0;
    VFXSpawnBroadcastCount = 0;
    LastSpawnRequest = FGridMonsterVFXSpawnRequest ();
    EventOccurrenceCounts.Init (0, VFXEventCount);
    EventSpawnRequestCounts.Init (0, VFXEventCount);
    LastAcceptedEventTimes.Init (
        -TNumericLimits<double>::Max (),
        VFXEventCount);
}

int32 UGridMonsterVFXComponent::GetSpawnRequestCountForEvent (
    EGridMonsterVFXEvent Event) const
{
    const int32 EventIndex = GetEventIndex (Event);
    return EventSpawnRequestCounts.IsValidIndex (EventIndex)
        ? EventSpawnRequestCounts[EventIndex]
        : 0;
}

int32 UGridMonsterVFXComponent::GetActiveNiagaraComponentCount () const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<UNiagaraComponent>& Component :
        ActiveNiagaraComponents)
    {
        Count += Component.IsValid () ? 1 : 0;
    }
    return Count;
}

void UGridMonsterVFXComponent::LogMonsterVFXState () const
{
    UE_LOG (
        LogGridMonsterVFX,
        Log,
        TEXT ("[GridMonsterVFX] State Monster=%s Initialized=%s Enabled=%s Native=%s Requests=%d Active=%d"),
        *GetNameSafe (OwnerMonster),
        bInitialized ? TEXT ("true") : TEXT ("false"),
        bVFXEnabled ? TEXT ("true") : TEXT ("false"),
        bNativeSpawnEnabled ? TEXT ("true") : TEXT ("false"),
        SpawnRequestCount,
        GetActiveNiagaraComponentCount ());
}

bool UGridMonsterVFXComponent::PlayDefinition (
    EGridMonsterVFXEvent Event,
    const FGridMonsterVFXEventDefinition& Definition,
    FName AttackId,
    const FGridAttackResult* Result,
    FVector ImpactWorldLocation,
    int32 TargetCharacterIndex)
{
    CompactActiveNiagaraComponents ();
    if (!CanRequestVFX (Event) ||
        !Definition.IsValidDefinition () ||
        !Definition.HasConfiguredSystem ())
    {
        return false;
    }

    const int32 EventIndex = GetEventIndex (Event);
    if (!EventOccurrenceCounts.IsValidIndex (EventIndex) ||
        !LastAcceptedEventTimes.IsValidIndex (EventIndex))
    {
        return false;
    }

    const UWorld* World = GetWorld ();
    const double CurrentTime = World
        ? static_cast<double> (World->GetTimeSeconds ())
        : 0.0;
    if (Definition.CooldownSeconds > 0.0f &&
        CurrentTime - LastAcceptedEventTimes[EventIndex] <
            static_cast<double> (Definition.CooldownSeconds))
    {
        return false;
    }

    const int32 OccurrenceNumber =
        EventOccurrenceCounts[EventIndex] + 1;
    const int32 Seed =
        FGridMonsterVFXSelector::BuildPresentationSeed (
            OwnerMonster->ResolvePersistenceId (),
            OwnerMonster->MonsterDefinition->MonsterId,
            Event,
            OccurrenceNumber);
    const int32 VariationIndex =
        FGridMonsterVFXSelector::SelectVariationIndex (
            Seed,
            Definition.Systems.Num ());
    if (!Definition.Systems.IsValidIndex (VariationIndex))
    {
        return false;
    }

    UNiagaraSystem* System =
        Definition.Systems[VariationIndex].LoadSynchronous ();
    if (!IsValid (System))
    {
        UE_LOG (
            LogGridMonsterVFX,
            Warning,
            TEXT ("[GridMonsterVFX] Monster=%s Event=%s LoadFailed=%s"),
            *GetNameSafe (OwnerMonster),
            *GetVFXEventText (Event),
            *Definition.Systems[VariationIndex].ToSoftObjectPath ().ToString ());
        return false;
    }

    const bool bIsImpact =
        Event == EGridMonsterVFXEvent::ImpactHit ||
        Event == EGridMonsterVFXEvent::ImpactMiss;
    USceneComponent* AttachComponent = nullptr;
    if (!bIsImpact && Definition.bAttachToSource)
    {
        AttachComponent = OwnerMonster->SkeletalMeshComponent
            ? Cast<USceneComponent> (OwnerMonster->SkeletalMeshComponent)
            : OwnerMonster->GetRootComponent ();
    }

    FTransform WorldTransform;
    if (bIsImpact)
    {
        FRotator Rotation = Definition.RotationOffset;
        const FVector Direction =
            ImpactWorldLocation - OwnerMonster->GetActorLocation ();
        if (!Direction.IsNearlyZero ())
        {
            Rotation = (Direction.Rotation () +
                Definition.RotationOffset).GetNormalized ();
        }
        WorldTransform = FTransform (
            Rotation,
            ImpactWorldLocation + Definition.LocationOffset,
            Definition.Scale);
    }
    else if (AttachComponent)
    {
        const FTransform SourceTransform =
            AttachComponent->GetSocketTransform (
                Definition.SocketName,
                RTS_World);
        WorldTransform = FTransform (
            Definition.RotationOffset,
            Definition.LocationOffset,
            Definition.Scale) * SourceTransform;
    }
    else
    {
        WorldTransform = FTransform (
            (OwnerMonster->GetActorRotation () +
                Definition.RotationOffset).GetNormalized (),
            OwnerMonster->GetActorLocation () +
                Definition.LocationOffset,
            Definition.Scale);
    }

    EventOccurrenceCounts[EventIndex] = OccurrenceNumber;
    ++EventSpawnRequestCounts[EventIndex];
    LastAcceptedEventTimes[EventIndex] = CurrentTime;

    FGridMonsterVFXSpawnRequest Request;
    Request.SequenceNumber = NextSequenceNumber++;
    Request.Event = Event;
    Request.MonsterId =
        OwnerMonster->MonsterDefinition->MonsterId;
    Request.AttackId = AttackId;
    Request.System = System;
    Request.WorldTransform = WorldTransform;
    Request.bAttachToSource = AttachComponent != nullptr;
    Request.AttachComponent = AttachComponent;
    Request.SocketName = AttachComponent
        ? Definition.SocketName
        : NAME_None;
    Request.TargetCharacterIndex = TargetCharacterIndex;
    if (Result)
    {
        Request.DamageType = Result->DamageType;
        Request.PhysicalSubtype = Result->PhysicalSubtype;
        Request.bHasAttackResult = true;
        Request.AttackResult = *Result;
    }

    LastSpawnRequest = Request;
    ++SpawnRequestCount;
    UE_LOG (
        LogGridMonsterVFX,
        Log,
        TEXT ("[GridMonsterVFX] Seq=%d Monster=%s Event=%s Attack=%s System=%s Target=%d Critical=%s Damage=%d Location=%s"),
        Request.SequenceNumber,
        *GetNameSafe (OwnerMonster),
        *GetVFXEventText (Event),
        *AttackId.ToString (),
        *System->GetPathName (),
        TargetCharacterIndex,
        Result && Result->bCriticalHit ? TEXT ("true") : TEXT ("false"),
        Result ? Result->GetTotalAppliedDamage () : 0,
        *WorldTransform.GetLocation ().ToCompactString ());
    ++VFXSpawnBroadcastCount;
    OnVFXSpawnRequested.Broadcast (Request);

    if (!bNativeSpawnEnabled || !GetWorld ())
    {
        return true;
    }

    UNiagaraComponent* SpawnedComponent = nullptr;
    if (AttachComponent)
    {
        SpawnedComponent =
            UNiagaraFunctionLibrary::SpawnSystemAttached (
                System,
                AttachComponent,
                Definition.SocketName,
                Definition.LocationOffset,
                Definition.RotationOffset,
                Definition.Scale,
                EAttachLocation::KeepRelativeOffset,
                true,
                ENCPoolMethod::None,
                true,
                true);
    }
    else
    {
        SpawnedComponent =
            UNiagaraFunctionLibrary::SpawnSystemAtLocation (
                this,
                System,
                WorldTransform.GetLocation (),
                WorldTransform.Rotator (),
                WorldTransform.GetScale3D (),
                true,
                true,
                ENCPoolMethod::None,
                true);
    }
    if (SpawnedComponent)
    {
        ActiveNiagaraComponents.Add (SpawnedComponent);
    }
    return true;
}

const FGridMonsterVFXEventDefinition*
UGridMonsterVFXComponent::GetMonsterDefinition (
    EGridMonsterVFXEvent Event) const
{
    if (!OwnerMonster || !OwnerMonster->MonsterDefinition)
    {
        return nullptr;
    }

    switch (Event)
    {
    case EGridMonsterVFXEvent::Alert:
        return &OwnerMonster->MonsterDefinition->AlertVFX;
    case EGridMonsterVFXEvent::Hurt:
        return &OwnerMonster->MonsterDefinition->HurtVFX;
    case EGridMonsterVFXEvent::Death:
        return &OwnerMonster->MonsterDefinition->DeathVFX;
    default:
        return nullptr;
    }
}

bool UGridMonsterVFXComponent::CanRequestVFX (
    EGridMonsterVFXEvent Event) const
{
    return bVFXEnabled &&
        bInitialized &&
        IsValid (OwnerMonster) &&
        IsValid (OwnerMonster->MonsterDefinition) &&
        OwnerMonster->bMonsterEnabled &&
        OwnerMonster->IsRuntimeLevelActive () &&
        (Event == EGridMonsterVFXEvent::Death ||
            !OwnerMonster->IsDead ());
}

int32 UGridMonsterVFXComponent::GetEventIndex (
    EGridMonsterVFXEvent Event) const
{
    const int32 EventIndex = static_cast<int32> (Event);
    return EventIndex >= 0 && EventIndex < VFXEventCount
        ? EventIndex
        : INDEX_NONE;
}

void UGridMonsterVFXComponent::CompactActiveNiagaraComponents ()
{
    ActiveNiagaraComponents.RemoveAll (
        [] (const TWeakObjectPtr<UNiagaraComponent>& Component)
        {
            return !Component.IsValid ();
        });
}
