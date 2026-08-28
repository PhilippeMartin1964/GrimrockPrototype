#include "Runtime/Monsters/GridMonsterAudioComponent.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogGridMonsterAudio);

namespace
{
	constexpr uint32 MON10AudioSeedSalt = 0x4D313032u;
	constexpr uint32 MON10AudioPitchSalt = 0x50495443u;

	FString GetAudioEventText(EGridMonsterAudioEvent Event)
	{
		if (const UEnum* Enum = StaticEnum<EGridMonsterAudioEvent>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Event));
		}
		return TEXT("Unknown");
	}
}

int32 FGridMonsterAudioSelector::BuildPresentationSeed(const FGuid& PersistenceId, FName MonsterId, EGridMonsterAudioEvent Event, int32 OccurrenceNumber)
{
	uint32 Seed = MON10AudioSeedSalt;
	Seed = HashCombine(Seed, GetTypeHash(PersistenceId));
	Seed = HashCombine(Seed, GetTypeHash(MonsterId));
	Seed = HashCombine(Seed, GetTypeHash(static_cast<uint8>(Event)));
	Seed = HashCombine(Seed, GetTypeHash(FMath::Max(0, OccurrenceNumber)));
	return static_cast<int32>(Seed);
}

int32 FGridMonsterAudioSelector::SelectVariationIndex(int32 PresentationSeed, int32 VariationCount)
{
	if (VariationCount <= 0)
	{
		return INDEX_NONE;
	}

	FRandomStream Stream(PresentationSeed);
	return Stream.RandRange(0, VariationCount - 1);
}

float FGridMonsterAudioSelector::SelectPitch(int32 PresentationSeed, float PitchMin, float PitchMax)
{
	if (!FMath::IsFinite(PitchMin) || !FMath::IsFinite(PitchMax) || PitchMin <= 0.0f || PitchMax < PitchMin)
	{
		return 1.0f;
	}
	if (FMath::IsNearlyEqual(PitchMin, PitchMax))
	{
		return PitchMin;
	}

	FRandomStream Stream(static_cast<int32>(HashCombine(static_cast<uint32>(PresentationSeed), MON10AudioPitchSalt)));
	return Stream.FRandRange(PitchMin, PitchMax);
}

UGridMonsterAudioComponent::UGridMonsterAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	EventOccurrenceCounts.Init(0, AudioEventCount);
	EventPlaybackRequestCounts.Init(0, AudioEventCount);
	LastAcceptedEventTimes.Init(-TNumericLimits<double>::Max(), AudioEventCount);
}

void UGridMonsterAudioComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeMonsterAudio();
	RefreshIdleAmbienceScheduling();
}

void UGridMonsterAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllMonsterAudio();
	Super::EndPlay(EndPlayReason);
}

bool UGridMonsterAudioComponent::InitializeMonsterAudio()
{
	OwnerMonster = Cast<AGridMonsterActor>(GetOwner());
	bInitialized = IsValid(OwnerMonster) && IsValid(OwnerMonster->MonsterDefinition) && OwnerMonster->MonsterDefinition->IsValidDefinition();
	return bInitialized;
}

bool UGridMonsterAudioComponent::PlayAlert()
{
	StopIdleAmbience();
	const FGridMonsterAudioEventDefinition* Definition = GetMonsterDefinition(EGridMonsterAudioEvent::Alert);
	return Definition &&
		PlayDefinition(EGridMonsterAudioEvent::Alert, *Definition, NAME_None, OwnerMonster ? OwnerMonster->GetActorLocation() : FVector::ZeroVector);
}

bool UGridMonsterAudioComponent::PlayAttack(const FGridMonsterAttackDefinition& Attack)
{
	StopIdleAmbience();
	return PlayDefinition(
		EGridMonsterAudioEvent::Attack, Attack.AttackAudio, Attack.AttackId,
		OwnerMonster ? OwnerMonster->GetActorLocation() : FVector::ZeroVector);
}

bool UGridMonsterAudioComponent::PlayAttackImpact(const FGridMonsterAttackDefinition& Attack, const FGridAttackResult& Result, FVector ImpactWorldLocation)
{
	const EGridMonsterAudioEvent Event = Result.bHit ? EGridMonsterAudioEvent::ImpactHit : EGridMonsterAudioEvent::ImpactMiss;
	const FGridMonsterAudioEventDefinition& Definition = Result.bHit ? Attack.ImpactHitAudio : Attack.ImpactMissAudio;
	if (ImpactWorldLocation.ContainsNaN() && OwnerMonster)
	{
		ImpactWorldLocation = OwnerMonster->GetActorLocation();
	}
	return PlayDefinition(Event, Definition, Attack.AttackId, ImpactWorldLocation);
}

bool UGridMonsterAudioComponent::PlayHurt()
{
	StopIdleAmbience();
	const FGridMonsterAudioEventDefinition* Definition = GetMonsterDefinition(EGridMonsterAudioEvent::Hurt);
	return Definition &&
		PlayDefinition(EGridMonsterAudioEvent::Hurt, *Definition, NAME_None, OwnerMonster ? OwnerMonster->GetActorLocation() : FVector::ZeroVector);
}

