#pragma once

#include "CoreMinimal.h"
#include "GridEventFeedTypes.generated.h"

/** High-level category used by the three Event Feed filters. */
UENUM(BlueprintType)
enum class EGridHudEventCategory : uint8
{
	Dungeon UMETA(DisplayName = "Events"),
	Combat UMETA(DisplayName = "Combat"),
	Dialogue UMETA(DisplayName = "Discussion")
};

/** Presentation semantic kept separate from the high-level filter category. */
UENUM(BlueprintType)
enum class EGridHudEventSemantic : uint8
{
	Default UMETA(DisplayName = "Default"),
	Information UMETA(DisplayName = "Information"),
	Interaction UMETA(DisplayName = "Interaction"),
	Locked UMETA(DisplayName = "Locked"),
	Mechanism UMETA(DisplayName = "Mechanism"),
	Secret UMETA(DisplayName = "Secret"),
	Danger UMETA(DisplayName = "Danger"),
	PhysicalDamage UMETA(DisplayName = "Physical Damage"),
	Fire UMETA(DisplayName = "Fire"),
	Ice UMETA(DisplayName = "Ice"),
	Lightning UMETA(DisplayName = "Lightning"),
	Poison UMETA(DisplayName = "Poison"),
	Arcane UMETA(DisplayName = "Arcane"),
	Healing UMETA(DisplayName = "Healing"),
	Buff UMETA(DisplayName = "Buff"),
	Debuff UMETA(DisplayName = "Debuff"),
	Dialogue UMETA(DisplayName = "Dialogue")
};

/** Controls whether a runtime event is eligible for the player-facing feed. */
UENUM(BlueprintType)
enum class EGridEventVisibility : uint8
{
	Hidden UMETA(DisplayName = "Hidden"),
	Observable UMETA(DisplayName = "Observable"),
	Always UMETA(DisplayName = "Always")
};

/**
 * Optional semantic fragment inside an event message.
 *
 * EVENTHUD01.1 stores fragments without rendering them. A later shared semantic
 * text system can use Semantic and SemanticId for native UMG colour/tool-tip
 * presentation without changing producer payloads.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridHudEventTextFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	EGridHudEventSemantic Semantic = EGridHudEventSemantic::Default;

	/** Stable optional semantic key for future shared tool-tip lookup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	FName SemanticId = NAME_None;
};

/** One logical Event Feed entry. Multi-line combat detail still counts as one entry. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridHudEventEntry
{
	GENERATED_BODY()

	/** Monotonic runtime sequence assigned by the Event Feed authority. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event Feed")
	int64 SequenceId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	EGridHudEventCategory Category = EGridHudEventCategory::Dungeon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	EGridHudEventSemantic Semantic = EGridHudEventSemantic::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	EGridEventVisibility Visibility = EGridEventVisibility::Observable;

	/** Stable producer-facing code; never used as localized player text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	FName EventCode = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	FText Header;

	/** Simple message fallback for entries that do not need semantic fragments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	FText Message;

	/** Optional structured message fragments for later semantic rich-text rendering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	TArray<FGridHudEventTextFragment> MessageFragments;

	/** Optional additional lines. Structured combat tracing is introduced by EVENTHUD01.7. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	TArray<FText> Details;

	/** Stable source identity when the producer has one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Feed")
	FName SourceId = NAME_None;
};

/**
 * Small runtime-only FIFO backing store kept independent from UWorldSubsystem so
 * ordering, visibility and overflow policy can be unit-tested without PIE.
 */
class GRIMROCKPROTOTYPE_API FGridEventFeedBuffer
{
public:
	static constexpr int32 DefaultMaxEntries = 200;

	explicit FGridEventFeedBuffer(int32 InMaxEntries = DefaultMaxEntries);

	/**
	 * Publishes one candidate entry.
	 *
	 * Hidden entries are always rejected. Observable entries require
	 * bIsObservableToPlayer. Always entries bypass that observation test.
	 * Sequence ids are assigned only to accepted entries.
	 */
	bool TryPublish(const FGridHudEventEntry& Entry, bool bIsObservableToPlayer, FGridHudEventEntry* OutPublishedEntry = nullptr);

	void SetMaxEntries(int32 InMaxEntries);
	int32 GetMaxEntries() const;
	const TArray<FGridHudEventEntry>& GetEntries() const;
	void Reset();

private:
	static bool ShouldAccept(EGridEventVisibility Visibility, bool bIsObservableToPlayer);
	void TrimToCapacity();

	TArray<FGridHudEventEntry> Entries;
	int64 NextSequenceId = 1;
	int32 MaxEntries = DefaultMaxEntries;
};
