#include "Runtime/GridLevelRuntimeActor.h"
#include "CoreGlobals.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EngineUtils.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "GridWorldPickupIdentityReadModel.h"
#include "GridCanonicalWorldSnapshotReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridThrownItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridLevelRuntime, Log, All);

namespace
{
	FGridItemInstance PersistenceMakeVisualItem(const FGridRuntimeItemState& Saved)
	{
		FGridItemInstance Instance;
		Instance.RuntimeObjectId = Saved.ObjectId;
		Instance.ItemDefinitionId = Saved.ItemDefinitionId;
		Instance.ItemLevel = Saved.ItemLevel;
		Instance.Quantity = FMath::Max(1, Saved.Quantity);
		return Instance;
	}

	const FGridLooseItemInstance* PersistenceFindLooseItemById(const UGridLevelAsset* LevelAsset, FGuid ObjectId)
	{
		if (!LevelAsset || !ObjectId.IsValid())
		{
			return nullptr;
		}

		return LevelAsset->LooseItemInstances.FindByPredicate(
			[ObjectId](const FGridLooseItemInstance& Instance)
			{
				return Instance.InstanceId == ObjectId;
			});
	}

	bool PersistenceHasMonsterSpawn(const UGridLevelAsset* LevelAsset, FGuid SpawnId)
	{
		return LevelAsset && SpawnId.IsValid() && LevelAsset->MonsterSpawns.ContainsByPredicate(
			[SpawnId](const FGridMonsterSpawnInstance& Spawn)
			{
				return Spawn.SpawnId == SpawnId;
			});
	}

	int32 PersistenceCountRemovedRuntimeObjects(const FGridLevelRuntimeState* RuntimeState)
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

	void PersistenceGetWorldMonsters(const UWorld* World, TArray<AGridMonsterActor*>& OutMonsters)
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

	// Validate every receptacle before resetting any existing level snapshot.
	TMap<FGuid, FGridRuntimeReceptacleState> CapturedReceptacles;
	for (const TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
	{
		const AGridReceptacleActor* Receptacle = Cast<AGridReceptacleActor>(Pair.Value.Get());
		if (!IsValid(Receptacle)) continue;
		FGridRuntimeReceptacleState ReceptacleState;
		if (!Pair.Key.IsValid() || !Receptacle->CaptureRuntimeReceptacleState(ReceptacleState) ||
			ReceptacleState.ObjectId != Pair.Key)
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridRuntimeState Capture rejected receptacle %s"), *Pair.Key.ToString());
			return false;
		}
		CapturedReceptacles.Add(Pair.Key, MoveTemp(ReceptacleState));
	}

