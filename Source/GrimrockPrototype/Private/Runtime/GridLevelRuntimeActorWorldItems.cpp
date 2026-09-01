#include "Runtime/GridLevelRuntimeActor.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
	FString GetWorldItemEdgeText(EGridEdge Edge)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			return EdgeEnum->GetNameStringByValue(static_cast<int64>(Edge));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Edge));
	}

	EGridEdge GetAdjacentPickupDirection(const FIntPoint& FromCell, const FIntPoint& ToCell)
	{
		const FIntPoint Delta = ToCell - FromCell;
		if (Delta == FIntPoint(0, 1))
		{
			return EGridEdge::North;
		}
		if (Delta == FIntPoint(1, 0))
		{
			return EGridEdge::East;
		}
		if (Delta == FIntPoint(0, -1))
		{
			return EGridEdge::South;
		}
		if (Delta == FIntPoint(-1, 0))
		{
			return EGridEdge::West;
		}
		return EGridEdge::None;
	}

	FName ResolveWorldPickupItemDefinitionId(const AGridItemActor* ItemActor, FName FallbackArchetypeId)
	{
		if (!ItemActor)
		{
			return FallbackArchetypeId;
		}
		if (const UGridItemDefinitionAsset* Definition = ItemActor->GetItemDefinitionAsset())
		{
			if (!Definition->ItemDefinitionId.IsNone())
			{
				return Definition->ItemDefinitionId;
			}
		}
		if (!ItemActor->GetItemDefinitionId().IsNone())
		{
			return ItemActor->GetItemDefinitionId();
		}
		if (!FallbackArchetypeId.IsNone())
		{
			return FallbackArchetypeId;
		}
		return ItemActor->GetItemArchetypeId();
	}
}

