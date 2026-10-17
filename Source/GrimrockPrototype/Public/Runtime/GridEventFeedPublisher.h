#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridEventFeedTypes.h"

namespace GridEventFeedPublisher
{
	/** Conservative anti-spoiler rule for cell-bound observable dungeon effects. */
	GRIMROCKPROTOTYPE_API bool IsCellObservable(const FIntPoint& PartyCell, const FIntPoint& EventCell, int32 MaxManhattanDistance = 1);

	/** Stable non-player-facing source identity for runtime object events. */
	GRIMROCKPROTOTYPE_API FName SourceIdFromGuid(const FGuid& SourceGuid);

	/** Publishes one already-resolved dungeon event without imposing gameplay rules. */
	GRIMROCKPROTOTYPE_API bool PublishDungeonEvent(
		const UObject* WorldContextObject,
		FName EventCode,
		const FText& Message,
		EGridHudEventSemantic Semantic,
		EGridEventVisibility Visibility,
		bool bIsObservableToPlayer,
		FName SourceId = NAME_None);

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
}
