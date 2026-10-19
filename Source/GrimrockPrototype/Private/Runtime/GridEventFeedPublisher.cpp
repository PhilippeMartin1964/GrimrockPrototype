#include "Runtime/GridEventFeedPublisher.h"

#include "Core/GridWorldObjectDefinitionAsset.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace GridEventFeedPublisher
{
	bool PublishEvent(const UObject* WorldContextObject, const FGridHudEventEntry& Entry, bool bIsObservableToPlayer)
	{
		if (!WorldContextObject)
		{
			return false;
		}

		UWorld* World = WorldContextObject->GetWorld();
		UGridEventFeedSubsystem* EventFeed = World ? World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
		return EventFeed ? EventFeed->PublishEvent(Entry, bIsObservableToPlayer) : false;
	}

	bool PublishNpcSpokenLine(
		const UObject* WorldContextObject,
		const FText& SpeakerDisplayName,
		const FText& SpokenLine,
		FName SourceId)
	{
		if (!WorldContextObject || SpeakerDisplayName.IsEmpty() || SpokenLine.IsEmpty())
		{
			return false;
		}

		FGridHudEventEntry Entry;
		Entry.Category = EGridHudEventCategory::Dialogue;
		Entry.Semantic = EGridHudEventSemantic::Dialogue;
		Entry.Visibility = EGridEventVisibility::Always;
		Entry.EventCode = FName(TEXT("Dialogue.NPC.Spoken"));
		Entry.Header = SpeakerDisplayName;
		Entry.Message = SpokenLine;
		Entry.SourceId = SourceId;
		return PublishEvent(WorldContextObject, Entry, true);
	}

	bool PublishPlayerSpokenReply(
		const UObject* WorldContextObject,
		const FText& SpeakerDisplayName,
		const FText& SpokenLine,
		FName SourceId)
	{
		if (!WorldContextObject || SpeakerDisplayName.IsEmpty() || SpokenLine.IsEmpty())
		{
			return false;
		}

		FGridHudEventEntry Entry;
		Entry.Category = EGridHudEventCategory::Dialogue;
		Entry.Semantic = EGridHudEventSemantic::Dialogue;
		Entry.Visibility = EGridEventVisibility::Always;
		Entry.EventCode = FName(TEXT("Dialogue.Player.Spoken"));
		Entry.Header = SpeakerDisplayName;
		Entry.Message = SpokenLine;
		Entry.SourceId = SourceId;
		return PublishEvent(WorldContextObject, Entry, true);
	}

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

	EGridDungeonConsequencePerception ResolveCellConsequencePerception(
		const AGridLevelRuntimeActor* RuntimeActor,
		const AGrimrockPartyPawn* PartyPawn,
		const FIntPoint& EventCell,
		FName EventCode,
		EGridEdge EventEdge,
		int32 VisualRangeCells,
		int32 HearingRangeCells)
	{
		if (!RuntimeActor || !PartyPawn)
		{
			return EGridDungeonConsequencePerception::Hidden;
		}

		const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
		const int32 Distance = FGridMonsterPathfinder::ManhattanDistance(PartyCell, EventCell);
		const bool bDoorEvent =
			EventCode == FName(TEXT("Dungeon.Door.Opened")) ||
			EventCode == FName(TEXT("Dungeon.Door.Closed")) ||
			EventCode == FName(TEXT("Dungeon.SecretDoor.Opened")) ||
			EventCode == FName(TEXT("Dungeon.SecretDoor.Closed"));

		// A closed door blocks CanMove through its own edge but remains visible as
		// the boundary directly in front of the party.
		auto GetOppositeEdge = [](EGridEdge Edge)
		{
			switch (Edge)
			{
				case EGridEdge::North: return EGridEdge::South;
				case EGridEdge::East: return EGridEdge::West;
				case EGridEdge::South: return EGridEdge::North;
				case EGridEdge::West: return EGridEdge::East;
				default: return EGridEdge::None;
			}
		};

		bool bVisibleDoorBoundary = false;
		if (bDoorEvent && EventEdge != EGridEdge::None)
		{
			if (Distance == 0)
			{
				bVisibleDoorBoundary = EventEdge == PartyPawn->Facing;
			}
			else if (Distance == 1 &&
				FGridMonsterPerception::IsTargetInFacingDirection(PartyCell, PartyPawn->Facing, EventCell))
			{
				bVisibleDoorBoundary = EventEdge == GetOppositeEdge(PartyPawn->Facing);
			}
		}

		const bool bVisible = bVisibleDoorBoundary || FGridMonsterPerception::HasDirectionalLineOfSight(
			PartyCell,
			PartyPawn->Facing,
			EventCell,
			FMath::Max(0, VisualRangeCells),
			[RuntimeActor](const FIntPoint& From, const FIntPoint& To)
			{
				const EGridEdge Direction = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(From, To);
				return Direction != EGridEdge::None && RuntimeActor->CanMove(From.X, From.Y, Direction);
			});
		if (bVisible)
		{
			return Distance <= 1
				? EGridDungeonConsequencePerception::VisibleNear
				: EGridDungeonConsequencePerception::VisibleDistant;
		}

		const bool bAudible = FGridMonsterPerception::CanHearThroughGrid(
			PartyCell,
			EventCell,
			FMath::Max(0, HearingRangeCells),
			[RuntimeActor](const FIntPoint& From, const FIntPoint& To)
			{
				const EGridEdge Direction = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(From, To);
				return Direction != EGridEdge::None && RuntimeActor->CanSoundTraverse(From.X, From.Y, Direction);
			});
		return bAudible ? EGridDungeonConsequencePerception::Audible : EGridDungeonConsequencePerception::Hidden;
	}

	FText ResolvePerceivedConsequenceText(
		FName EventCode,
		const FText& VisibleFallback,
		EGridDungeonConsequencePerception Perception)
	{
		if (Perception == EGridDungeonConsequencePerception::Hidden)
		{
			return FText::GetEmpty();
		}

		if (Perception == EGridDungeonConsequencePerception::VisibleNear ||
			Perception == EGridDungeonConsequencePerception::VisibleDistant)
		{
			const bool bNear = Perception == EGridDungeonConsequencePerception::VisibleNear;
			if (EventCode == FName(TEXT("Dungeon.Door.Opened")))
			{
				return bNear
					? NSLOCTEXT("GridEventFeed", "PerceivedDoorOpenedNear", "Une porte devant vous s'ouvre.")
					: NSLOCTEXT("GridEventFeed", "PerceivedDoorOpenedDistant", "Une porte s'ouvre au fond du couloir.");
			}
			if (EventCode == FName(TEXT("Dungeon.Door.Closed")))
			{
				return bNear
					? NSLOCTEXT("GridEventFeed", "PerceivedDoorClosedNear", "Une porte devant vous se ferme.")
					: NSLOCTEXT("GridEventFeed", "PerceivedDoorClosedDistant", "Une porte se ferme au fond du couloir.");
			}
			if (EventCode == FName(TEXT("Dungeon.SecretDoor.Opened")) ||
				EventCode == FName(TEXT("Dungeon.SecretDoor.Closed")))
			{
				return bNear
					? NSLOCTEXT("GridEventFeed", "PerceivedSecretDoorNear", "Une partie du mur devant vous se met en mouvement.")
					: NSLOCTEXT("GridEventFeed", "PerceivedSecretDoorDistant", "Une partie du mur se met en mouvement au loin.");
			}
			return VisibleFallback;
		}

		if (EventCode == FName(TEXT("Dungeon.Door.Opened")))
		{
			return NSLOCTEXT("GridEventFeed", "PerceivedDoorOpenedAudible", "Vous entendez une porte s'ouvrir au loin.");
		}
		if (EventCode == FName(TEXT("Dungeon.Door.Closed")))
		{
			return NSLOCTEXT("GridEventFeed", "PerceivedDoorClosedAudible", "Vous entendez une porte se fermer au loin.");
		}
		return NSLOCTEXT("GridEventFeed", "PerceivedMechanismAudible", "Vous entendez un mécanisme au loin.");
	}

	bool BeginDungeonEventGroup(
		const UObject* WorldContextObject,
		FName EventCode,
		const FText& RootMessage,
		EGridHudEventSemantic Semantic,
		EGridEventVisibility Visibility,
		bool bRootObservableToPlayer,
		FName SourceId)
	{
		if (!WorldContextObject || RootMessage.IsEmpty())
		{
			return false;
		}
		UWorld* World = WorldContextObject->GetWorld();
		UGridEventFeedSubsystem* Feed = World ? World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
		if (!Feed)
		{
			return false;
		}

		FGridHudEventEntry RootEntry;
		RootEntry.Category = EGridHudEventCategory::Dungeon;
		RootEntry.Semantic = Semantic;
		RootEntry.Visibility = Visibility;
		RootEntry.EventCode = EventCode;
		RootEntry.Message = RootMessage;
		RootEntry.SourceId = SourceId;
		return Feed->BeginDungeonEventGroup(RootEntry, bRootObservableToPlayer);
	}

	bool BeginDungeonEventGroupAtCell(
		const UObject* WorldContextObject,
		int32 CellX,
		int32 CellY,
		FName EventCode,
		const FText& RootMessage,
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
		return BeginDungeonEventGroup(
			WorldContextObject, EventCode, RootMessage, Semantic, Visibility, bObservable, SourceId);
	}

	bool EndDungeonEventGroup(const UObject* WorldContextObject)
	{
		UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		UGridEventFeedSubsystem* Feed = World ? World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
		return Feed ? Feed->EndDungeonEventGroup() : false;
	}

	void CancelDungeonEventGroup(const UObject* WorldContextObject)
	{
		UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		if (UGridEventFeedSubsystem* Feed = World ? World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr)
		{
			Feed->CancelDungeonEventGroup();
		}
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

		FGridHudEventEntry Entry;
		Entry.Category = EGridHudEventCategory::Dungeon;
		Entry.Semantic = Semantic;
		Entry.Visibility = Visibility;
		Entry.EventCode = EventCode;
		Entry.Header = Header;
		Entry.Message = Message;
		Entry.SourceId = SourceId;

		UWorld* World = WorldContextObject->GetWorld();
		UGridEventFeedSubsystem* Feed = World ? World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
		const bool bEligibleForPlayer =
			Visibility == EGridEventVisibility::Always ||
			(Visibility == EGridEventVisibility::Observable && bIsObservableToPlayer);
		if (Feed && Feed->HasActiveDungeonEventGroup())
		{
			return bEligibleForPlayer && Feed->AppendDungeonEventGroupConsequence(Message, Semantic);
		}
		return PublishEvent(WorldContextObject, Entry, bIsObservableToPlayer);
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

	bool PublishPerceivedDungeonConsequenceAtCell(
		const AGridLevelRuntimeActor* RuntimeActor,
		int32 CellX,
		int32 CellY,
		FName EventCode,
		const FText& VisibleMessage,
		EGridHudEventSemantic Semantic,
		FName SourceId,
		EGridEdge EventEdge,
		int32 VisualRangeCells,
		int32 HearingRangeCells)
	{
		if (!RuntimeActor)
		{
			return false;
		}
		const UWorld* World = RuntimeActor->GetWorld();
		const AGrimrockPartyPawn* PartyPawn =
			World ? Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
		const EGridDungeonConsequencePerception Perception = ResolveCellConsequencePerception(
			RuntimeActor, PartyPawn, FIntPoint(CellX, CellY), EventCode, EventEdge, VisualRangeCells, HearingRangeCells);
		const FText PerceivedMessage = ResolvePerceivedConsequenceText(EventCode, VisibleMessage, Perception);
		if (PerceivedMessage.IsEmpty())
		{
			return false;
		}
		return PublishDungeonEvent(
			RuntimeActor,
			EventCode,
			PerceivedMessage,
			Semantic,
			EGridEventVisibility::Always,
			true,
			SourceId);
	}
}