bool AGridLevelRuntimeActor::CanPartyPickupItemEntry(const FGridSpawnedItemRuntimeEntry& Entry, const AGrimrockPartyPawn* PartyPawn, bool bLogRejection) const
{
	if (!PartyPawn)
	{
		if (bLogRejection)
		{
			UE_LOG(LogTemp, Warning, TEXT("Grid item pickup rejected: missing party pawn."));
		}
		return false;
	}

	if (PartyPawn->LevelRuntimeActor != this)
	{
		if (bLogRejection)
		{
			UE_LOG(LogTemp, Warning, TEXT("Grid item pickup rejected: party runtime actor does not match item runtime actor. ItemCell=(%d,%d)."), Entry.Cell.X,
				Entry.Cell.Y);
		}
		return false;
	}

	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);

	// Free floor pickups use a physical horizontal reach instead of requiring
	// the party to stand on the exact same grid cell. The grid still limits
	// this to the current cell or one traversable cardinal neighbour.
	if (Entry.Edge == EGridEdge::None)
	{
		const AGridItemActor* ItemActor = Entry.ItemActor.Get();
		if (!IsValid(ItemActor))
		{
			if (bLogRejection)
			{
				UE_LOG(LogTemp, Warning, TEXT("Grid item pickup rejected: free pickup actor is missing. ItemCell=(%d,%d)."), Entry.Cell.X, Entry.Cell.Y);
			}
			return false;
		}

		const float PickupReach = FMath::Max(0.0f, WorldItemPickupReach);
		const float DistanceSquared = FVector::DistSquared2D(PartyPawn->GetActorLocation(), ItemActor->GetActorLocation());
		if (DistanceSquared > FMath::Square(PickupReach))
		{
			if (bLogRejection)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("Grid item pickup rejected: free pickup is out of reach. PartyCell=(%d,%d) ItemCell=(%d,%d) Distance=%.1f Reach=%.1f."),
					PartyCell.X, PartyCell.Y, Entry.Cell.X, Entry.Cell.Y, FMath::Sqrt(DistanceSquared), PickupReach);
			}
			return false;
		}

		if (Entry.Cell == PartyCell)
		{
			return true;
		}

		const EGridEdge PickupDirection = GetAdjacentPickupDirection(PartyCell, Entry.Cell);
		if (PickupDirection == EGridEdge::None)
		{
			if (bLogRejection)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("Grid item pickup rejected: free pickup is not in the party cell or a cardinal neighbour. PartyCell=(%d,%d) ItemCell=(%d,%d)."),
					PartyCell.X, PartyCell.Y, Entry.Cell.X, Entry.Cell.Y);
			}
			return false;
		}

		if (!CanMove(PartyCell.X, PartyCell.Y, PickupDirection))
		{
			if (bLogRejection)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("Grid item pickup rejected: free pickup is separated by a blocked grid edge. PartyCell=(%d,%d) ItemCell=(%d,%d) Direction=%s."),
					PartyCell.X, PartyCell.Y, Entry.Cell.X, Entry.Cell.Y, *GetWorldItemEdgeText(PickupDirection));
			}
			return false;
		}

		return true;
	}

	if (Entry.Cell == PartyCell)
	{
		const bool bFacesItemEdge = PartyPawn->Facing != EGridEdge::None && Entry.Edge == PartyPawn->Facing;
		if (!bFacesItemEdge && bLogRejection)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Grid item pickup rejected: item in party cell is not on the edge currently faced by the party. PartyCell=(%d,%d) Facing=%s ItemEdge=%s."),
				PartyCell.X, PartyCell.Y, *GetWorldItemEdgeText(PartyPawn->Facing), *GetWorldItemEdgeText(Entry.Edge));
		}
		return bFacesItemEdge;
	}

	if (PartyPawn->Facing == EGridEdge::None)
	{
		if (bLogRejection)
		{
			UE_LOG(LogTemp, Warning, TEXT("Grid item pickup rejected: party facing is None. PartyCell=(%d,%d) ItemCell=(%d,%d) ItemEdge=%s."), PartyCell.X,
				PartyCell.Y, Entry.Cell.X, Entry.Cell.Y, *GetWorldItemEdgeText(Entry.Edge));
		}
		return false;
	}

	int32 FrontCellX = PartyCell.X;
	int32 FrontCellY = PartyCell.Y;
	if (!TryGetNeighborCell(PartyCell.X, PartyCell.Y, PartyPawn->Facing, FrontCellX, FrontCellY) || Entry.Cell != FIntPoint(FrontCellX, FrontCellY))
	{
		if (bLogRejection)
		{
			UE_LOG(LogTemp, Warning,
				TEXT(
					"Grid item pickup rejected: item is not in the party cell or the cell directly ahead. PartyCell=(%d,%d) Facing=%s ItemCell=(%d,%d) ItemEdge=%s."),
				PartyCell.X, PartyCell.Y, *GetWorldItemEdgeText(PartyPawn->Facing), Entry.Cell.X, Entry.Cell.Y, *GetWorldItemEdgeText(Entry.Edge));
		}
		return false;
	}

	const EGridEdge RequiredItemEdge = GridDirectionUtils::GetOpposite(PartyPawn->Facing);
	if (Entry.Edge != RequiredItemEdge)
	{
		if (bLogRejection)
		{
			UE_LOG(LogTemp, Warning,
				TEXT(
					"Grid item pickup rejected: item in front cell is not on the edge facing the party. PartyCell=(%d,%d) Facing=%s ItemCell=(%d,%d) ItemEdge=%s RequiredEdge=%s."),
				PartyCell.X, PartyCell.Y, *GetWorldItemEdgeText(PartyPawn->Facing), Entry.Cell.X, Entry.Cell.Y, *GetWorldItemEdgeText(Entry.Edge),
				*GetWorldItemEdgeText(RequiredItemEdge));
		}
		return false;
	}

	return true;
}