bool UGridMonsterAudioComponent::PlayDeath()
{
	StopIdleAmbience();
	const FGridMonsterAudioEventDefinition* Definition = GetMonsterDefinition(EGridMonsterAudioEvent::Death);
	return Definition &&
		PlayDefinition(EGridMonsterAudioEvent::Death, *Definition, NAME_None, OwnerMonster ? OwnerMonster->GetActorLocation() : FVector::ZeroVector);
}

bool UGridMonsterAudioComponent::PlayIdleAmbienceNow()
{
	if (!CanScheduleIdleAmbience())
	{
		return false;
	}
	return PlayDefinition(EGridMonsterAudioEvent::Idle, OwnerMonster->MonsterDefinition->IdleAudio, NAME_None, OwnerMonster->GetActorLocation());
}

void UGridMonsterAudioComponent::RefreshIdleAmbienceScheduling()
{
	StopIdleAmbience();
	if (!CanScheduleIdleAmbience())
	{
		return;
	}

	UWorld* World = GetWorld();
	const UGridMonsterDefinitionAsset* Definition = OwnerMonster->MonsterDefinition;
	const int32 Seed = FGridMonsterAudioSelector::BuildPresentationSeed(
		OwnerMonster->ResolvePersistenceId(), Definition->MonsterId, EGridMonsterAudioEvent::Idle, ++IdleScheduleOccurrence);
	FRandomStream DelayStream(Seed);
	const float Delay = FMath::IsNearlyEqual(Definition->IdleAudioMinDelay, Definition->IdleAudioMaxDelay)
		? Definition->IdleAudioMinDelay
		: DelayStream.FRandRange(Definition->IdleAudioMinDelay, Definition->IdleAudioMaxDelay);
	World->GetTimerManager().SetTimer(IdleAmbienceTimerHandle, this, &UGridMonsterAudioComponent::HandleIdleAmbienceTimer, Delay, false);
}

void UGridMonsterAudioComponent::StopIdleAmbience()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IdleAmbienceTimerHandle);
	}
	IdleAmbienceTimerHandle.Invalidate();
}

void UGridMonsterAudioComponent::StopAllMonsterAudio()
{
	// One-shots are intentionally fire-and-forget. Only scheduled ambience is
	// owned by this component and can be stopped without affecting gameplay.
	StopIdleAmbience();
}

void UGridMonsterAudioComponent::ResetTransientAudioState()
{
	StopAllMonsterAudio();
	NextSequenceNumber = 1;
	IdleScheduleOccurrence = 0;
	PlaybackRequestCount = 0;
	LastPlaybackRequest = FGridMonsterAudioPlaybackRequest();
	EventOccurrenceCounts.Init(0, AudioEventCount);
	EventPlaybackRequestCounts.Init(0, AudioEventCount);
	LastAcceptedEventTimes.Init(-TNumericLimits<double>::Max(), AudioEventCount);
}

int32 UGridMonsterAudioComponent::GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent Event) const
{
	const int32 EventIndex = GetEventIndex(Event);
	return EventPlaybackRequestCounts.IsValidIndex(EventIndex) ? EventPlaybackRequestCounts[EventIndex] : 0;
}

bool UGridMonsterAudioComponent::IsIdleAmbienceScheduled() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(IdleAmbienceTimerHandle);
}

void UGridMonsterAudioComponent::LogMonsterAudioState() const
{
	UE_LOG(LogGridMonsterAudio, Log, TEXT("[GridMonsterAudio] State Monster=%s Initialized=%s Enabled=%s Native=%s Requests=%d IdleScheduled=%s"),
		*GetNameSafe(OwnerMonster), bInitialized ? TEXT("true") : TEXT("false"), bAudioEnabled ? TEXT("true") : TEXT("false"),
		bNativePlaybackEnabled ? TEXT("true") : TEXT("false"), PlaybackRequestCount, IsIdleAmbienceScheduled() ? TEXT("true") : TEXT("false"));
}

