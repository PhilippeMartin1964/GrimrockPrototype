#include "Runtime/GridLevelRuntimeActor.h"

#include "Core/GridObjectArchetypeAsset.h"
#include "EngineUtils.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
	FName ResolvePickupItemDefinitionId(const AGridItemActor* ItemActor, FName FallbackArchetypeId)
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

	const FGridLevelObjectData* FindLevelObjectDataById(const UGridLevelAsset* LevelAsset, FGuid ObjectId)
	{
		if (!LevelAsset || !ObjectId.IsValid())
		{
			return nullptr;
		}

		for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
		{
			if (ObjectData.ObjectId == ObjectId)
			{
				return &ObjectData;
			}
		}

		return nullptr;
	}

	int32 CountRemovedRuntimeObjects(const FGridLevelRuntimeState* RuntimeState)
	{
		if (!RuntimeState)
		{
			return 0;
		}

		int32 RemovedCount = 0;
		for (const TPair<FGuid, FGridRuntimeObjectPresenceState>& Pair : RuntimeState->ObjectPresence)
		{
			if (Pair.Value.bRemovedFromInitialPlacement)
			{
				++RemovedCount;
			}
		}
		return RemovedCount;
	}

	void GetWorldMonsters(const UWorld* World, TArray<AGridMonsterActor*>& OutMonsters)
	{
		OutMonsters.Reset();
		if (!World)
		{
			return;
		}

		for (TActorIterator<AGridMonsterActor> It(const_cast<UWorld*>(World)); It; ++It)
		{
			if (IsValid(*It))
			{
				OutMonsters.Add(*It);
			}
		}

		OutMonsters.Sort(
			[](const AGridMonsterActor& Left, const AGridMonsterActor& Right)
			{
				const FGuid LeftId = Left.ResolvePersistenceId();
				const FGuid RightId = Right.ResolvePersistenceId();
				if (LeftId.IsValid() != RightId.IsValid())
				{
					return LeftId.IsValid();
				}
				if (LeftId != RightId)
				{
					return LeftId.ToString(EGuidFormats::Digits) < RightId.ToString(EGuidFormats::Digits);
				}
				return Left.GetPathName() < Right.GetPathName();
			});
	}
}