bool AGridLevelRuntimeActor::CanPartyPickupItemActor(const AGridItemActor* ItemActor, const AGrimrockPartyPawn* PartyPawn) const
{
	if (!IsValid(ItemActor) || !PartyPawn)
	{
		return false;
	}

	for (const FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
	{
		if (Entry.ItemActor.Get() == ItemActor)
		{
			return CanPartyPickupItemEntry(Entry, PartyPawn, false);
		}
	}

	return false;
}

bool AGridLevelRuntimeActor::TryPickupItemAtCell(int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn)
{
	if (!PartyPawn)
	{
		return false;
	}

	const FIntPoint TargetCell(CellX, CellY);
	for (int32 EntryIndex = 0; EntryIndex < SpawnedItemEntries.Num(); ++EntryIndex)
	{
		FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
		if (Entry.Cell != TargetCell)
		{
			continue;
		}

		AGridItemActor* ItemActor = Entry.ItemActor.Get();
		if (!IsValid(ItemActor))
		{
			const FIntPoint RemovedCell = Entry.Cell;
			SpawnedItemEntries.RemoveAtSwap(EntryIndex);
			SpawnedItemActors.RemoveAllSwap(
				[](const TObjectPtr<AGridItemActor>& SpawnedItemActor)
				{
					return !IsValid(SpawnedItemActor.Get());
				});
			if (ActivationComponent)
			{
				ActivationComponent->RefreshPressurePlatesAtCell(RemovedCell.X, RemovedCell.Y);
			}
			return false;
		}

		if (!CanPartyPickupItemEntry(Entry, PartyPawn))
		{
			continue;
		}

		const FName ItemDefinitionId =
			ResolveWorldPickupItemDefinitionId(ItemActor, Entry.ItemDefinitionId.IsNone() ? Entry.ItemArchetypeId : Entry.ItemDefinitionId);
		if (ItemDefinitionId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("Item pickup failed at cell %d,%d: missing item definition id."), CellX, CellY);
			return false;
		}

		FGridItemInstance ItemInstance;
		ItemInstance.RuntimeObjectId = ItemActor->GetRuntimeObjectId();
		if (!ItemInstance.RuntimeObjectId.IsValid())
		{
			ItemInstance.RuntimeObjectId = FGuid::NewGuid();
		}
		ItemInstance.ItemDefinitionId = ItemDefinitionId;
		ItemInstance.Quantity = FMath::Max(1, Entry.Quantity);
		ItemInstance.Weight = 0.0f;
		ItemInstance.bLightsEnabled = ItemActor->AreItemLightsEnabled();
		ItemInstance.ReadableContentAsset = ItemActor->ReadableContentAsset;
		ItemInstance.ReadableContentId = ItemActor->ReadableContentId;
		ItemInstance.ReadTitleOverride = ItemActor->ReadTitleOverride;
		ItemInstance.ReadTextOverride = ItemActor->ReadTextOverride;
		ItemInstance.LastWorldTransform = ItemActor->GetActorTransform();

		if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory(ItemInstance))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"), *ItemDefinitionId.ToString(),
				*ItemInstance.RuntimeObjectId.ToString());
			ShowInteractionFeedback(FText::FromString(TEXT("Inventaire plein.")));
			return false;
		}

		if (IsValid(ItemActor))
		{
			ItemActor->OnRemovedFromWorld();
			ItemActor->Destroy();
		}
		SpawnedItemActors.RemoveAllSwap(
			[ItemActor](const TObjectPtr<AGridItemActor>& SpawnedItemActor)
			{
				return SpawnedItemActor.Get() == ItemActor;
			});

		const FIntPoint PickedCell = Entry.Cell;
		SpawnedItemEntries.RemoveAtSwap(EntryIndex);
		if (ActivationComponent)
		{
			ActivationComponent->RefreshPressurePlatesAtCell(PickedCell.X, PickedCell.Y);
		}

		UE_LOG(LogTemp, Log, TEXT("Picked up item %s from cell %d,%d."), *ItemDefinitionId.ToString(), CellX, CellY);
		return true;
	}

	return false;
}

bool AGridLevelRuntimeActor::TryPickupItemActor(AGridItemActor* ItemActor, AGrimrockPartyPawn* PartyPawn)
{
	if (!IsValid(ItemActor) || !PartyPawn)
	{
		return false;
	}

	for (int32 EntryIndex = 0; EntryIndex < SpawnedItemEntries.Num(); ++EntryIndex)
	{
		FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
		if (Entry.ItemActor.Get() != ItemActor)
		{
			continue;
		}

		if (!CanPartyPickupItemEntry(Entry, PartyPawn))
		{
			return false;
		}

		const FName ItemDefinitionId =
			ResolveWorldPickupItemDefinitionId(ItemActor, Entry.ItemDefinitionId.IsNone() ? Entry.ItemArchetypeId : Entry.ItemDefinitionId);
		if (ItemDefinitionId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("Item pickup failed for actor %s: missing item definition id."), *ItemActor->GetName());
			return false;
		}

		FGridItemInstance ItemInstance;
		ItemInstance.RuntimeObjectId = ItemActor->GetRuntimeObjectId();
		if (!ItemInstance.RuntimeObjectId.IsValid())
		{
			ItemInstance.RuntimeObjectId = FGuid::NewGuid();
		}
		ItemInstance.ItemDefinitionId = ItemDefinitionId;
		ItemInstance.Quantity = FMath::Max(1, Entry.Quantity);
		ItemInstance.Weight = 0.0f;
		ItemInstance.bLightsEnabled = ItemActor->AreItemLightsEnabled();
		ItemInstance.ReadableContentAsset = ItemActor->ReadableContentAsset;
		ItemInstance.ReadableContentId = ItemActor->ReadableContentId;
		ItemInstance.ReadTitleOverride = ItemActor->ReadTitleOverride;
		ItemInstance.ReadTextOverride = ItemActor->ReadTextOverride;
		ItemInstance.LastWorldTransform = ItemActor->GetActorTransform();

		if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory(ItemInstance))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"), *ItemDefinitionId.ToString(),
				*ItemInstance.RuntimeObjectId.ToString());
			ShowInteractionFeedback(FText::FromString(TEXT("Inventaire plein.")));
			return false;
		}

		ItemActor->OnRemovedFromWorld();
		ItemActor->Destroy();

		SpawnedItemActors.RemoveAllSwap(
			[ItemActor](const TObjectPtr<AGridItemActor>& SpawnedItemActor)
			{
				return SpawnedItemActor.Get() == ItemActor;
			});

		const FIntPoint PickedCell = Entry.Cell;
		SpawnedItemEntries.RemoveAtSwap(EntryIndex);
		if (ActivationComponent)
		{
			ActivationComponent->RefreshPressurePlatesAtCell(PickedCell.X, PickedCell.Y);
		}

		UE_LOG(LogTemp, Log, TEXT("Picked up item %s from clicked actor at cell %d,%d."), *ItemDefinitionId.ToString(), PickedCell.X, PickedCell.Y);
		return true;
	}

	return false;
}

