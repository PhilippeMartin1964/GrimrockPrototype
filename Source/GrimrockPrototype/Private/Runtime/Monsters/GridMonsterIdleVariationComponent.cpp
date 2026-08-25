#include "Runtime/Monsters/GridMonsterIdleVariationComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogGridMonsterIdleVariation);

namespace
{
	constexpr uint32 MON10IdleSeedSalt = 0x49313034u;
	constexpr uint32 MON10IdleVariationSalt = 0x56415249u;
	constexpr uint32 MON10IdleDelaySalt = 0x44454C59u;
	constexpr float SafeIdleDelayFallback = 1.0f;
}

bool FGridMonsterIdleVariationDefinition::IsValidDefinition() const
{
	return !VariationId.IsNone() && !Animation.IsNull() && FMath::IsFinite(PlayRate) && PlayRate > 0.0f && FMath::IsFinite(ExpectedDuration) &&
		ExpectedDuration > 0.0f;
}

int32 FGridMonsterIdleVariationSelector::BuildPresentationSeed(const FGuid& PersistenceId, FName MonsterId, int32 OccurrenceNumber)
{
	uint32 Seed = MON10IdleSeedSalt;
	Seed = HashCombine(Seed, GetTypeHash(PersistenceId));
	Seed = HashCombine(Seed, GetTypeHash(MonsterId));
	Seed = HashCombine(Seed, GetTypeHash(FMath::Max(0, OccurrenceNumber)));
	return static_cast<int32>(Seed);
}

int32 FGridMonsterIdleVariationSelector::SelectVariationIndex(
	int32 PresentationSeed, int32 VariationCount, int32 PreviousVariationIndex, bool bAvoidImmediateRepeat)
{
	if (VariationCount <= 0)
	{
		return INDEX_NONE;
	}
	if (VariationCount == 1)
	{
		return 0;
	}

	FRandomStream Stream(static_cast<int32>(HashCombine(static_cast<uint32>(PresentationSeed), MON10IdleVariationSalt)));
	if (bAvoidImmediateRepeat && PreviousVariationIndex >= 0 && PreviousVariationIndex < VariationCount)
	{
		const int32 CompactIndex = Stream.RandRange(0, VariationCount - 2);
		return CompactIndex >= PreviousVariationIndex ? CompactIndex + 1 : CompactIndex;
	}
	return Stream.RandRange(0, VariationCount - 1);
}

float FGridMonsterIdleVariationSelector::SelectDelay(int32 PresentationSeed, float MinimumDelay, float MaximumDelay)
{
	if (!FMath::IsFinite(MinimumDelay) || !FMath::IsFinite(MaximumDelay) || MinimumDelay <= 0.0f || MaximumDelay < MinimumDelay)
	{
		return SafeIdleDelayFallback;
	}
	if (FMath::IsNearlyEqual(MinimumDelay, MaximumDelay))
	{
		return MinimumDelay;
	}

	FRandomStream Stream(static_cast<int32>(HashCombine(static_cast<uint32>(PresentationSeed), MON10IdleDelaySalt)));
	return Stream.FRandRange(MinimumDelay, MaximumDelay);
}

UGridMonsterIdleVariationComponent::UGridMonsterIdleVariationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGridMonsterIdleVariationComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeIdleVariations();
	RefreshIdleVariationScheduling();
}

void UGridMonsterIdleVariationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopIdleVariations();
	Super::EndPlay(EndPlayReason);
}

bool UGridMonsterIdleVariationComponent::InitializeIdleVariations()
{
	OwnerMonster = Cast<AGridMonsterActor>(GetOwner());
	bInitialized = IsValid(OwnerMonster) && IsValid(OwnerMonster->MonsterDefinition) && OwnerMonster->MonsterDefinition->IsValidDefinition();
	return bInitialized;
}

void UGridMonsterIdleVariationComponent::RefreshIdleVariationScheduling()
{
	if (!CanUseIdleVariations())
	{
		StopIdleVariations();
		return;
	}
	if (bIdleVariationActive || IsIdleVariationDelayScheduled())
	{
		return;
	}

	const UGridMonsterDefinitionAsset* Definition = OwnerMonster->MonsterDefinition;
	ScheduledOccurrenceNumber = CompletedOccurrenceNumber + 1;
	const int32 Seed =
		FGridMonsterIdleVariationSelector::BuildPresentationSeed(OwnerMonster->ResolvePersistenceId(), Definition->MonsterId, ScheduledOccurrenceNumber);
	LastScheduledDelay = FGridMonsterIdleVariationSelector::SelectDelay(Seed, Definition->IdleVariationMinDelay, Definition->IdleVariationMaxDelay);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			IdleVariationDelayTimerHandle, this, &UGridMonsterIdleVariationComponent::HandleIdleVariationDelayTimer, LastScheduledDelay, false);
	}
}

