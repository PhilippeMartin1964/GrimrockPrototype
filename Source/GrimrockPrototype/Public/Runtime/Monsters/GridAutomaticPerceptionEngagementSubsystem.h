#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridAutomaticPerceptionEngagementSubsystem.generated.h"

class AGridLevelRuntimeActor;

DECLARE_LOG_CATEGORY_EXTERN(LogGridAutomaticEngagement, Log, All);

namespace GridAutomaticPerceptionEngagement
{
	/**
     * Synchronous scope used only by MON14.1 automatic engagement.
     * Perception is still fully refreshed (sight + hearing), but a monster is
     * considered a direct combat source only when it can actually see the party.
     */
	class GRIMROCKPROTOTYPE_API FScopedVisualSourceRequirement
	{
	public:
		FScopedVisualSourceRequirement();
		~FScopedVisualSourceRequirement();

		FScopedVisualSourceRequirement(const FScopedVisualSourceRequirement&) = delete;
		FScopedVisualSourceRequirement& operator=(const FScopedVisualSourceRequirement&) = delete;
	};

	GRIMROCKPROTOTYPE_API bool IsVisualSourceRequired();

	/** Queue one coalesced automatic-perception evaluation for this runtime. */
	GRIMROCKPROTOTYPE_API void Request(AGridLevelRuntimeActor* RuntimeActor, FName Reason);
}

/**
 * MON14.1 event-driven bridge between exploration perception and combat.
 *
 * No perception work is performed in Tick. Producers only request an
 * evaluation; requests are coalesced and processed on the next safe runtime
 * tick so movement interpolation, rebuilds and atomic encounter transactions
 * can finish first.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridAutomaticPerceptionEngagementSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RequestEvaluation(AGridLevelRuntimeActor* RuntimeActor, FName Reason);

	/** Executes the currently queued request immediately. Used by automation tests. */
	bool ProcessPendingEvaluationNow();

	int32 GetQueuedRequestCount() const
	{
		return QueuedRequestCount;
	}
	int32 GetEffectiveEvaluationCount() const
	{
		return EffectiveEvaluationCount;
	}
	int32 GetSuccessfulStartCount() const
	{
		return SuccessfulStartCount;
	}
	bool HasPendingEvaluation() const
	{
		return bEvaluationQueued;
	}

private:
	void HandleDeferredEvaluation();
	void RequeueAfterUnsafeRuntime(AGridLevelRuntimeActor* RuntimeActor);

	TWeakObjectPtr<AGridLevelRuntimeActor> PendingRuntimeActor;
	FName PendingReason = NAME_None;
	bool bEvaluationQueued = false;
	int32 QueuedRequestCount = 0;
	int32 EffectiveEvaluationCount = 0;
	int32 SuccessfulStartCount = 0;
};