bool AGridLevelRuntimeActor::CaptureCurrentLevelRuntimeState()
{
	if (!LevelAsset)
	{
		return false;
	}

	FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
	if (!State)
	{
		return false;
	}

	State->Doors.Reset();
	State->InteractiveObjects.Reset();
	State->ObjectPresence.Reset();
	State->Items.Reset();
	State->Receptacles.Reset();
	State->Monsters.Reset();
	State->bHasBeenVisited = true;

	for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
	{
		if (!ObjectData.ObjectId.IsValid())
		{
			continue;
		}

		if (ObjectData.Type == EGridLevelObjectType::Door && DoorSystemComponent)
		{
			bool bDoorOpen = false;
			bool bDoorMoving = false;
			bool bDoorBlocked = true;
			if (DoorSystemComponent->GetDoorState(ObjectData.ObjectId, bDoorOpen, bDoorMoving, bDoorBlocked))
			{
				FGridRuntimeDoorState DoorState;
				DoorState.ObjectId = ObjectData.ObjectId;
				DoorState.bIsOpen = bDoorOpen;
				DoorState.bBlocksMovement = bDoorBlocked;
				State->Doors.Add(DoorState.ObjectId, DoorState);
			}
		}
	}

	TSet<FGuid> ActiveObjectIds;
	if (ActivationComponent)
	{
		ActiveObjectIds = ActivationComponent->GetActiveObjectIds();
		for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
		{
			if (!ObjectData.ObjectId.IsValid())
			{
				continue;
			}

			const bool bIsInteractiveObject = ObjectData.Type == EGridLevelObjectType::Button || ObjectData.Type == EGridLevelObjectType::Lever ||
				ObjectData.Type == EGridLevelObjectType::PressurePlate || ObjectData.Type == EGridLevelObjectType::Receptacle ||
				ObjectData.Type == EGridLevelObjectType::Trigger;
			if (!bIsInteractiveObject)
			{
				continue;
			}

			const bool bIsActive = ActiveObjectIds.Contains(ObjectData.ObjectId);
			FGridRuntimeInteractiveState InteractiveState;
			InteractiveState.ObjectId = ObjectData.ObjectId;
			InteractiveState.bIsActivated = bIsActive;
			InteractiveState.bIsPressed = bIsActive;
			InteractiveState.bIsOn = bIsActive;
			State->InteractiveObjects.Add(ObjectData.ObjectId, InteractiveState);
		}
	}

	TSet<FGuid> ExistingPlacedItemObjectIds;
	for (const FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
	{
		AGridItemActor* ItemActor = Entry.ItemActor.Get();
		if (!IsValid(ItemActor))
		{
			continue;
		}
		ExistingPlacedItemObjectIds.Add(Entry.ObjectId);
		FGridRuntimeItemState ItemState;
		ItemState.ObjectId = Entry.ObjectId;
		ItemState.ArchetypeId = Entry.ItemArchetypeId;
		ItemState.ItemDefinitionId =
			!Entry.ItemDefinitionId.IsNone() ? Entry.ItemDefinitionId : ResolvePickupItemDefinitionId(ItemActor, Entry.ItemArchetypeId);
		ItemState.Quantity = FMath::Max(1, Entry.Quantity);
		ItemState.CellX = Entry.Cell.X;
		ItemState.CellY = Entry.Cell.Y;
		ItemState.Edge = Entry.Edge;

		if (ItemState.ItemDefinitionId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("GridRuntimeState Capture skipped item: ObjectId=%s Actor=%s no ItemDefinitionId or legacy ArchetypeId resolved."),
				*Entry.ObjectId.ToString(), *GetNameSafe(ItemActor));
			continue;
		}
		ItemState.Transform = ItemActor->GetActorTransform();
		ItemState.bIsSimulatingPhysics = ItemActor->MeshComponent ? ItemActor->MeshComponent->IsSimulatingPhysics() : false;
		ItemState.bIsContainedInReceptacle = false;
		ItemState.bLightsEnabled = ItemActor->AreItemLightsEnabled();
		ItemState.ReadableContentAsset = ItemActor->ReadableContentAsset;
		ItemState.ReadableContentId = ItemActor->ReadableContentId;
		ItemState.ReadTitleOverride = ItemActor->ReadTitleOverride;
		ItemState.ReadTextOverride = ItemActor->ReadTextOverride;
		State->Items.Add(Entry.ObjectId, ItemState);
	}
	for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
	{
		if (ObjectData.Type != EGridLevelObjectType::Item || !ObjectData.ObjectId.IsValid())
		{
			continue;
		}

		FGridRuntimeObjectPresenceState PresenceState;
		PresenceState.ObjectId = ObjectData.ObjectId;
		PresenceState.bRemovedFromInitialPlacement = !ExistingPlacedItemObjectIds.Contains(ObjectData.ObjectId);
		State->ObjectPresence.Add(ObjectData.ObjectId, PresenceState);
	}

	for (const TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
	{
		AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor>(Pair.Value.Get());
		if (!IsValid(ReceptacleActor))
		{
			continue;
		}
		FGridRuntimeReceptacleState ReceptacleState;
		ReceptacleActor->CaptureRuntimeReceptacleState(ReceptacleState);
		State->Receptacles.Add(Pair.Key, ReceptacleState);
	}

	TArray<AGridMonsterActor*> Monsters;
	GetWorldMonsters(GetWorld(), Monsters);
	TMap<FGuid, int32> PersistenceIdCounts;
	TSet<FGuid> CapturedMonsterSpawnIds;
	for (AGridMonsterActor* Monster : Monsters)
	{
		if (Monster->ResolveRuntimeDungeonLevelId(State->LevelId) != State->LevelId)
		{
			continue;
		}

		const FGuid PersistenceId = Monster->ResolvePersistenceId();
		if (!PersistenceId.IsValid())
		{
			UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterState] Capture skipped Level=%s Monster=%s Reason=InvalidPersistenceId"),
				*State->LevelId.ToString(), *GetNameSafe(Monster));
			continue;
		}
		++PersistenceIdCounts.FindOrAdd(PersistenceId);
	}

	for (AGridMonsterActor* Monster : Monsters)
	{
		if (Monster->ResolveRuntimeDungeonLevelId(State->LevelId) != State->LevelId)
		{
			continue;
		}

		const FGuid PersistenceId = Monster->ResolvePersistenceId();
		const int32 DuplicateCount = PersistenceIdCounts.FindRef(PersistenceId);
		if (DuplicateCount > 1)
		{
			UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterState] DuplicatePersistenceId Level=%s Monster=%s PersistenceId=%s Count=%d"),
				*State->LevelId.ToString(), *GetNameSafe(Monster), *PersistenceId.ToString(), DuplicateCount);
			continue;
		}

		FGridRuntimeMonsterState MonsterState;
		if (Monster->CaptureRuntimeMonsterState(MonsterState, State->LevelId))
		{
			State->Monsters.Add(MonsterState.PersistenceId, MonsterState);
			if (MonsterState.SpawnObjectId.IsValid() && LevelAsset->FindMonsterSpawnById(MonsterState.SpawnObjectId))
			{
				FGridRuntimeMonsterPlacementState& PlacementState = State->MonsterPlacements.FindOrAdd(MonsterState.SpawnObjectId);
				PlacementState.SpawnId = MonsterState.SpawnObjectId;
				PlacementState.bIsSpawned = true;
				PlacementState.bHasMonsterState = true;
				PlacementState.MonsterState = MonsterState;
				CapturedMonsterSpawnIds.Add(MonsterState.SpawnObjectId);
			}
		}
	}

	for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
	{
		if (ObjectData.Type != EGridLevelObjectType::MonsterSpawn || !ObjectData.ObjectId.IsValid() || CapturedMonsterSpawnIds.Contains(ObjectData.ObjectId))
		{
			continue;
		}

		FGridRuntimeMonsterPlacementState* PlacementState = State->MonsterPlacements.Find(ObjectData.ObjectId);
		if (!PlacementState)
		{
			FGridRuntimeMonsterPlacementState InitialPlacementState;
			InitialPlacementState.SpawnId = ObjectData.ObjectId;
			InitialPlacementState.bIsSpawned = ObjectData.bInitiallyEnabled;
			State->MonsterPlacements.Add(ObjectData.ObjectId, InitialPlacementState);
		}
	}

	int32 DeadMonsterCount = 0;
	for (const TPair<FGuid, FGridRuntimeMonsterState>& Pair : State->Monsters)
	{
		DeadMonsterCount += Pair.Value.bIsDead ? 1 : 0;
	}

	UE_LOG(LogTemp, Log,
		TEXT(
			"GridRuntimeState Capture Level=%s Doors=%d RemovedObjects=%d Items=%d Receptacles=%d Interactives=%d Monsters=%d MonsterPlacements=%d DeadMonsters=%d"),
		*State->LevelId.ToString(), State->Doors.Num(), CountRemovedRuntimeObjects(State), State->Items.Num(), State->Receptacles.Num(),
		State->InteractiveObjects.Num(), State->Monsters.Num(), State->MonsterPlacements.Num(), DeadMonsterCount);

	return true;
}