bool UGridMonsterIdleVariationComponent::PlayIdleVariationNow()
{
	if (bIdleVariationActive || !CanUseIdleVariations())
	{
		StopIdleVariations();
		return false;
	}

	ClearDelayTimer();
	const UGridMonsterDefinitionAsset* Definition = OwnerMonster->MonsterDefinition;
	const int32 OccurrenceNumber = ScheduledOccurrenceNumber > CompletedOccurrenceNumber ? ScheduledOccurrenceNumber : CompletedOccurrenceNumber + 1;
	const int32 Seed = FGridMonsterIdleVariationSelector::BuildPresentationSeed(OwnerMonster->ResolvePersistenceId(), Definition->MonsterId, OccurrenceNumber);
	const int32 VariationIndex = FGridMonsterIdleVariationSelector::SelectVariationIndex(
		Seed, Definition->IdleVariations.Num(), PreviousVariationIndex, Definition->bAvoidImmediateIdleVariationRepeat);
	if (!Definition->IdleVariations.IsValidIndex(VariationIndex))
	{
		ScheduledOccurrenceNumber = 0;
		return false;
	}

	const FGridMonsterIdleVariationDefinition& Variation = Definition->IdleVariations[VariationIndex];
	UAnimSequenceBase* Animation = Variation.Animation.LoadSynchronous();
	if (!IsValid(Animation))
	{
		UE_LOG(LogGridMonsterIdleVariation, Warning, TEXT("[GridMonsterIdle] Monster=%s Variation=%s LoadFailed=%s"), *GetNameSafe(OwnerMonster),
			*Variation.VariationId.ToString(), *Variation.Animation.ToSoftObjectPath().ToString());
		ScheduledOccurrenceNumber = 0;
		RefreshIdleVariationScheduling();
		return false;
	}

	float EffectiveDuration = Animation->GetPlayLength() / Variation.PlayRate;
	if (!FMath::IsFinite(EffectiveDuration) || EffectiveDuration <= 0.0f)
	{
		EffectiveDuration = Variation.ExpectedDuration;
	}
	EffectiveDuration = FMath::Max(0.01f, EffectiveDuration);

	FGridMonsterIdleVariationPlaybackRequest Request;
	Request.SequenceNumber = NextSequenceNumber++;
	Request.OccurrenceNumber = OccurrenceNumber;
	Request.MonsterId = Definition->MonsterId;
	Request.VariationId = Variation.VariationId;
	Request.VariationIndex = VariationIndex;
	Request.Animation = Animation;
	Request.SlotName = Definition->IdleVariationSlotName;
	Request.PlayRate = Variation.PlayRate;
	Request.EffectiveDuration = EffectiveDuration;
	Request.BlendInTime = Definition->IdleVariationBlendInTime;
	Request.BlendOutTime = Definition->IdleVariationBlendOutTime;

	CompletedOccurrenceNumber = OccurrenceNumber;
	ScheduledOccurrenceNumber = 0;
	PreviousVariationIndex = VariationIndex;
	CurrentVariationIndex = VariationIndex;
	bIdleVariationActive = true;
	LastPlaybackRequest = Request;
	++PlaybackRequestCount;

	UE_LOG(LogGridMonsterIdleVariation, Log,
		TEXT("[GridMonsterIdle] Seq=%d Monster=%s Occurrence=%d Variation=%s Index=%d Animation=%s Delay=%.2f Duration=%.2f PlayRate=%.2f Slot=%s Native=%s"),
		Request.SequenceNumber, *GetNameSafe(OwnerMonster), Request.OccurrenceNumber, *Request.VariationId.ToString(), Request.VariationIndex,
		*Animation->GetPathName(), LastScheduledDelay, Request.EffectiveDuration, Request.PlayRate, *Request.SlotName.ToString(),
		bNativePlaybackEnabled ? TEXT("true") : TEXT("false"));

	++PlaybackRequestBroadcastCount;
	OnIdleVariationPlaybackRequested.Broadcast(Request);

	if (!CanUseIdleVariations())
	{
		StopIdleVariations();
		return true;
	}

	ActiveIdleDynamicMontage = nullptr;
	if (bNativePlaybackEnabled)
	{
		UAnimInstance* AnimInstance = OwnerMonster->SkeletalMeshComponent ? OwnerMonster->SkeletalMeshComponent->GetAnimInstance() : nullptr;
		if (AnimInstance)
		{
			ActiveIdleDynamicMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
				Animation, Request.SlotName, Request.BlendInTime, Request.BlendOutTime, Request.PlayRate, 1, -1.0f, 0.0f);
		}
		if (!ActiveIdleDynamicMontage)
		{
			UE_LOG(LogGridMonsterIdleVariation, Warning, TEXT("[GridMonsterIdle] Monster=%s Variation=%s NativePlaybackFailed"), *GetNameSafe(OwnerMonster),
				*Request.VariationId.ToString());
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			IdleVariationDurationTimerHandle, this, &UGridMonsterIdleVariationComponent::HandleIdleVariationDurationTimer, Request.EffectiveDuration, false);
	}
	return true;
}

