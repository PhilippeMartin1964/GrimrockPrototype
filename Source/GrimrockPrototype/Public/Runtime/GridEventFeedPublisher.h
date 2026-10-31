#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridEventFeedTypes.h"
#include "Core/GridTypes.h"

class UGridWorldObjectDefinitionAsset;
class AGridLevelRuntimeActor;
class AGrimrockPartyPawn;

enum class EGridDungeonConsequencePerception : uint8
{
	Hidden,
	VisibleNear,
	VisibleDistant,
	Audible
};

namespace GridEventFeedPublisher
{
	/** Publishes an already-resolved event payload into the world Event Feed. */
	GRIMROCKPROTOTYPE_API bool PublishEvent(
		const UObject* WorldContextObject,
		const FGridHudEventEntry& Entry,
		bool bIsObservableToPlayer = true);

	/** EVENTHUD01.XP01: one independent bright-green event per committed
	 * XP reward transaction, regardless of the XP source (monster, trap, etc).
	 * UniformShare > 0 only if every beneficiary received the same amount.
	 * Never merges into an active dungeon consequence group.
	 */
	GRIMROCKPROTOTYPE_API bool PublishPartyExperienceAward(
		const UObject* WorldContextObject, int32 AppliedXP,
		int32 RecipientCount, int32 UniformShare);

	/**
	 * EVENTHUD01.9 — publishes one NPC line that has actually been spoken.
	 * Proposed/unselected dialogue choices must never call this API.
	 */
	GRIMROCKPROTOTYPE_API bool PublishNpcSpokenLine(
		const UObject* WorldContextObject,
		const FText& SpeakerDisplayName,
		const FText& SpokenLine,
		FName SourceId = NAME_None);

	/**
	 * EVENTHUD01.9 — publishes one player/group reply only after it has actually
	 * been selected and spoken. There is intentionally no API for offered choices.
	 */
	GRIMROCKPROTOTYPE_API bool PublishPlayerSpokenReply(
		const UObject* WorldContextObject,
		const FText& SpeakerDisplayName,
		const FText& SpokenLine,
		FName SourceId = NAME_None);

	/** Player-facing localized header for a readable world object. */
	GRIMROCKPROTOTYPE_API FText ResolveReadableHeader(const UGridWorldObjectDefinitionAsset* Definition);
	/** Conservative anti-spoiler rule for cell-bound observable dungeon effects. */
	GRIMROCKPROTOTYPE_API bool IsCellObservable(const FIntPoint& PartyCell, const FIntPoint& EventCell, int32 MaxManhattanDistance = 1);

	/** Stable non-player-facing source identity for runtime object events. */
	GRIMROCKPROTOTYPE_API FName SourceIdFromGuid(const FGuid& SourceGuid);

	/** EVENTHUD01.12 player perception of a consequence at one dungeon cell. */
	GRIMROCKPROTOTYPE_API EGridDungeonConsequencePerception ResolveCellConsequencePerception(
		const AGridLevelRuntimeActor* RuntimeActor,
		const AGrimrockPartyPawn* PartyPawn,
		const FIntPoint& EventCell,
		FName EventCode = NAME_None,
		EGridEdge EventEdge = EGridEdge::None,
		int32 VisualRangeCells = 6,
		int32 HearingRangeCells = 6);

	/** Converts a resolved consequence into non-omniscient player-facing wording. */
	GRIMROCKPROTOTYPE_API FText ResolvePerceivedConsequenceText(
		FName EventCode,
		const FText& VisibleFallback,
		EGridDungeonConsequencePerception Perception);

	/** Opens one synchronous cause/consequence group. */
	GRIMROCKPROTOTYPE_API bool BeginDungeonEventGroup(
		const UObject* WorldContextObject,
		FName EventCode,
		const FText& RootMessage,
		EGridHudEventSemantic Semantic,
		EGridEventVisibility Visibility,
		bool bRootObservableToPlayer,
		FName SourceId = NAME_None);

	GRIMROCKPROTOTYPE_API bool BeginDungeonEventGroupAtCell(
		const UObject* WorldContextObject,
		int32 CellX,
		int32 CellY,
		FName EventCode,
		const FText& RootMessage,
		EGridHudEventSemantic Semantic,
		EGridEventVisibility Visibility = EGridEventVisibility::Observable,
		FName SourceId = NAME_None,
		int32 MaxManhattanDistance = 1);

	GRIMROCKPROTOTYPE_API bool EndDungeonEventGroup(const UObject* WorldContextObject);
	GRIMROCKPROTOTYPE_API void CancelDungeonEventGroup(const UObject* WorldContextObject);

	/** Publishes one already-resolved dungeon event without imposing gameplay rules. */
	GRIMROCKPROTOTYPE_API bool PublishDungeonEvent(
		const UObject* WorldContextObject,
		FName EventCode,
		const FText& Message,
		EGridHudEventSemantic Semantic,
		EGridEventVisibility Visibility,
		bool bIsObservableToPlayer,
		FName SourceId = NAME_None,
		FText Header = FText::GetEmpty());

	/**
	 * Publishes a cell-bound dungeon event.
	 *
	 * Observable events are accepted only when the party is within the
	 * conservative Manhattan-distance window. Always/Hidden retain their core
	 * visibility semantics.
	 */
	GRIMROCKPROTOTYPE_API bool PublishDungeonEventAtCell(
		const UObject* WorldContextObject,
		int32 CellX,
		int32 CellY,
		FName EventCode,
		const FText& Message,
		EGridHudEventSemantic Semantic,
		EGridEventVisibility Visibility = EGridEventVisibility::Observable,
		FName SourceId = NAME_None,
		int32 MaxManhattanDistance = 1);

	/** Spatially resolved consequence: visible, audible, or deliberately hidden. */
	GRIMROCKPROTOTYPE_API bool PublishPerceivedDungeonConsequenceAtCell(
		const AGridLevelRuntimeActor* RuntimeActor,
		int32 CellX,
		int32 CellY,
		FName EventCode,
		const FText& VisibleMessage,
		EGridHudEventSemantic Semantic,
		FName SourceId = NAME_None,
		EGridEdge EventEdge = EGridEdge::None,
		int32 VisualRangeCells = 6,
		int32 HearingRangeCells = 6);
}