bool AGridLevelRuntimeActor::ApplyCurrentLevelRuntimeState()
{
	if (!LevelAsset)
	{
		return false;
	}

	const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel();
	if (!State || !State->bHasBeenVisited)
	{
		ApplyInitialMonsterStateForCurrentLevel();
		return false;
	}

	AbortActiveCombatAndMonsterActions();

	TArray<AGridMonsterActor*> WorldMonsters;
	GetWorldMonsters(GetWorld(), WorldMonsters);
	for (AGridMonsterActor* Monster : WorldMonsters)
	{
		SetMonsterRuntimeLevelActive(Monster, false);
	}

	if (UGridMonsterOccupancySubsystem* Occupancy = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr)
	{
		Occupancy->ResetRegistry();
	}

	if (DoorSystemComponent)
	{
		for (const TPair<FGuid, FGridRuntimeDoorState>& Pair : State->Doors)
		{
			DoorSystemComponent->ApplyDoorState(Pair.Key, Pair.Value.bIsOpen, Pair.Value.bBlocksMovement);
		}
	}

	if (ActivationComponent)
	{
		TSet<FGuid> ActiveObjectIds;
		for (const TPair<FGuid, FGridRuntimeInteractiveState>& Pair : State->InteractiveObjects)
		{
			if (Pair.Value.bIsActivated || Pair.Value.bIsOn || Pair.Value.bIsPressed)
			{
				ActiveObjectIds.Add(Pair.Key);
			}
		}
		ActivationComponent->SetActiveObjectIds(ActiveObjectIds);

		for (const TPair<FGuid, FGridRuntimeInteractiveState>& Pair : State->InteractiveObjects)
		{
			const bool bIsActive = Pair.Value.bIsActivated || Pair.Value.bIsOn || Pair.Value.bIsPressed;
			if (AGridLeverActor* LeverActor = FindRuntimeObjectActor<AGridLeverActor>(Pair.Key))
			{
				LeverActor->SetLeverState(bIsActive);
			}
			if (AGridPressurePlateActor* PlateActor = FindRuntimeObjectActor<AGridPressurePlateActor>(Pair.Key))
			{
				PlateActor->SetPressed(bIsActive);
			}
		}
	}

	auto RemoveSpawnedItemEntry = [this](int32 EntryIndex)
	{
		if (!SpawnedItemEntries.IsValidIndex(EntryIndex))
		{
			return;
		}

		AGridItemActor* ItemActor = SpawnedItemEntries[EntryIndex].ItemActor.Get();
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
		SpawnedItemEntries.RemoveAtSwap(EntryIndex);
	};

	for (const TPair<FGuid, FGridRuntimeObjectPresenceState>& Pair : State->ObjectPresence)
	{
		const FGridRuntimeObjectPresenceState& PresenceState = Pair.Value;
		if (!PresenceState.bRemovedFromInitialPlacement)
		{
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("GridRuntimeState Apply RemovedInitialObject ObjectId=%s"), *PresenceState.ObjectId.ToString());

		for (int32 EntryIndex = SpawnedItemEntries.Num() - 1; EntryIndex >= 0; --EntryIndex)
		{
			if (SpawnedItemEntries[EntryIndex].ObjectId == PresenceState.ObjectId)
			{
				RemoveSpawnedItemEntry(EntryIndex);
			}
		}
	}

	for (const TPair<FGuid, FGridRuntimeItemState>& Pair : State->Items)
	{
		const FGridRuntimeItemState& ItemState = Pair.Value;
		if (ItemState.bIsContainedInReceptacle)
		{
			continue;
		}

		bool bFoundExistingItem = false;
		for (FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
		{
			if (Entry.ObjectId != Pair.Key)
			{
				continue;
			}

			if (AGridItemActor* ItemActor = Entry.ItemActor.Get())
			{
				ItemActor->SetActorTransform(ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
				ItemActor->SetRuntimeObjectId(Pair.Key);
				ItemActor->SetRuntimeCell(ItemState.CellX, ItemState.CellY);
				ItemActor->SetItemLightsEnabled(ItemState.bLightsEnabled);
				ItemActor->InitializeReadableContent(
					ItemState.ReadableContentAsset, ItemState.ReadableContentId, ItemState.ReadTitleOverride, ItemState.ReadTextOverride);
				Entry.Cell = FIntPoint(ItemState.CellX, ItemState.CellY);
				Entry.Edge = ItemState.Edge;
				Entry.Quantity = FMath::Max(1, ItemState.Quantity);
				bFoundExistingItem = true;
			}
			break;
		}

		const FName RuntimeItemDefinitionId = !ItemState.ItemDefinitionId.IsNone() ? ItemState.ItemDefinitionId : ItemState.ArchetypeId;
		if (!bFoundExistingItem && !RuntimeItemDefinitionId.IsNone())
		{
			UGridItemDefinitionAsset* ItemDefinition = ResolveRuntimeItemDefinition(RuntimeItemDefinitionId);
			AGridItemActor* ItemActor = SpawnItemActorForDefinition(ItemDefinition, RuntimeItemDefinitionId, this, nullptr);
			if (ItemActor)
			{
				const FGridLevelObjectData* ItemObjectData = FindLevelObjectDataById(LevelAsset, Pair.Key);
				const FIntPoint RuntimeCell =
					ItemObjectData ? FIntPoint(ItemObjectData->CellX, ItemObjectData->CellY) : FIntPoint(ItemState.CellX, ItemState.CellY);
				const EGridEdge RuntimeEdge = ItemObjectData ? ItemObjectData->Edge : ItemState.Edge;

				ItemActor->SetActorTransform(ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
				ItemActor->SetRuntimeObjectId(Pair.Key);
				ItemActor->SetRuntimeCell(RuntimeCell.X, RuntimeCell.Y);
				ItemActor->ConfigureAsWorldPickup();
				ItemActor->OnPlacedInWorld();
				ItemActor->SetItemLightsEnabled(ItemState.bLightsEnabled);
				ItemActor->InitializeReadableContent(
					ItemState.ReadableContentAsset, ItemState.ReadableContentId, ItemState.ReadTitleOverride, ItemState.ReadTextOverride);
				SpawnedItemActors.Add(ItemActor);

				FGridSpawnedItemRuntimeEntry Entry;
				Entry.Cell = RuntimeCell;
				Entry.Edge = RuntimeEdge;
				Entry.ItemActor = ItemActor;
				Entry.ObjectId = Pair.Key;
				Entry.ItemArchetypeId = !ItemState.ArchetypeId.IsNone() ? ItemState.ArchetypeId : RuntimeItemDefinitionId;
				Entry.ItemDefinitionAsset = ItemDefinition;
				Entry.ItemDefinitionId = RuntimeItemDefinitionId;
				Entry.Quantity = FMath::Max(1, ItemState.Quantity);
				SpawnedItemEntries.Add(Entry);
			}
		}
	}
	for (const TPair<FGuid, FGridRuntimeReceptacleState>& Pair : State->Receptacles)
	{
		AGridReceptacleActor* ReceptacleActor = FindRuntimeObjectActor<AGridReceptacleActor>(Pair.Key);
		if (!ReceptacleActor)
		{
			continue;
		}
		const int32 ClearedItemCount = ReceptacleActor->ForceClearRuntimeContents(false);
		const FGridLevelObjectData* ReceptacleObjectData = FindLevelObjectDataById(LevelAsset, Pair.Key);
		const UGridObjectArchetypeAsset* ReceptacleArchetype = ReceptacleObjectData ? FindObjectArchetype(ReceptacleObjectData->ArchetypeId) : nullptr;
		const TSubclassOf<AGridItemActor> PreferredItemActorClass = ReceptacleActor->ContainedItemActorClass
			? ReceptacleActor->ContainedItemActorClass
			: (ReceptacleArchetype ? ReceptacleArchetype->ItemActorClass : nullptr);
		for (const FGridRuntimeItemState& ItemState : Pair.Value.ContainedItems)
		{
			const FName RuntimeItemDefinitionId = !ItemState.ItemDefinitionId.IsNone() ? ItemState.ItemDefinitionId : ItemState.ArchetypeId;
			if (RuntimeItemDefinitionId.IsNone())
			{
				UE_LOG(LogTemp, Warning,
					TEXT("GridRuntimeState Apply skipped receptacle item: ReceptacleId=%s RuntimeId=%s no ItemDefinitionId or legacy ArchetypeId resolved."),
					*Pair.Key.ToString(), *ItemState.ObjectId.ToString());
				continue;
			}
			UGridItemDefinitionAsset* ItemDefinition = ResolveRuntimeItemDefinition(RuntimeItemDefinitionId);
			AGridItemActor* ItemActor = SpawnItemActorForDefinition(
				ItemDefinition, RuntimeItemDefinitionId, ReceptacleActor, ReceptacleActor->ItemAttachPoint.Get(), PreferredItemActorClass);
			if (ItemActor)
			{
				ItemActor->SetRuntimeObjectId(ItemState.ObjectId);

				if (ItemDefinition)
				{
					ItemActor->InitializeFromItemDefinition(ItemDefinition, ItemState.ObjectId);
				}
				else
				{
					ItemActor->InitializeFromItemDefinitionId(RuntimeItemDefinitionId, ItemState.ObjectId);
				}
				ItemActor->SetItemLightsEnabled(ItemState.bLightsEnabled);
			}
			FGridRuntimeItemState ResolvedItemState = ItemState;
			ResolvedItemState.ItemDefinitionId = RuntimeItemDefinitionId;
			if (ResolvedItemState.ArchetypeId.IsNone())
			{
				ResolvedItemState.ArchetypeId = RuntimeItemDefinitionId;
			}
			ReceptacleActor->RestoreRuntimeContainedItem(ResolvedItemState, ItemActor);
		}
		UE_LOG(LogTemp, Verbose, TEXT("GridRuntimeState Apply Receptacle Final ObjectId=%s HasItem=%s Count=%d"), *Pair.Key.ToString(),
			ReceptacleActor->HasItem() ? TEXT("true") : TEXT("false"), ReceptacleActor->GetContainedItemCount());
	}

	TMap<FGuid, TArray<AGridMonsterActor*>> MonstersByPersistenceId;
	TArray<AGridMonsterActor*> CurrentLevelMonsters;
	for (AGridMonsterActor* Monster : WorldMonsters)
	{
		if (Monster->ResolveRuntimeDungeonLevelId(State->LevelId) != State->LevelId)
		{
			continue;
		}

		CurrentLevelMonsters.Add(Monster);
		const FGuid PersistenceId = Monster->ResolvePersistenceId();
		if (PersistenceId.IsValid())
		{
			MonstersByPersistenceId.FindOrAdd(PersistenceId).Add(Monster);
		}
		else
		{
			UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterState] ActivateLevel skipped Level=%s Monster=%s Reason=InvalidPersistenceId"),
				*State->LevelId.ToString(), *GetNameSafe(Monster));
		}
	}

	for (const TPair<FGuid, TArray<AGridMonsterActor*>>& Pair : MonstersByPersistenceId)
	{
		if (Pair.Value.Num() > 1)
		{
			UE_LOG(LogGridMonsterState, Error, TEXT("[GridMonsterState] DuplicatePersistenceId Level=%s PersistenceId=%s Count=%d"), *State->LevelId.ToString(),
				*Pair.Key.ToString(), Pair.Value.Num());
		}
	}

	TArray<FGuid> SavedMonsterIds;
	State->Monsters.GetKeys(SavedMonsterIds);
	SavedMonsterIds.Sort(
		[](const FGuid& Left, const FGuid& Right)
		{
			return Left.ToString(EGuidFormats::Digits) < Right.ToString(EGuidFormats::Digits);
		});

	TSet<FGuid> AppliedMonsterIds;
	for (const FGuid& SavedMonsterId : SavedMonsterIds)
	{
		const FGridRuntimeMonsterState* SavedMonsterState = State->Monsters.Find(SavedMonsterId);
		if (!SavedMonsterState)
		{
			continue;
		}

		const TArray<AGridMonsterActor*>* MatchingActors = MonstersByPersistenceId.Find(SavedMonsterId);
		if (!MatchingActors || MatchingActors->IsEmpty())
		{
			UE_LOG(LogGridMonsterState, Warning, TEXT("[GridMonsterState] MissingActor Level=%s PersistenceId=%s Definition=%s Cell=(%d,%d)"),
				*State->LevelId.ToString(), *SavedMonsterId.ToString(), *SavedMonsterState->MonsterDefinitionId.ToString(), SavedMonsterState->CellX,
				SavedMonsterState->CellY);
			continue;
		}
		if (MatchingActors->Num() != 1)
		{
			continue;
		}

		AGridMonsterActor* Monster = (*MatchingActors)[0];
		Monster->RestoreRuntimeMonsterState(*SavedMonsterState, this);
		AppliedMonsterIds.Add(SavedMonsterId);
	}

	for (AGridMonsterActor* Monster : CurrentLevelMonsters)
	{
		const FGuid PersistenceId = Monster->ResolvePersistenceId();
		const TArray<AGridMonsterActor*>* MatchingActors = MonstersByPersistenceId.Find(PersistenceId);
		if (!PersistenceId.IsValid() || (MatchingActors && MatchingActors->Num() > 1) || AppliedMonsterIds.Contains(PersistenceId))
		{
			continue;
		}

		// Version 1 and partial legacy states intentionally preserve the
		// actor's initial runtime values.
		Monster->EnsureInitialCombatState();
		SetMonsterRuntimeLevelActive(Monster, true);
	}

	for (AGridMonsterActor* Monster : CurrentLevelMonsters)
	{
		if (!Monster->IsRuntimeLevelActive() || Monster->IsDead() || !Monster->bMonsterEnabled)
		{
			continue;
		}

		if (UGridMonsterBehaviorComponent* Behavior = Monster->FindComponentByClass<UGridMonsterBehaviorComponent>())
		{
			Behavior->RefreshPerception();
		}
	}

	if (ActivationComponent)
	{
		ActivationComponent->RefreshAllPressurePlates();
	}

	int32 DeadMonsterCount = 0;
	for (const TPair<FGuid, FGridRuntimeMonsterState>& Pair : State->Monsters)
	{
		DeadMonsterCount += Pair.Value.bIsDead ? 1 : 0;
	}

	UE_LOG(LogTemp, Log, TEXT("GridRuntimeState Apply Level=%s Doors=%d RemovedObjects=%d Items=%d Receptacles=%d Interactives=%d Monsters=%d DeadMonsters=%d"),
		*State->LevelId.ToString(), State->Doors.Num(), CountRemovedRuntimeObjects(State), State->Items.Num(), State->Receptacles.Num(),
		State->InteractiveObjects.Num(), State->Monsters.Num(), DeadMonsterCount);

	return true;
}
