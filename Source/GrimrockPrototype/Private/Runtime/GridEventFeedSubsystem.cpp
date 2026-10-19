#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GrimrockGameInstance.h"

#include "Engine/World.h"

void UGridEventFeedSubsystem::Deinitialize()
{
	Buffer.Reset();
	DungeonEventGroup.Reset();
	Super::Deinitialize();
}

bool UGridEventFeedSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

bool UGridEventFeedSubsystem::PublishEvent(const FGridHudEventEntry& Entry, bool bIsObservableToPlayer)
{
	FGridHudEventEntry Candidate = Entry;
	if (!Candidate.bHasGameplayTimestamp)
	{
		if (UWorld* World = GetWorld())
		{
			if (const UGrimrockGameInstance* GameInstance =
				World->GetGameInstance<UGrimrockGameInstance>())
			{
				Candidate.bHasGameplayTimestamp = true;
				Candidate.GameplayElapsedSeconds =
					FMath::Max<int64>(0, GameInstance->GetGameplayElapsedWholeSeconds());
			}
		}
	}

	FGridHudEventEntry PublishedEntry;
	if (!Buffer.TryPublish(Candidate, bIsObservableToPlayer, &PublishedEntry))
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

bool UGridEventFeedSubsystem::BeginDungeonEventGroup(
	const FGridHudEventEntry& RootEntry,
	bool bRootObservableToPlayer)
{
	if (DungeonEventGroup.bActive || RootEntry.Category != EGridHudEventCategory::Dungeon || RootEntry.Message.IsEmpty())
	{
		return false;
	}

	DungeonEventGroup.bActive = true;
	DungeonEventGroup.RootEntry = RootEntry;
	DungeonEventGroup.bRootObservableToPlayer = bRootObservableToPlayer;
	DungeonEventGroup.Consequences.Reset();
	DungeonEventGroup.ConsequenceSemantics.Reset();
	return true;
}

bool UGridEventFeedSubsystem::AppendDungeonEventGroupConsequence(
	const FText& Consequence,
	EGridHudEventSemantic Semantic)
{
	if (!DungeonEventGroup.bActive || Consequence.IsEmpty())
	{
		return false;
	}
	DungeonEventGroup.Consequences.Add(Consequence);
	DungeonEventGroup.ConsequenceSemantics.Add(Semantic);
	return true;
}

bool UGridEventFeedSubsystem::EndDungeonEventGroup()
{
	if (!DungeonEventGroup.bActive)
	{
		return false;
	}

	FGridHudEventEntry FinalEntry = DungeonEventGroup.RootEntry;
	const bool bRootVisible =
		FinalEntry.Visibility == EGridEventVisibility::Always ||
		(FinalEntry.Visibility == EGridEventVisibility::Observable && DungeonEventGroup.bRootObservableToPlayer);

	TArray<FText> Clauses;
	if (bRootVisible && !FinalEntry.Message.IsEmpty())
	{
		Clauses.Add(FinalEntry.Message);
	}
	Clauses.Append(DungeonEventGroup.Consequences);
	const TArray<EGridHudEventSemantic> ConsequenceSemantics = DungeonEventGroup.ConsequenceSemantics;

	DungeonEventGroup.Reset();
	if (Clauses.IsEmpty())
	{
		return false;
	}

	auto NormalizeClause = [](const FText& TextValue)
	{
		FString Value = TextValue.ToString().TrimStartAndEnd();
		while (Value.EndsWith(TEXT(".")))
		{
			Value.LeftChopInline(1);
			Value.TrimEndInline();
		}
		return Value;
	};

	FString Combined;
	for (int32 Index = 0; Index < Clauses.Num(); ++Index)
	{
		const FString Clause = NormalizeClause(Clauses[Index]);
		if (Clause.IsEmpty())
		{
			continue;
		}
		if (!Combined.IsEmpty())
		{
			Combined += Index == 1 && bRootVisible ? TEXT(" → ") : TEXT(" · ");
		}
		Combined += Clause;
	}
	if (Combined.IsEmpty())
	{
		return false;
	}
	Combined += TEXT(".");

	FinalEntry.Message = FText::FromString(Combined);
	FinalEntry.Header = bRootVisible ? FinalEntry.Header : FText::GetEmpty();
	if (!bRootVisible)
	{
		FinalEntry.EventCode = FName(TEXT("Dungeon.Consequence.Group"));
		FinalEntry.Semantic = ConsequenceSemantics.IsEmpty()
			? EGridHudEventSemantic::Information
			: ConsequenceSemantics[0];
		FinalEntry.SourceId = NAME_None;
	}
	FinalEntry.Visibility = EGridEventVisibility::Always;
	return PublishEvent(FinalEntry, true);
}

void UGridEventFeedSubsystem::CancelDungeonEventGroup()
{
	DungeonEventGroup.Reset();
}

bool UGridEventFeedSubsystem::HasActiveDungeonEventGroup() const
{
	return DungeonEventGroup.bActive;
}