bool AGridLevelRuntimeActor::TryRouteWorldItemThroughOpenPit(
	const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinitionAsset, int32 CellX, int32 CellY, const FVector& LocalOffset)
{
	FGridObjectTransitionParams Transition;
	if (!DungeonAsset || !ItemInstance.IsValid() || !ItemDefinitionAsset || !FindOpenPitAtCell(CellX, CellY, Transition))
	{
		return false;
	}

	if (Transition.TargetLevelId.IsNone() || Transition.TargetLevelId == CurrentDungeonLevelId)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridPit ItemTransfer rejected Item=%s RuntimeId=%s Source=(%d,%d) Reason=InvalidTargetLevel Target=%s"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), CellX, CellY, *Transition.TargetLevelId.ToString());
		return false;
	}

	const FGridDungeonLevelEntry* TargetEntry = DungeonAsset->FindLevelEntry(Transition.TargetLevelId);
	UGridLevelAsset* TargetLevelAsset = TargetEntry && TargetEntry->bEnabled ? TargetEntry->LevelAsset.Get() : nullptr;
	if (!TargetLevelAsset || !TargetLevelAsset->IsValidCoord(Transition.TargetCellX, Transition.TargetCellY))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridPit ItemTransfer rejected Item=%s RuntimeId=%s Source=(%d,%d) Reason=InvalidTargetCell TargetLevel=%s Target=(%d,%d)"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), CellX, CellY, *Transition.TargetLevelId.ToString(),
			Transition.TargetCellX, Transition.TargetCellY);
		return false;
	}

	const FGridLevelCellData& TargetCell = TargetLevelAsset->GetCell(Transition.TargetCellX, Transition.TargetCellY);
	if (TargetCell.CellType == EGridCellType::Empty || TargetCell.bBlocksOccupancy)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridPit ItemTransfer rejected Item=%s RuntimeId=%s Reason=TargetNotWalkable TargetLevel=%s Target=(%d,%d)"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), *Transition.TargetLevelId.ToString(),
			Transition.TargetCellX, Transition.TargetCellY);
		return false;
	}

	const bool bDestinationContainsOpenPit = TargetLevelAsset->Objects.ContainsByPredicate(
		[this, &Transition](const FGridLevelObjectData& Candidate)
		{
			return Candidate.Type == EGridLevelObjectType::Pit && Candidate.CellX == Transition.TargetCellX &&
				Candidate.CellY == Transition.TargetCellY && Candidate.bInitiallyEnabled &&
				IsPitOpenForLevel(Transition.TargetLevelId, Candidate);
		});
	if (bDestinationContainsOpenPit)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GridPit ItemTransfer rejected Item=%s RuntimeId=%s Reason=ChainedPitNotSupported TargetLevel=%s Target=(%d,%d)"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), *Transition.TargetLevelId.ToString(),
			Transition.TargetCellX, Transition.TargetCellY);
		return false;
	}

	const float TargetCellSize = FMath::Max(1.0f, TargetLevelAsset->CellSize);
	const float MaxOffset = TargetCellSize * 0.35f;
	const FVector ClampedOffset(
		FMath::Clamp(LocalOffset.X, -MaxOffset, MaxOffset),
		FMath::Clamp(LocalOffset.Y, -MaxOffset, MaxOffset),
		0.0f);
	const FVector TargetWorldLocation =
		GetActorLocation() + GridOrigin +
		FVector((Transition.TargetCellX + 0.5f) * TargetCellSize, (Transition.TargetCellY + 0.5f) * TargetCellSize, 12.0f) + ClampedOffset;

	FGridRuntimeItemState ItemState;
	ItemState.ObjectId = ItemInstance.RuntimeObjectId;
	ItemState.ArchetypeId = ItemInstance.ItemDefinitionId;
	ItemState.ItemDefinitionId = ItemInstance.ItemDefinitionId;
	ItemState.Quantity = FMath::Max(1, ItemInstance.Quantity);
	ItemState.CellX = Transition.TargetCellX;
	ItemState.CellY = Transition.TargetCellY;
	ItemState.Edge = EGridEdge::None;
	ItemState.Transform = FTransform(FRotator::ZeroRotator, TargetWorldLocation, FVector::OneVector);
	ItemState.bIsSimulatingPhysics = false;
	ItemState.bIsContainedInReceptacle = false;
	ItemState.bLightsEnabled = ItemInstance.bLightsEnabled;
	ItemState.ReadableContentAsset = ItemInstance.ReadableContentAsset;
	ItemState.ReadableContentId = ItemInstance.ReadableContentId;
	ItemState.ReadTitleOverride = ItemInstance.ReadTitleOverride;
	ItemState.ReadTextOverride = ItemInstance.ReadTextOverride;

	FGridPendingWorldItemState PendingState;
	PendingState.ItemState = ItemState;
	PendingState.ItemDefinitionAsset = ItemDefinitionAsset;

	FGridLevelRuntimeState& TargetState = DungeonRuntimeState.LevelStates.FindOrAdd(Transition.TargetLevelId);
	TargetState.LevelId = Transition.TargetLevelId;
	TargetState.PendingInboundItems.Add(ItemState.ObjectId, PendingState);

	UE_LOG(LogTemp, Log,
		TEXT("GridPit ItemTransfer queued Item=%s RuntimeId=%s SourceLevel=%s Source=(%d,%d) TargetLevel=%s Target=(%d,%d) Offset=%s"),
		*ItemState.ItemDefinitionId.ToString(), *ItemState.ObjectId.ToString(), *CurrentDungeonLevelId.ToString(), CellX, CellY,
		*Transition.TargetLevelId.ToString(), Transition.TargetCellX, Transition.TargetCellY, *ClampedOffset.ToCompactString());
	return true;
}