bool UGridMonsterAudioComponent::PlayDefinition(
	EGridMonsterAudioEvent Event, const FGridMonsterAudioEventDefinition& Definition, FName AttackId, const FVector& WorldLocation)
{
	if (!CanRequestAudio(Event) || !Definition.IsValidDefinition() || !Definition.HasConfiguredSound())
	{
		return false;
	}

	const int32 EventIndex = GetEventIndex(Event);
	if (!EventOccurrenceCounts.IsValidIndex(EventIndex) || !LastAcceptedEventTimes.IsValidIndex(EventIndex))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const double CurrentTime = World ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
	if (Definition.CooldownSeconds > 0.0f && CurrentTime - LastAcceptedEventTimes[EventIndex] < static_cast<double>(Definition.CooldownSeconds))
	{
		UE_LOG(LogGridMonsterAudio, Verbose, TEXT("[GridMonsterAudio] Rejected Monster=%s Event=%s Reason=Cooldown"), *GetNameSafe(OwnerMonster),
			*GetAudioEventText(Event));
		return false;
	}

	const int32 OccurrenceNumber = EventOccurrenceCounts[EventIndex] + 1;
	const int32 Seed = FGridMonsterAudioSelector::BuildPresentationSeed(
		OwnerMonster->ResolvePersistenceId(), OwnerMonster->MonsterDefinition->MonsterId, Event, OccurrenceNumber);
	const int32 VariationIndex = FGridMonsterAudioSelector::SelectVariationIndex(Seed, Definition.Sounds.Num());
	if (!Definition.Sounds.IsValidIndex(VariationIndex))
	{
		return false;
	}

	USoundBase* Sound = Definition.Sounds[VariationIndex].LoadSynchronous();
	if (!IsValid(Sound))
	{
		return false;
	}

	EventOccurrenceCounts[EventIndex] = OccurrenceNumber;
	++EventPlaybackRequestCounts[EventIndex];
	LastAcceptedEventTimes[EventIndex] = CurrentTime;

	FGridMonsterAudioPlaybackRequest Request;
	Request.SequenceNumber = NextSequenceNumber++;
	Request.Event = Event;
	Request.MonsterId = OwnerMonster->MonsterDefinition->MonsterId;
	Request.AttackId = AttackId;
	Request.Sound = Sound;
	Request.WorldLocation = WorldLocation;
	Request.VolumeMultiplier = Definition.VolumeMultiplier;
	Request.PitchMultiplier = FGridMonsterAudioSelector::SelectPitch(Seed, Definition.PitchMin, Definition.PitchMax);

	LastPlaybackRequest = Request;
	++PlaybackRequestCount;
	OnAudioPlaybackRequested.Broadcast(Request);

	UE_LOG(LogGridMonsterAudio, Log, TEXT("[GridMonsterAudio] Seq=%d Monster=%s Event=%s Attack=%s Sound=%s Volume=%.2f Pitch=%.2f Location=%s"),
		Request.SequenceNumber, *GetNameSafe(OwnerMonster), *GetAudioEventText(Event), *AttackId.ToString(), *Sound->GetPathName(), Request.VolumeMultiplier,
		Request.PitchMultiplier, *Request.WorldLocation.ToCompactString());

	if (bNativePlaybackEnabled && GetWorld())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Request.WorldLocation, Request.VolumeMultiplier, Request.PitchMultiplier);
	}
	return true;
}

const FGridMonsterAudioEventDefinition* UGridMonsterAudioComponent::GetMonsterDefinition(EGridMonsterAudioEvent Event) const
{
	if (!OwnerMonster || !OwnerMonster->MonsterDefinition)
	{
		return nullptr;
	}

	switch (Event)
	{
		case EGridMonsterAudioEvent::Alert:
			return &OwnerMonster->MonsterDefinition->AlertAudio;
		case EGridMonsterAudioEvent::Hurt:
			return &OwnerMonster->MonsterDefinition->HurtAudio;
		case EGridMonsterAudioEvent::Death:
			return &OwnerMonster->MonsterDefinition->DeathAudio;
		case EGridMonsterAudioEvent::Idle:
			return &OwnerMonster->MonsterDefinition->IdleAudio;
		default:
			return nullptr;
	}
}

bool UGridMonsterAudioComponent::CanRequestAudio(EGridMonsterAudioEvent Event) const
{
	return bAudioEnabled && bInitialized && IsValid(OwnerMonster) && IsValid(OwnerMonster->MonsterDefinition) && OwnerMonster->bMonsterEnabled &&
		OwnerMonster->IsRuntimeLevelActive() && (Event == EGridMonsterAudioEvent::Death || !OwnerMonster->IsDead());
}

bool UGridMonsterAudioComponent::CanScheduleIdleAmbience() const
{
	return CanRequestAudio(EGridMonsterAudioEvent::Idle) && OwnerMonster->MonsterDefinition->bEnableIdleAudio &&
		OwnerMonster->MonsterDefinition->IdleAudio.HasConfiguredSound() && OwnerMonster->MonsterDefinition->IdleAudio.IsValidDefinition() &&
		(OwnerMonster->MonsterState == EGridMonsterState::Idle || OwnerMonster->MonsterState == EGridMonsterState::Dormant) && GetWorld();
}

int32 UGridMonsterAudioComponent::GetEventIndex(EGridMonsterAudioEvent Event) const
{
	const int32 EventIndex = static_cast<int32>(Event);
	return EventIndex >= 0 && EventIndex < AudioEventCount ? EventIndex : INDEX_NONE;
}

void UGridMonsterAudioComponent::HandleIdleAmbienceTimer()
{
	if (!CanScheduleIdleAmbience())
	{
		StopIdleAmbience();
		return;
	}

	PlayIdleAmbienceNow();
	RefreshIdleAmbienceScheduling();
}
