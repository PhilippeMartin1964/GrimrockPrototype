#include "Runtime/GridEventFeedSubsystem.h"

void UGridEventFeedSubsystem::Deinitialize()
{
	Buffer.Reset();
	Super::Deinitialize();
}

bool UGridEventFeedSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

bool UGridEventFeedSubsystem::PublishEvent(const FGridHudEventEntry& Entry, bool bIsObservableToPlayer)
{
	FGridHudEventEntry PublishedEntry;
	if (!Buffer.TryPublish(Entry, bIsObservableToPlayer, &PublishedEntry))
	{
		return false;
	}

	OnEventPublished.Broadcast(PublishedEntry);
	return true;
}

const TArray<FGridHudEventEntry>& UGridEventFeedSubsystem::GetEntries() const
{
	return Buffer.GetEntries();
}

TArray<FGridHudEventEntry> UGridEventFeedSubsystem::GetEntriesSnapshot() const
{
	return Buffer.GetEntries();
}

int32 UGridEventFeedSubsystem::GetMaxEventCount() const
{
	return Buffer.GetMaxEntries();
}

void UGridEventFeedSubsystem::SetMaxEventCount(int32 InMaxEventCount)
{
	Buffer.SetMaxEntries(InMaxEventCount);
}