int32 AGridLevelRuntimeActor::ApplyPendingInboundItemsForCurrentLevel()
{
	const FName RuntimeLevelId = CurrentDungeonLevelId.IsNone() ? FName(TEXT("SingleLevel")) : CurrentDungeonLevelId;
	const FGridLevelRuntimeState* InitialState = DungeonRuntimeState.LevelStates.Find(RuntimeLevelId);
	if (!InitialState || InitialState->PendingInboundItems.IsEmpty() || !LevelAsset)
	{
		return 0;
	}

	TArray<FGuid> PendingIds;
	InitialState->PendingInboundItems.GetKeys(PendingIds);
	PendingIds.Sort(
		[](const FGuid& Left, const FGuid& Right)
		{
			return Left.ToString(EGuidFormats::Digits) < Right.ToString(EGuidFormats::Digits);
		});

	int32 AppliedCount = 0;
	for (const FGuid& RuntimeObjectId : PendingIds)
	{
		const FGridLevelRuntimeState* ReadState = DungeonRuntimeState.LevelStates.Find(RuntimeLevelId);
		const FGridPendingWorldItemState* Pending = ReadState ? ReadState->PendingInboundItems.Find(RuntimeObjectId) : nullptr;
		if (!Pending)
		{
			continue;
		}

		const FGridPendingWorldItemState PendingCopy = *Pending;
		const FGridRuntimeItemState& ItemState = PendingCopy.ItemState;
		if (!LevelAsset->IsValidCoord(ItemState.CellX, ItemState.CellY))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridPit PendingItem apply skipped RuntimeId=%s Reason=InvalidCell Cell=(%d,%d)"),
				*RuntimeObjectId.ToString(), ItemState.CellX, ItemState.CellY);
			continue;
		}

		FGridItemInstance ItemInstance;
		ItemInstance.RuntimeObjectId = RuntimeObjectId;
		ItemInstance.ItemDefinitionId = ItemState.ItemDefinitionId;
		ItemInstance.Quantity = FMath::Max(1, ItemState.Quantity);
		ItemInstance.bLightsEnabled = ItemState.bLightsEnabled;
		ItemInstance.ReadableContentAsset = ItemState.ReadableContentAsset;
		ItemInstance.ReadableContentId = ItemState.ReadableContentId;
		ItemInstance.ReadTitleOverride = ItemState.ReadTitleOverride;
		ItemInstance.ReadTextOverride = ItemState.ReadTextOverride;
		ItemInstance.LastWorldTransform = ItemState.Transform;

		const FVector CellCenter = GetCellCenterWorld(ItemState.CellX, ItemState.CellY, 12.0f);
		FVector LocalOffset = ItemState.Transform.GetLocation() - CellCenter;
		LocalOffset.Z = 0.0f;

		UGridItemDefinitionAsset* Definition = PendingCopy.ItemDefinitionAsset.Get();
		if (!Definition)
		{
			Definition = ResolveRuntimeItemDefinition(ItemState.ItemDefinitionId);
		}

		if (!TryDropItemInstanceAtCell(ItemInstance, Definition, ItemState.CellX, ItemState.CellY, EGridEdge::None, LocalOffset))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridPit PendingItem apply failed RuntimeId=%s Item=%s Level=%s Cell=(%d,%d)"),
				*RuntimeObjectId.ToString(), *ItemState.ItemDefinitionId.ToString(), *RuntimeLevelId.ToString(), ItemState.CellX, ItemState.CellY);
			continue;
		}

		if (FGridLevelRuntimeState* MutableState = DungeonRuntimeState.LevelStates.Find(RuntimeLevelId))
		{
			MutableState->PendingInboundItems.Remove(RuntimeObjectId);
		}
		++AppliedCount;
	}

	if (AppliedCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("GridPit PendingItem apply Level=%s Applied=%d"), *RuntimeLevelId.ToString(), AppliedCount);
	}
	return AppliedCount;
}