	// Stage all physical rows before modifying the previous snapshot. The
	// historical registry-less automation fixtures are not a production path.
	bool bRequireCanonicalCapture = true;
#if WITH_DEV_AUTOMATION_TESTS
	bRequireCanonicalCapture = !GIsAutomationTesting || !GetWorld() || GetWorld()->GetGameInstance();
#endif
	RefreshWorldItemPhysicsCells();
	TMap<FGuid, FGridRuntimeItemState> CapturedItems;
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
		ItemState.ItemDefinitionId = bRequireCanonicalCapture
			? Entry.ItemDefinitionId
			: FGridWorldPickupIdentityReadModel::Resolve(ItemActor, Entry.ItemDefinitionId, Entry.ItemLevel);
		if (bRequireCanonicalCapture &&
			(!Entry.ObjectId.IsValid() || ItemActor->GetRuntimeObjectId() != Entry.ObjectId ||
			ItemActor->ItemDefinitionId != Entry.ItemDefinitionId ||
			ItemActor->CanonicalWorldItemLevel != Entry.ItemLevel ||
			CapturedItems.Contains(Entry.ObjectId)))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridRuntimeState Capture rejected conflicting world identity: ObjectId=%s"), *Entry.ObjectId.ToString());
			return false;
		}
		ItemState.ItemLevel = Entry.ItemLevel;
		ItemState.Quantity = bRequireCanonicalCapture ? Entry.Quantity : FMath::Max(1, Entry.Quantity);
		ItemState.CellX = Entry.Cell.X;
		ItemState.CellY = Entry.Cell.Y;
		ItemState.Edge = Entry.Edge;

		if (ItemState.ItemDefinitionId.IsNone())
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridRuntimeState Capture skipped item: ObjectId=%s Actor=%s no ItemDefinitionId resolved."),
				*Entry.ObjectId.ToString(), *GetNameSafe(ItemActor));
			if (bRequireCanonicalCapture) return false;
			continue;
		}
		ItemState.Transform = ItemActor->GetActorTransform();
		ItemState.bIsSimulatingPhysics = ItemActor->MeshComponent ? ItemActor->MeshComponent->IsSimulatingPhysics() : false;
		if (IsValid(ItemActor->MeshComponent) && ItemState.bIsSimulatingPhysics)
		{
			ItemState.bHasWorldMeshTransform = true;
			ItemState.WorldMeshTransform = ItemActor->MeshComponent->GetComponentTransform();
		}
		ItemState.bIsContainedInReceptacle = false;
		ItemState.bLightsEnabled = ItemActor->AreItemLightsEnabled();
		ItemState.ReadableContentAsset = ItemActor->ReadableContentAsset;
		ItemState.ReadableContentId = ItemActor->ReadableContentId;
		ItemState.ReadTitleOverride = ItemActor->ReadTitleOverride;
		ItemState.ReadTextOverride = ItemActor->ReadTextOverride;
		CapturedItems.Add(Entry.ObjectId, ItemState);
	}
	// Every recoverable projectile is already consumed from inventory, but
	// is not yet in SpawnedItemEntries. Capture by its physical GUID.
	// In-flight items reload as pickups, not a second shot.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AGridThrownItemActor> It(World); It; ++It)
		{
			const AGridThrownItemActor* Arrow = *It;
			if (!IsValid(Arrow) || Arrow->RuntimeActor != this ||
				Arrow->HasCompletedImpactConversion() ||
				(!bRequireCanonicalCapture && !Arrow->bRangedAmmunitionProjectile))
			{
				continue;
			}
			const FGuid Guid = Arrow->ThrownItemInstance.RuntimeObjectId;
			if (!Guid.IsValid() || (bRequireCanonicalCapture &&
				(Arrow->ThrownItemInstance.Quantity != 1 || Arrow->GetRuntimeObjectId() != Guid ||
				 Arrow->ItemDefinitionId != Arrow->ThrownItemInstance.ItemDefinitionId ||
				 Arrow->CanonicalWorldItemLevel != Arrow->ThrownItemInstance.ItemLevel)))
			{
				if (bRequireCanonicalCapture) return false;
				continue;
			}
			if (CapturedItems.Contains(Guid))
			{
				UE_LOG(LogGridLevelRuntime, Warning,
					TEXT("[GridProjectile] CaptureDuplicateGuid Id=%s"), *Guid.ToString());
				if (bRequireCanonicalCapture) return false;
				continue;
			}
			FGridRuntimeItemState Item;
			Item.ObjectId = Guid;
			Item.ItemDefinitionId = Arrow->ThrownItemInstance.ItemDefinitionId;
			Item.ItemLevel = Arrow->ThrownItemInstance.ItemLevel;
			Item.Quantity = 1;
			Item.Transform = Arrow->GetActorTransform();
			const FTransform FlightPose = IsValid(Arrow->MeshComponent)
				? Arrow->MeshComponent->GetComponentTransform() : Item.Transform;
			// Only arrows preserve the flight mesh pose. Other recoverable
			// items rematerialize their canonical pickup mesh, not its flight scale.
			Item.bHasWorldMeshTransform = Arrow->bRangedAmmunitionProjectile && IsValid(Arrow->MeshComponent);
			Item.WorldMeshTransform = Item.bHasWorldMeshTransform ? FlightPose : Item.Transform;
			if (!Arrow->bRangedAmmunitionProjectile)
				Item.Transform.SetLocation(FlightPose.GetLocation());
			// Only worldless historical fixtures still require this v27 field.
#if WITH_DEV_AUTOMATION_TESTS
			if (!bRequireCanonicalCapture)
				Item.ProjectileDefinitionAsset = Arrow->GetItemDefinitionAsset();
#endif
			Item.bLightsEnabled = Arrow->AreItemLightsEnabled();
			Item.ReadableContentAsset = Arrow->ThrownItemInstance.ReadableContentAsset;
			Item.ReadableContentId = Arrow->ThrownItemInstance.ReadableContentId;
			Item.ReadTitleOverride = Arrow->ThrownItemInstance.ReadTitleOverride;
			Item.ReadTextOverride = Arrow->ThrownItemInstance.ReadTextOverride;
			int32 X = INDEX_NONE, Y = INDEX_NONE;
			FVector Offset;
			if (!TryResolveWorldCellFromImpactPoint(FlightPose.GetLocation(), X, Y, Offset))
			{
				X = Arrow->SourceCellX;
				Y = Arrow->SourceCellY;
				if (!Arrow->bRangedAmmunitionProjectile)
				{
					if (!IsWalkableCell(X, Y)) return false;
					Item.Transform.SetLocation(GetCellCenterWorld(X, Y, 12.f));
				}
			}
			Item.CellX = X;
			Item.CellY = Y;
			Item.bEmbeddedProjectile = Arrow->CaptureEmbeddedAttachment(
				Item.EmbeddedMonsterPersistenceId, Item.EmbeddedBone,
				Item.EmbeddedBoneRelativeTransform);
			CapturedItems.Add(Guid, Item);
		}
	}

	if (bRequireCanonicalCapture)
	{
		FGridLevelRuntimeState Candidate;
		if (const FGridLevelRuntimeState* Previous = FindRuntimeStateForCurrentLevel())
		{
			Candidate.LevelId = Previous->LevelId;
			Candidate.PendingInboundItems = Previous->PendingInboundItems;
		}
		Candidate.Items = CapturedItems;
		Candidate.Receptacles = CapturedReceptacles;
		FString Error;
		if (!FGridCanonicalWorldSnapshotReadModel::ValidateLevel(
			FGridCanonicalWorldSnapshotReadModel::GetRegistry(GetWorld()), Candidate, Error))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridRuntimeState Capture rejected: %s"), *Error);
			return false;
		}
	}

	FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel();
	if (!State)
	{
		return false;
	}

	State->Doors.Reset();
	State->InteractiveObjects.Reset();
	State->ObjectPresence.Reset();
	State->Items = MoveTemp(CapturedItems);
	State->Receptacles.Reset();
	State->Monsters.Reset();
	State->bHasBeenVisited = true;

	for (const FGridWorldObjectInstance& Instance : LevelAsset->WorldObjectInstances)
	{
		if (!Instance.InstanceId.IsValid())
		{
			continue;
		}

		if (Instance.Type == EGridLevelObjectType::Door && DoorSystemComponent)
		{
			bool bDoorOpen = false;
			bool bDoorMoving = false;
			bool bDoorBlocked = true;
			if (DoorSystemComponent->GetDoorState(Instance.InstanceId, bDoorOpen, bDoorMoving, bDoorBlocked))
			{
				FGridRuntimeDoorState DoorState;
				DoorState.ObjectId = Instance.InstanceId;
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
		for (const FGridWorldObjectInstance& Instance : LevelAsset->WorldObjectInstances)
		{
			if (!Instance.InstanceId.IsValid())
			{
				continue;
			}

			// Relocations share the same active-id authority as other stateful world objects.
			// Persist them so revisiting a level does not erase an enabled stair/teleporter.
			const bool bIsInteractiveObject = Instance.Type == EGridLevelObjectType::Button || Instance.Type == EGridLevelObjectType::Lever ||
				Instance.Type == EGridLevelObjectType::PressurePlate || Instance.Type == EGridLevelObjectType::Receptacle ||
				Instance.Type == EGridLevelObjectType::Trigger || Instance.Type == EGridLevelObjectType::Relocation;
			if (!bIsInteractiveObject)
			{
				continue;
			}

			const bool bIsActive = ActiveObjectIds.Contains(Instance.InstanceId);
			FGridRuntimeInteractiveState InteractiveState;
			InteractiveState.ObjectId = Instance.InstanceId;
			InteractiveState.bIsActivated = bIsActive;
			InteractiveState.bIsPressed = bIsActive;
			InteractiveState.bIsOn = bIsActive;
			State->InteractiveObjects.Add(Instance.InstanceId, InteractiveState);
		}
	}


	for (const FGridLooseItemInstance& Instance : LevelAsset->LooseItemInstances)
	{
		if (!Instance.InstanceId.IsValid())
		{
			continue;
		}

		FGridRuntimeObjectPresenceState PresenceState;
		PresenceState.ObjectId = Instance.InstanceId;
		PresenceState.bRemovedFromInitialPlacement = !ExistingPlacedItemObjectIds.Contains(Instance.InstanceId);
		State->ObjectPresence.Add(Instance.InstanceId, PresenceState);
	}
	// An ItemSpawn's stable SpawnId is also the initial pickup's ObjectId.
	// Persist collection/removal like a loose item, so it cannot regenerate.
	for (const FGridItemSpawnInstance& Spawn : LevelAsset->ItemSpawns)
	{
		if (!Spawn.SpawnId.IsValid())
		{
			continue;
		}
		FGridRuntimeObjectPresenceState PresenceState;
		PresenceState.ObjectId = Spawn.SpawnId;
		PresenceState.bRemovedFromInitialPlacement = !ExistingPlacedItemObjectIds.Contains(Spawn.SpawnId);
		State->ObjectPresence.Add(Spawn.SpawnId, PresenceState);
	}

	State->Receptacles = MoveTemp(CapturedReceptacles);

	TArray<AGridMonsterActor*> Monsters;
	PersistenceGetWorldMonsters(GetWorld(), Monsters);
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
			if (MonsterState.SpawnObjectId.IsValid() && PersistenceHasMonsterSpawn(LevelAsset, MonsterState.SpawnObjectId))
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

	for (const FGridMonsterSpawnInstance& Spawn : LevelAsset->MonsterSpawns)
	{
		if (!Spawn.SpawnId.IsValid() || CapturedMonsterSpawnIds.Contains(Spawn.SpawnId))
		{
			continue;
		}

		FGridRuntimeMonsterPlacementState* PlacementState = State->MonsterPlacements.Find(Spawn.SpawnId);
		if (!PlacementState)
		{
			FGridRuntimeMonsterPlacementState InitialPlacementState;
			InitialPlacementState.SpawnId = Spawn.SpawnId;
			InitialPlacementState.bIsSpawned = Spawn.bSpawnAtStart;
			State->MonsterPlacements.Add(Spawn.SpawnId, InitialPlacementState);
		}
	}

	int32 DeadMonsterCount = 0;
	for (const TPair<FGuid, FGridRuntimeMonsterState>& Pair : State->Monsters)
	{
		DeadMonsterCount += Pair.Value.bIsDead ? 1 : 0;
	}

	UE_LOG(LogGridLevelRuntime, Verbose,
		TEXT(
			"GridRuntimeState Capture Level=%s Doors=%d RemovedObjects=%d Items=%d Receptacles=%d Interactives=%d Monsters=%d MonsterPlacements=%d DeadMonsters=%d"),
		*State->LevelId.ToString(), State->Doors.Num(), PersistenceCountRemovedRuntimeObjects(State), State->Items.Num(), State->Receptacles.Num(),
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
	const UGridItemSubsystem* RestoreRegistry =
		FGridCanonicalWorldSnapshotReadModel::GetRegistry(GetWorld());
	// Pending inbound items on an unvisited level are also validated.
	bool bRequireCanonicalRestore = true;
#if WITH_DEV_AUTOMATION_TESTS
	bRequireCanonicalRestore = !GIsAutomationTesting || !GetWorld() || GetWorld()->GetGameInstance();
#endif
	if (bRequireCanonicalRestore)
	{
		FString ItemError;
		if (State && !FGridCanonicalWorldSnapshotReadModel::ValidateLevel(RestoreRegistry, *State, ItemError))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridRuntimeState Apply rejected: %s"), *ItemError);
			return false;
		}
	}
	if (!State || !State->bHasBeenVisited)
	{
		ApplyInitialMonsterStateForCurrentLevel();
		ApplyPendingInboundItemsForCurrentLevel();
		return false;
	}

	AbortActiveCombatAndMonsterActions();


	TArray<AGridMonsterActor*> WorldMonsters;
	PersistenceGetWorldMonsters(GetWorld(), WorldMonsters);
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

	for (const TPair<FGuid, FGridRuntimePitState>& Pair : State->Pits)
	{
		if (AGridPitTrapdoorActor* PitActor = FindRuntimeObjectActor<AGridPitTrapdoorActor>(Pair.Key))
		{
			PitActor->SnapPitOpenState(Pair.Value.bIsOpen);
		}
	}

	for (const TPair<FGuid, FGridRuntimeObjectVisualState>& Pair : State->ObjectVisuals)
	{
		AGridRuntimeObjectActor* RuntimeObject = FindRuntimeObjectActor<AGridRuntimeObjectActor>(Pair.Key);
		if (!RuntimeObject)
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridRuntimeState Apply visual skipped: ObjectId=%s Reason=runtime actor not found."), *Pair.Key.ToString());
			continue;
		}

		for (const TPair<FName, FName>& MaterialOverride : Pair.Value.MaterialAliasesBySlot)
		{
			FString Error;
			if (!RuntimeObject->SetRuntimeMaterialAlias(MaterialOverride.Key, MaterialOverride.Value, false, Error))
			{
				UE_LOG(LogGridLevelRuntime, Warning,
					TEXT("GridRuntimeState Apply visual skipped: ObjectId=%s Slot=%s Alias=%s Reason=%s"), *Pair.Key.ToString(),
					*MaterialOverride.Key.ToString(), *MaterialOverride.Value.ToString(), *Error);
			}
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

		UE_LOG(LogGridLevelRuntime, Verbose, TEXT("GridRuntimeState Apply RemovedInitialObject ObjectId=%s"), *PresenceState.ObjectId.ToString());

		for (int32 EntryIndex = SpawnedItemEntries.Num() - 1; EntryIndex >= 0; --EntryIndex)
		{
			if (SpawnedItemEntries[EntryIndex].ObjectId == PresenceState.ObjectId)
			{
				RemoveSpawnedItemEntry(EntryIndex);
			}
		}
	}

	// A loaded world may still have live projectile presentation actors.
	// Saved GUIDs are authoritative: remove previous actors first.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AGridThrownItemActor> It(World); It; ++It)
		{
			AGridThrownItemActor* Arrow = *It;
			if (IsValid(Arrow) && Arrow->RuntimeActor == this)
			{
				Arrow->Destroy();
			}
		}
	}
	for (int32 Index = SpawnedItemEntries.Num() - 1; Index >= 0; --Index)
	{
		const FGridRuntimeItemState* Saved = State->Items.Find(SpawnedItemEntries[Index].ObjectId);
		if (Saved && Saved->bEmbeddedProjectile)
		{
			RemoveSpawnedItemEntry(Index);
		}
	}
	for (const TPair<FGuid, FGridRuntimeItemState>& Pair : State->Items)
	{
		const FGridRuntimeItemState& ItemState = Pair.Value;
		if (ItemState.bIsContainedInReceptacle || ItemState.bEmbeddedProjectile)
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
				ItemActor->ApplyCanonicalWorldVisual(PersistenceMakeVisualItem(ItemState));
				ItemActor->SetActorTransform(ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
				if (ItemState.bHasWorldMeshTransform && IsValid(ItemActor->MeshComponent))
				{
					ItemActor->MeshComponent->SetWorldTransform(ItemState.WorldMeshTransform,
						false, nullptr, ETeleportType::TeleportPhysics);
				}
				ItemActor->SetRuntimeObjectId(Pair.Key);
				ItemActor->SetRuntimeCell(ItemState.CellX, ItemState.CellY);
				ItemActor->SetItemLightsEnabled(ItemState.bLightsEnabled);
				ItemActor->InitializeReadableContent(
					ItemState.ReadableContentAsset, ItemState.ReadableContentId, ItemState.ReadTitleOverride, ItemState.ReadTextOverride);
				Entry.Cell = FIntPoint(ItemState.CellX, ItemState.CellY);
				Entry.Edge = ItemState.Edge;
				Entry.Quantity = FMath::Max(1, ItemState.Quantity);
				Entry.ItemLevel = ItemState.ItemLevel;
				bFoundExistingItem = true;
			}
			break;
		}

		const FName RuntimeItemDefinitionId = ItemState.ItemDefinitionId;
		if (!bFoundExistingItem && !RuntimeItemDefinitionId.IsNone())
		{
			// A flying canonical item reloads as a pickup, not a second
			// projectile. Old SaveGame pointers are not its authoring authority.
			const bool bCanonicalItem = RestoreRegistry &&
				RestoreRegistry->HasCanonicalItem(RuntimeItemDefinitionId);
			FGridResolvedItem ResolvedItem;
			if (bCanonicalItem &&
				!RestoreRegistry->ResolveItem(RuntimeItemDefinitionId, ItemState.ItemLevel, ResolvedItem))
			{
				UE_LOG(LogGridLevelRuntime, Warning,
					TEXT("[GridProjectile] RestorePickupRejected Item=%s Level=%d Reason=InvalidCanonicalItem"),
					*RuntimeItemDefinitionId.ToString(), ItemState.ItemLevel);
				continue;
			}
			// Restore the complete world profile for every registered
			// physical ItemId, including ARM N1-N20 after a PIT transfer.
			const bool bCanonicalWorld = bCanonicalItem;
			UGridItemDefinitionAsset* ItemDefinition = nullptr;
#if WITH_DEV_AUTOMATION_TESTS
			if (!bRequireCanonicalRestore && !bCanonicalWorld)
				ItemDefinition = ItemState.ProjectileDefinitionAsset.Get();
#endif
			AGridItemActor* ItemActor = SpawnItemActorForDefinition(ItemDefinition, RuntimeItemDefinitionId, this, nullptr);
			if (ItemActor)
			{
				if (bCanonicalWorld &&
					!ItemActor->InitializeFromCanonicalItem(
						PersistenceMakeVisualItem(ItemState), RestoreRegistry))
				{
					UE_LOG(LogGridLevelRuntime, Warning,
						TEXT("GridRuntimeState Apply canonical world item rejected: %s N%d."),
						*RuntimeItemDefinitionId.ToString(), ItemState.ItemLevel);
					ItemActor->Destroy();
					continue; // Never downgrade a known canonical item.
				}
				const FGridLooseItemInstance* ItemPlacement = PersistenceFindLooseItemById(LevelAsset, Pair.Key);
				const FIntPoint RuntimeCell =
					ItemPlacement ? FIntPoint(ItemPlacement->CellX, ItemPlacement->CellY) : FIntPoint(ItemState.CellX, ItemState.CellY);
				const EGridEdge RuntimeEdge = ItemPlacement ? ItemPlacement->SurfaceSide : ItemState.Edge;

				ItemActor->ApplyCanonicalWorldVisual(PersistenceMakeVisualItem(ItemState));
				ItemActor->SetActorTransform(ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
				if (ItemState.bHasWorldMeshTransform && IsValid(ItemActor->MeshComponent))
				{
					ItemActor->MeshComponent->SetWorldTransform(ItemState.WorldMeshTransform,
						false, nullptr, ETeleportType::TeleportPhysics);
				}
				ItemActor->SetRuntimeObjectId(Pair.Key);
				ItemActor->SetRuntimeCell(RuntimeCell.X, RuntimeCell.Y);
				ItemActor->ConfigureAsWorldPickup();
				if (ItemState.bHasWorldMeshTransform && IsValid(ItemActor->MeshComponent))
				{
					ItemActor->MeshComponent->SetWorldTransform(ItemState.WorldMeshTransform,
						false, nullptr, ETeleportType::TeleportPhysics);
				}
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
				Entry.ItemDefinitionAsset = ItemDefinition;
				Entry.ItemDefinitionId = RuntimeItemDefinitionId;
				Entry.ItemLevel = ItemState.ItemLevel;
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
		ReceptacleActor->SetCanRemoveItem(Pair.Value.bCanRemoveItem);
		ReceptacleActor->SetCanInsertItems(Pair.Value.bCanInsertItems);
		const int32 ClearedItemCount = ReceptacleActor->ForceClearRuntimeContents(false);
		for (const FGridRuntimeItemState& ItemState : Pair.Value.ContainedItems)
		{
			const FName RuntimeItemDefinitionId = ItemState.ItemDefinitionId;
			if (RuntimeItemDefinitionId.IsNone())
			{
				UE_LOG(LogGridLevelRuntime, Warning,
					TEXT("GridRuntimeState Apply skipped receptacle item: ReceptacleId=%s RuntimeId=%s no ItemDefinitionId resolved."),
					*Pair.Key.ToString(), *ItemState.ObjectId.ToString());
				continue;
			}
			FGridResolvedItem CanonicalItem;
			if (!RestoreRegistry || !RestoreRegistry->ResolveItem(RuntimeItemDefinitionId, ItemState.ItemLevel, CanonicalItem) ||
				!CanonicalItem.IsValid() || CanonicalItem.ItemLevel != ItemState.ItemLevel ||
				!ItemState.ObjectId.IsValid() || ItemState.Quantity < 1 ||
				ItemState.Quantity > FMath::Max(1, CanonicalItem.MaxStack))
			{
				UE_LOG(LogGridLevelRuntime, Warning,
					TEXT("GridRuntimeState Apply canonical receptacle item rejected: %s N%d."),
					*RuntimeItemDefinitionId.ToString(), ItemState.ItemLevel);
				continue;
			}
			AGridItemActor* ItemActor = SpawnItemActorForDefinition(
				nullptr, RuntimeItemDefinitionId, ReceptacleActor, ReceptacleActor->ItemAttachPoint.Get());
			if (IsValid(ItemActor))
			{
				if (!ItemActor->InitializeFromCanonicalItem(PersistenceMakeVisualItem(ItemState), RestoreRegistry))
				{
					ItemActor->Destroy();
					continue;
				}
				ItemActor->SetRuntimeObjectId(ItemState.ObjectId);
			}
			if (!ReceptacleActor->RestoreRuntimeContainedItem(ItemState, ItemActor) && IsValid(ItemActor))
			{
				ItemActor->Destroy();
			}
		}
		UE_LOG(LogGridLevelRuntime, Verbose, TEXT("GridRuntimeState Apply Receptacle Final ObjectId=%s HasItem=%s Count=%d"), *Pair.Key.ToString(),
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

		Monster->EnsureInitialCombatState();
		SetMonsterRuntimeLevelActive(Monster, true);
	}

	// Restore attached ammunition only after monsters have been restored.
	for (const TPair<FGuid, FGridRuntimeItemState>& Pair : State->Items)
	{
		const FGridRuntimeItemState& Item = Pair.Value;
		if (!Item.bEmbeddedProjectile || Item.ItemDefinitionId.IsNone())
		{
			continue;
		}
		// 06.3BC: stored embedded arrows can have a legacy pointer from an
		// older SaveGame, or none at all. An indexed ItemId must resolve as
		// real N0 ammunition before any projectile or world pickup is made.
		const bool bCanonicalItem = RestoreRegistry &&
			RestoreRegistry->HasCanonicalItem(Item.ItemDefinitionId);
		FGridResolvedItem ResolvedAmmo;
		const bool bCanonicalAmmo = bCanonicalItem &&
			Item.ItemLevel == 0 &&
			RestoreRegistry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, ResolvedAmmo) &&
			!ResolvedAmmo.bIsArmament && !ResolvedAmmo.AmmoKindId.IsNone() &&
			ResolvedAmmo.MaxStack > 1;
		if (bCanonicalItem && !bCanonicalAmmo)
		{
			UE_LOG(LogGridLevelRuntime, Warning,
				TEXT("[GridProjectile] RestoreEmbeddedRejected Item=%s Level=%d Reason=InvalidCanonicalAmmo"),
				*Item.ItemDefinitionId.ToString(), Item.ItemLevel);
			continue;
		}
		UGridItemDefinitionAsset* Definition = nullptr;
#if WITH_DEV_AUTOMATION_TESTS
		if (!bRequireCanonicalRestore && !bCanonicalAmmo)
			Definition = Item.ProjectileDefinitionAsset.Get();
#endif
		FGridItemInstance Unit;
		Unit.RuntimeObjectId = Pair.Key;
		Unit.ItemDefinitionId = Item.ItemDefinitionId;
		Unit.ItemLevel = Item.ItemLevel;
		Unit.Quantity = 1;
		Unit.Weight = bCanonicalAmmo ? ResolvedAmmo.WeightKg :
			(IsValid(Definition) ? Definition->Weight : 0.0f);
		Unit.OwnerType = EGridItemOwnerType::World;
		Unit.OwnerCharacterIndex = INDEX_NONE;
		Unit.bLightsEnabled = Item.bLightsEnabled;
		const FTransform SavedMesh = Item.bHasWorldMeshTransform ? Item.WorldMeshTransform : Item.Transform;
		bool bSuccess = false;
		bool bAttached = false;
		if (const TArray<AGridMonsterActor*>* Matches =
			MonstersByPersistenceId.Find(Item.EmbeddedMonsterPersistenceId))
		{
			if (Matches->Num() == 1 && IsValid((*Matches)[0]) &&
				!(*Matches)[0]->IsDead() && (bCanonicalAmmo || IsValid(Definition)))
			{
				AGridThrownItemActor* Arrow = SpawnThrownItemProjectile(Unit, Definition,
					SavedMesh.GetLocation(), SavedMesh.GetRotation().GetForwardVector() * 1200.0f,
					Item.CellX, Item.CellY, true);
				if (IsValid(Arrow))
				{
					bAttached = Arrow->RestoreEmbeddedAttachment((*Matches)[0],
						Item.EmbeddedBone, Item.EmbeddedBoneRelativeTransform);
					bSuccess = bAttached;
					if (!bAttached)
					{
						Arrow->Destroy();
					}
				}
			}
		}
		if (!bSuccess)
		{
			bSuccess = TryDropItemInstanceAtWorldTransform(Unit, Definition, SavedMesh);
		}
		UE_LOG(LogGridLevelRuntime, Log,
			TEXT("[GridProjectile] LoadArrow Id=%s Reattached=%d Success=%d MonsterId=%s"),
			*Pair.Key.ToString(), bAttached ? 1 : 0, bSuccess ? 1 : 0,
			*Item.EmbeddedMonsterPersistenceId.ToString());
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

	UE_LOG(LogGridLevelRuntime, Verbose, TEXT("GridRuntimeState Apply Level=%s Doors=%d RemovedObjects=%d Items=%d Receptacles=%d Interactives=%d Monsters=%d DeadMonsters=%d"),
		*State->LevelId.ToString(), State->Doors.Num(), PersistenceCountRemovedRuntimeObjects(State), State->Items.Num(), State->Receptacles.Num(),
		State->InteractiveObjects.Num(), State->Monsters.Num(), DeadMonsterCount);

	ApplyPendingInboundItemsForCurrentLevel();
    RestoreDungeonTrapRevealVisuals(); // Save/Continue: restore materialized known traps.
	RefreshOpenPitItemMonitoring();
	RefreshCombatOilSurfaceVisuals();
	return true;
}
