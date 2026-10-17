#include "Runtime/GridEventFeedTypes.h"

FGridEventFeedBuffer::FGridEventFeedBuffer(int32 InMaxEntries)
{
	SetMaxEntries(InMaxEntries);
}

bool FGridEventFeedBuffer::TryPublish(
	const FGridHudEventEntry& Entry, bool bIsObservableToPlayer, FGridHudEventEntry* OutPublishedEntry)
{
	if (!ShouldAccept(Entry.Visibility, bIsObservableToPlayer))
	{
		return false;
	}

	FGridHudEventEntry PublishedEntry = Entry;
	PublishedEntry.SequenceId = NextSequenceId++;
	Entries.Add(PublishedEntry);
	TrimToCapacity();

	if (OutPublishedEntry)
	{
		*OutPublishedEntry = PublishedEntry;
	}
	return true;
}

void FGridEventFeedBuffer::SetMaxEntries(int32 InMaxEntries)
{
	MaxEntries = FMath::Max(1, InMaxEntries);
	TrimToCapacity();
}

int32 FGridEventFeedBuffer::GetMaxEntries() const
{
	return MaxEntries;
}

const TArray<FGridHudEventEntry>& FGridEventFeedBuffer::GetEntries() const
{
	return Entries;
}

void FGridEventFeedBuffer::Reset()
{
	Entries.Reset();
	NextSequenceId = 1;
}

bool FGridEventFeedBuffer::ShouldAccept(EGridEventVisibility Visibility, bool bIsObservableToPlayer)
{
	switch (Visibility)
	{
		case EGridEventVisibility::Hidden:
			return false;

		case EGridEventVisibility::Always:
			return true;

		case EGridEventVisibility::Observable:
		default:
			return bIsObservableToPlayer;
	}
}

void FGridEventFeedBuffer::TrimToCapacity()
{
	const int32 OverflowCount = Entries.Num() - MaxEntries;
	if (OverflowCount > 0)
	{
		Entries.RemoveAt(0, OverflowCount, EAllowShrinking::No);
	}
}
