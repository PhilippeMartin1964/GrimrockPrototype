#include "Runtime/GridPartyProgressionReminderState.h"

bool FGridPartyProgressionReminderState::Advance(
	const TMap<FGuid, int32>& PendingCharacterLevels, float ActiveDeltaSeconds)
{
	if (PendingCharacterLevels.IsEmpty())
	{
		Reset();
		return false;
	}

	bool bChanged = PendingCharacterLevels.Num() != LastPendingCharacterLevels.Num();
	if (!bChanged)
	{
		for (const TPair<FGuid, int32>& Pending : PendingCharacterLevels)
		{
			const int32* PreviousLevel = LastPendingCharacterLevels.Find(Pending.Key);
			if (!PreviousLevel || *PreviousLevel != Pending.Value)
			{
				bChanged = true;
				break;
			}
		}
	}

	if (bChanged)
	{
		LastPendingCharacterLevels = PendingCharacterLevels;
		SecondsSinceNotice = 0.0f;
		return true;
	}

	SecondsSinceNotice += FMath::Max(0.0f, ActiveDeltaSeconds);
	if (SecondsSinceNotice >= ReminderIntervalSeconds)
	{
		SecondsSinceNotice = FMath::Fmod(SecondsSinceNotice, ReminderIntervalSeconds);
		return true;
	}
	return false;
}

void FGridPartyProgressionReminderState::Reset()
{
	LastPendingCharacterLevels.Reset();
	SecondsSinceNotice = 0.0f;
}
