#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridEventFeedTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridEventFeedSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridEventFeedPublishedSignature, const FGridHudEventEntry&, Entry);

/**
 * World-scoped authority for the transient player-facing Event Feed.
 *
 * Gameplay systems publish resolved information here. This subsystem owns only
 * ordering, observability policy, bounded history and presentation notification;
 * it never computes combat, interaction or dialogue rules.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridEventFeedSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	/**
	 * Adds an event when its visibility policy allows it.
	 *
	 * Producers of Observable events pass whether the effect is currently
	 * perceptible by the player. Hidden is always rejected; Always is always
	 * accepted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grimrock|Event Feed")
	bool PublishEvent(const FGridHudEventEntry& Entry, bool bIsObservableToPlayer = true);

	/** C++ zero-copy read access for future native presenters. */
	const TArray<FGridHudEventEntry>& GetEntries() const;

	/** Blueprint-friendly snapshot; EVENTHUD01.2 can consume this without owning state. */
	UFUNCTION(BlueprintPure, Category = "Grimrock|Event Feed")
	TArray<FGridHudEventEntry> GetEntriesSnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Grimrock|Event Feed")
	int32 GetMaxEventCount() const;

	/** Runtime preference hook. Values below one are clamped to one. */
	UFUNCTION(BlueprintCallable, Category = "Grimrock|Event Feed")
	void SetMaxEventCount(int32 InMaxEventCount);

	/** Broadcast only after an entry has passed visibility policy and entered the buffer. */
	UPROPERTY(BlueprintAssignable, Category = "Grimrock|Event Feed")
	FGridEventFeedPublishedSignature OnEventPublished;

private:
	FGridEventFeedBuffer Buffer;
};