void UGridMonsterIdleVariationComponent::StopIdleVariations()
{
	ClearDelayTimer();
	ClearDurationTimer();

	if (ActiveIdleDynamicMontage && IsValid(OwnerMonster) && OwnerMonster->SkeletalMeshComponent)
	{
		if (UAnimInstance* AnimInstance = OwnerMonster->SkeletalMeshComponent->GetAnimInstance())
		{
			const float BlendOutTime = OwnerMonster->MonsterDefinition ? OwnerMonster->MonsterDefinition->IdleVariationBlendOutTime : 0.0f;
			AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), ActiveIdleDynamicMontage);
		}
	}

	ActiveIdleDynamicMontage = nullptr;
	ScheduledOccurrenceNumber = 0;
	LastScheduledDelay = 0.0f;
	CurrentVariationIndex = INDEX_NONE;
	bIdleVariationActive = false;
}

void UGridMonsterIdleVariationComponent::ResetTransientIdleVariationState()
{
	StopIdleVariations();
	NextSequenceNumber = 1;
	CompletedOccurrenceNumber = 0;
	PreviousVariationIndex = INDEX_NONE;
	LastPlaybackRequest = FGridMonsterIdleVariationPlaybackRequest();
	PlaybackRequestCount = 0;
	PlaybackRequestBroadcastCount = 0;
}

bool UGridMonsterIdleVariationComponent::IsIdleVariationDelayScheduled() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(IdleVariationDelayTimerHandle);
}

void UGridMonsterIdleVariationComponent::LogIdleVariationState() const
{
	UE_LOG(LogGridMonsterIdleVariation, Log,
		TEXT("[GridMonsterIdle] State Monster=%s Initialized=%s Enabled=%s Native=%s Requests=%d Scheduled=%s Active=%s Current=%d Previous=%d Occurrence=%d"),
		*GetNameSafe(OwnerMonster), bInitialized ? TEXT("true") : TEXT("false"), bIdleVariationsEnabled ? TEXT("true") : TEXT("false"),
		bNativePlaybackEnabled ? TEXT("true") : TEXT("false"), PlaybackRequestCount, IsIdleVariationDelayScheduled() ? TEXT("true") : TEXT("false"),
		bIdleVariationActive ? TEXT("true") : TEXT("false"), CurrentVariationIndex, PreviousVariationIndex, CompletedOccurrenceNumber);
}

bool UGridMonsterIdleVariationComponent::CanUseIdleVariations() const
{
	if (!bIdleVariationsEnabled || !bInitialized || !IsValid(OwnerMonster) || !IsValid(OwnerMonster->MonsterDefinition) ||
		!OwnerMonster->MonsterDefinition->IsValidDefinition() || !OwnerMonster->MonsterDefinition->bEnableIdleVariations ||
		OwnerMonster->MonsterDefinition->IdleVariations.IsEmpty() || OwnerMonster->IsDead() || !OwnerMonster->bMonsterEnabled ||
		!OwnerMonster->IsRuntimeLevelActive() || OwnerMonster->IsHidden() ||
		(OwnerMonster->MonsterState != EGridMonsterState::Idle && OwnerMonster->MonsterState != EGridMonsterState::Dormant) || OwnerMonster->bIsMoving ||
		OwnerMonster->bIsTurning)
	{
		return false;
	}
	if (!GetWorld())
	{
		return false;
	}

	if (OwnerMonster->CombatComponent && OwnerMonster->CombatComponent->bAttackPresentationActive)
	{
		return false;
	}
	if (OwnerMonster->DeathComponent && (OwnerMonster->DeathComponent->bDeathCommitted || OwnerMonster->DeathComponent->bDeathPresentationActive))
	{
		return false;
	}
	return true;
}

void UGridMonsterIdleVariationComponent::ClearDelayTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IdleVariationDelayTimerHandle);
	}
	IdleVariationDelayTimerHandle.Invalidate();
}

void UGridMonsterIdleVariationComponent::ClearDurationTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IdleVariationDurationTimerHandle);
	}
	IdleVariationDurationTimerHandle.Invalidate();
}

void UGridMonsterIdleVariationComponent::HandleIdleVariationDelayTimer()
{
	ClearDelayTimer();
	if (!CanUseIdleVariations())
	{
		StopIdleVariations();
		return;
	}
	PlayIdleVariationNow();
}

void UGridMonsterIdleVariationComponent::HandleIdleVariationDurationTimer()
{
	ClearDurationTimer();
	ActiveIdleDynamicMontage = nullptr;
	CurrentVariationIndex = INDEX_NONE;
	bIdleVariationActive = false;
	LastScheduledDelay = 0.0f;
	RefreshIdleVariationScheduling();
}
