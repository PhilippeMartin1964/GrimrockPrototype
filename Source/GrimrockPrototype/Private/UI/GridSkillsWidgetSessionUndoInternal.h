#pragma once

#include "CoreMinimal.h"

/**
 * Internal policy for reconciling UI-session-only Skill undo history.
 *
 * The authoritative rank is gameplay state. The two maps only describe ranks
 * purchased through the currently open Skills allocation session.
 */
namespace GridSkillsWidgetSessionUndo
{
	/**
	 * Returns true only when stale/incomplete history was invalidated.
	 *
	 * A coherent tracked Skill must satisfy:
	 *   AuthoritativeRank == SessionFloor + SessionPurchaseCount
	 *
	 * Reconciliation is suppressed during the synchronous inventory notification
	 * emitted by the widget's own purchase/refund transaction because the
	 * authoritative rank changes before the UI history is recorded/consumed.
	 */
	inline bool ReconcileHistory(
		const FString& Key,
		int32 AuthoritativeRank,
		bool bSuppressReconciliation,
		TMap<FString, int32>& PurchasedRankCounts,
		TMap<FString, int32>& RankFloors)
	{
		if (bSuppressReconciliation || Key.IsEmpty())
		{
			return false;
		}

		const int32* PurchaseCount = PurchasedRankCounts.Find(Key);
		const int32* RankFloor = RankFloors.Find(Key);
		if (!PurchaseCount && !RankFloor)
		{
			return false;
		}

		const bool bCoherent =
			PurchaseCount &&
			RankFloor &&
			*PurchaseCount > 0 &&
			*RankFloor >= 0 &&
			AuthoritativeRank >= 0 &&
			static_cast<int64>(AuthoritativeRank) ==
				static_cast<int64>(*RankFloor) + static_cast<int64>(*PurchaseCount);

		if (bCoherent)
		{
			return false;
		}

		PurchasedRankCounts.Remove(Key);
		RankFloors.Remove(Key);
		return true;
	}
}