int32 AGridLevelRuntimeActor::DropWorldItemsThroughOpenPitAtCell(int32 CellX, int32 CellY)
{
	if (!LevelAsset)
	{
		return 0;
	}

	FGridObjectTransitionParams Transition;
	if (!FindOpenPitAtCell(CellX, CellY, Transition))
	{
		return 0;
	}

	const FIntPoint PitCell(CellX, CellY);
	int32 DroppedCount = 0;
	for (int32 EntryIndex = SpawnedItemEntries.Num() - 1; EntryIndex >= 0; --EntryIndex)
	{
		if (!SpawnedItemEntries.IsValidIndex(EntryIndex))
		{
			continue;
		}

		FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
		AGridItemActor* ItemActor = Entry.ItemActor.Get();
		if (Entry.Cell != PitCell || Entry.Edge != EGridEdge::None || !IsValid(ItemActor))
		{
			continue;
		}

		UGridItemDefinitionAsset* Definition = Entry.ItemDefinitionAsset.Get();
		if (!Definition)
		{
			const FName DefinitionId = !Entry.ItemDefinitionId.IsNone() ? Entry.ItemDefinitionId : Entry.ItemArchetypeId;
			Definition = ResolveRuntimeItemDefinition(DefinitionId);
		}
		if (!Definition)
		{
			continue;
		}

		FGridItemInstance ItemInstance;
		ItemInstance.RuntimeObjectId = Entry.ObjectId.IsValid() ? Entry.ObjectId : ItemActor->GetRuntimeObjectId();
		if (!ItemInstance.RuntimeObjectId.IsValid())
		{
			ItemInstance.RuntimeObjectId = FGuid::NewGuid();
		}
		ItemInstance.ItemDefinitionId = Definition->ItemDefinitionId;
		ItemInstance.Quantity = FMath::Max(1, Entry.Quantity);
		ItemInstance.bLightsEnabled = ItemActor->AreItemLightsEnabled();
		ItemInstance.ReadableContentAsset = ItemActor->ReadableContentAsset;
		ItemInstance.ReadableContentId = ItemActor->ReadableContentId;
		ItemInstance.ReadTitleOverride = ItemActor->ReadTitleOverride;
		ItemInstance.ReadTextOverride = ItemActor->ReadTextOverride;
		ItemInstance.LastWorldTransform = ItemActor->GetActorTransform();

		FVector LocalOffset = ItemActor->GetActorLocation() - GetCellCenterWorld(CellX, CellY, 12.0f);
		LocalOffset.Z = 0.0f;
		if (!TryRouteWorldItemThroughOpenPit(ItemInstance, Definition, CellX, CellY, LocalOffset))
		{
			continue;
		}

		ItemActor->OnRemovedFromWorld();
		ItemActor->Destroy();
		SpawnedItemActors.RemoveAllSwap(
			[ItemActor](const TObjectPtr<AGridItemActor>& Candidate)
			{
				return Candidate.Get() == ItemActor;
			});
		SpawnedItemEntries.RemoveAtSwap(EntryIndex);
		++DroppedCount;
	}

	if (DroppedCount > 0 && ActivationComponent)
	{
		ActivationComponent->RefreshPressurePlatesAtCell(CellX, CellY);
	}

	UE_LOG(LogTemp, Log, TEXT("GridPit opened under WorldItems Cell=(%d,%d) Dropped=%d"), CellX, CellY, DroppedCount);
	return DroppedCount;
}

