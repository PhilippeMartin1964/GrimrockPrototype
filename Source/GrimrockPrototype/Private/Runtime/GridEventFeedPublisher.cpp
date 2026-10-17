#include "Runtime/GridEventFeedPublisher.h"

#include "Core/GridWorldObjectDefinitionAsset.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridEventFeedPublisher
{
	FText ResolveReadableHeader(const UGridWorldObjectDefinitionAsset* Definition)
	{
		if (Definition && Definition->DefinitionId == FName(TEXT("WallInscription")))
		{
			return NSLOCTEXT("GridEventFeed", "WallInscriptionHeader", "Écriture murale :");
		}
		if (Definition && !Definition->DisplayName.IsEmpty())
		{
			return Definition->DisplayName;
		}
		return NSLOCTEXT("GridEventFeed", "ReadableHeader", "Inscription :");
	}

	bool IsCellObservable(const FIntPoint& PartyCell, const FIntPoint& EventCell, int32 MaxManhattanDistance)
	{
		const int32 SafeDistance = FMath::Max(0, MaxManhattanDistance);
		const int32 Distance = FMath::Abs(PartyCell.X - EventCell.X) + FMath::Abs(PartyCell.Y - EventCell.Y);
		return Distance <= SafeDistance;
	}

	FName SourceIdFromGuid(const FGuid& SourceGuid)
	{
		return SourceGuid.IsValid() ? FName(*SourceGuid.ToString(EGuidFormats::Digits)) : NAME_None;
	}

	bool PublishDungeonEvent(
		const UObject* WorldContextObject,
		FName EventCode,
		const FText& Message,
		EGridHudEventSemantic Semantic,
		EGridEventVisibility Visibility,
		bool bIsObservableToPlayer,
		FName SourceId,
		FText Header)
	{
		if (!WorldContextObject || Message.IsEmpty())
		{
			return false;
		}

		UWorld* World = WorldContextObject->GetWorld();
		UGridEventFeedSubsystem* EventFeed = World ? World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
		if (!EventFeed)
		{
			return false;
		}

		FGridHudEventEntry Entry;
		Entry.Category = EGridHudEventCategory::Dungeon;
		Entry.Semantic = Semantic;
		Entry.Visibility = Visibility;
		Entry.EventCode = EventCode;
		Entry.Header = Header;
		Entry.Message = Message;
		Entry.SourceId = SourceId;
		return EventFeed->PublishEvent(Entry, bIsObservableToPlayer);
	}

	bool PublishDungeonEventAtCell(
		const UObject* WorldContextObject,
		int32 CellX,
		int32 CellY,
		FName EventCode,
		const FText& Message,
		EGridHudEventSemantic Semantic,
		EGridEventVisibility Visibility,
		FName SourceId,
		int32 MaxManhattanDistance)
	{
		bool bObservable = Visibility == EGridEventVisibility::Always;
		if (Visibility == EGridEventVisibility::Observable && WorldContextObject)
		{
			if (UWorld* World = WorldContextObject->GetWorld())
			{
				if (const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(World, 0)))
				{
					bObservable = IsCellObservable(
						FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY),
						FIntPoint(CellX, CellY),
						MaxManhattanDistance);
				}
			}
		}

		return PublishDungeonEvent(
			WorldContextObject,
			EventCode,
			Message,
			Semantic,
			Visibility,
			bObservable,
			SourceId);
	}
}