bool AGridLevelRuntimeActor::TryDropItemInstanceAtCell(
	const FGridItemInstance& ItemInstance, int32 CellX, int32 CellY, EGridEdge Edge, const FVector& LocalOffset)
{
	return TryDropItemInstanceAtCell(ItemInstance, nullptr, CellX, CellY, Edge, LocalOffset);
}

bool AGridLevelRuntimeActor::TryDropItemInstanceAtCell(
	const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinitionAsset, int32 CellX, int32 CellY, EGridEdge Edge, const FVector& LocalOffset)
{
	if (!LevelAsset || !ItemInstance.IsValid() || !LevelAsset->IsValidCoord(CellX, CellY))
	{
		return false;
	}

	const FGridLevelCellData& Cell = LevelAsset->GetCell(CellX, CellY);
	if (Cell.CellType == EGridCellType::Empty || Cell.bBlocksOccupancy)
	{
		return false;
	}

	UGridItemDefinitionAsset* ItemDefinition = IsValid(ItemDefinitionAsset) ? ItemDefinitionAsset : ResolveRuntimeItemDefinition(ItemInstance.ItemDefinitionId);
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory WorldDrop Failed Item=%s Reason=DefinitionNotResolved"), *ItemInstance.ItemDefinitionId.ToString());
		return false;
	}

	if (Edge == EGridEdge::None)
	{
		FGridObjectTransitionParams PitTransition;
		if (FindOpenPitAtCell(CellX, CellY, PitTransition))
		{
			return TryRouteWorldItemThroughOpenPit(ItemInstance, ItemDefinition, CellX, CellY, LocalOffset);
		}
	}

	FTransform DropTransform;
	if (Edge == EGridEdge::None)
	{
		DropTransform = FTransform(FRotator::ZeroRotator, GetCellCenterWorld(CellX, CellY, 12.f) + LocalOffset, FVector::OneVector);
	}
	else
	{
		FGridLevelObjectData PlacementData;
		PlacementData.Type = EGridLevelObjectType::Item;
		PlacementData.CellX = CellX;
		PlacementData.CellY = CellY;
		PlacementData.Edge = Edge;
		if (!GetObjectPlacementTransform(PlacementData, DropTransform))
		{
			return false;
		}
		DropTransform.AddToTranslation(LocalOffset);
	}

	if (!IsSafeRuntimeRenderTransform(DropTransform))
	{
		return false;
	}

	AGridItemActor* ItemActor = SpawnItemActorForDefinition(ItemDefinition, ItemInstance.ItemDefinitionId, this, nullptr);
	if (!ItemActor)
	{
		return false;
	}

	const FGuid RuntimeObjectId = ItemInstance.RuntimeObjectId.IsValid() ? ItemInstance.RuntimeObjectId : FGuid::NewGuid();
	ItemActor->SetActorTransform(DropTransform);
	ItemActor->InitializeFromItemDefinition(ItemDefinition, RuntimeObjectId);
	ItemActor->SetRuntimeObjectId(RuntimeObjectId);
	ItemActor->SetRuntimeCell(CellX, CellY);
	ItemActor->ConfigureAsWorldPickup();
	ItemActor->OnPlacedInWorld();
	ItemActor->SetItemLightsEnabled(ItemInstance.bLightsEnabled);
	ItemActor->InitializeReadableContent(
		ItemInstance.ReadableContentAsset, ItemInstance.ReadableContentId, ItemInstance.ReadTitleOverride, ItemInstance.ReadTextOverride);

	FGridSpawnedItemRuntimeEntry Entry;
	Entry.Cell = FIntPoint(CellX, CellY);
	Entry.Edge = Edge;
	Entry.ItemActor = ItemActor;
	Entry.ObjectId = RuntimeObjectId;
	Entry.ItemArchetypeId = ItemInstance.ItemDefinitionId;
	Entry.ItemDefinitionAsset = ItemDefinition;
	Entry.ItemDefinitionId = ItemInstance.ItemDefinitionId;
	Entry.Quantity = FMath::Max(1, ItemInstance.Quantity);

	SpawnedItemActors.Add(ItemActor);
	SpawnedItemEntries.Add(Entry);
	if (ActivationComponent)
	{
		ActivationComponent->RefreshPressurePlatesAtCell(CellX, CellY);
	}

	UE_LOG(LogTemp, Log, TEXT("GridInventory WorldDrop Item=%s RuntimeId=%s Quantity=%d Cell=(%d,%d) Edge=%d Result=true"), *Entry.ItemDefinitionId.ToString(),
		*Entry.ObjectId.ToString(), Entry.Quantity, CellX, CellY, static_cast<int32>(Edge));
	return true;
}

bool AGridLevelRuntimeActor::TrySpawnThrownItemProjectile(
	const FGridItemInstance& ItemInstance, const FVector& StartWorldLocation, const FVector& LaunchVelocity, int32 SourceCellX, int32 SourceCellY)
{
	UGridItemDefinitionAsset* ItemDefinition = ResolveRuntimeItemDefinition(ItemInstance.ItemDefinitionId);
	return SpawnThrownItemProjectile(ItemInstance, ItemDefinition, StartWorldLocation, LaunchVelocity, SourceCellX, SourceCellY) != nullptr;
}

AGridThrownItemActor* AGridLevelRuntimeActor::SpawnThrownItemProjectile(const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinition,
	const FVector& StartWorldLocation, const FVector& LaunchVelocity, int32 SourceCellX, int32 SourceCellY)
{
	if (!ItemInstance.IsValid() || !ItemDefinition || ItemDefinition->ItemDefinitionId != ItemInstance.ItemDefinitionId ||
		!ItemDefinition->IsPhysicallyThrowable() || LaunchVelocity.IsNearlyZero() || !IsWalkableCell(SourceCellX, SourceCellY))
	{
		return nullptr;
	}

	const FTransform SpawnTransform(LaunchVelocity.Rotation(), StartWorldLocation, FVector::OneVector);
	if (!IsSafeRuntimeRenderTransform(SpawnTransform))
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGridThrownItemActor* ThrownActor = World->SpawnActor<AGridThrownItemActor>(AGridThrownItemActor::StaticClass(), SpawnTransform, Params);
	if (!ThrownActor)
	{
		return nullptr;
	}

	ThrownActor->InitializeThrownItem(this, ItemInstance, ItemDefinition, LaunchVelocity, SourceCellX, SourceCellY);

	UE_LOG(LogTemp, Log, TEXT("GridInventory Throw Spawn Item=%s RuntimeId=%s SourceCell=(%d,%d) Speed=%.2f Result=true"),
		*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), SourceCellX, SourceCellY, LaunchVelocity.Size());
	return ThrownActor;
}

bool AGridLevelRuntimeActor::TryResolveWorldCellFromImpactPoint(const FVector& WorldPoint, int32& OutCellX, int32& OutCellY, FVector& OutLocalOffset) const
{
	OutCellX = INDEX_NONE;
	OutCellY = INDEX_NONE;
	OutLocalOffset = FVector::ZeroVector;
	if (!LevelAsset || LevelAsset->CellSize <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector GridLocalPoint = WorldPoint - GetActorLocation() - GridOrigin;
	OutCellX = FMath::FloorToInt(GridLocalPoint.X / LevelAsset->CellSize);
	OutCellY = FMath::FloorToInt(GridLocalPoint.Y / LevelAsset->CellSize);
	if (!IsWalkableCell(OutCellX, OutCellY))
	{
		return false;
	}

	const FVector CellCenter = GetCellCenterWorld(OutCellX, OutCellY, 12.0f);
	const FVector RawOffset = WorldPoint - CellCenter;
	const float MaxOffset = LevelAsset->CellSize * 0.35f;
	OutLocalOffset = FVector(FMath::Clamp(RawOffset.X, -MaxOffset, MaxOffset), FMath::Clamp(RawOffset.Y, -MaxOffset, MaxOffset), 0.0f);
	return true;
}

float AGridLevelRuntimeActor::GetWorldItemWeightAtCell(int32 CellX, int32 CellY, bool bIncludeEdgeItems) const
{
	const FIntPoint TargetCell(CellX, CellY);
	float TotalWeight = 0.0f;

	for (const FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
	{
		if (Entry.Cell != TargetCell || (!bIncludeEdgeItems && Entry.Edge != EGridEdge::None) || !IsValid(Entry.ItemActor.Get()))
		{
			continue;
		}

		UGridItemDefinitionAsset* ItemDefinition = Entry.ItemDefinitionAsset.Get();
		if (!ItemDefinition)
		{
			const FName DefinitionId = !Entry.ItemDefinitionId.IsNone() ? Entry.ItemDefinitionId : Entry.ItemArchetypeId;
			ItemDefinition = ResolveRuntimeItemDefinition(DefinitionId);
		}
		if (!ItemDefinition)
		{
			continue;
		}

		const int32 Quantity = FMath::Max(1, Entry.Quantity);
		const float Contribution = ItemDefinition->Weight * Quantity;
		TotalWeight += Contribution;
		UE_LOG(LogTemp, Verbose, TEXT("GridPressurePlate WeightScan Cell=(%d,%d) Item=%s Quantity=%d UnitWeight=%.2f TotalContribution=%.2f"), CellX, CellY,
			*ItemDefinition->ItemDefinitionId.ToString(), Quantity, ItemDefinition->Weight, Contribution);
	}

	return TotalWeight;
}
