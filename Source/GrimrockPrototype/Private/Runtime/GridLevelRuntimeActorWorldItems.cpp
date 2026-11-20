#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPlacementTransformResolver.h"

#include "Core/GridDirectionUtils.h"
#include "Runtime/Combat/GridProjectileGeometry.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "PhysicsEngine/BodyInstance.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridItemWeightReadModel.h"
#include "GridWorldItemLevelReadModel.h"
#include "GridWorldPickupIdentityReadModel.h"
#include "GridLegacyItemLookup.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridLevelRuntime, Log, All);

namespace
{
	// Build one physical instance for pickup and pit transfer. Production
	// never invents a replacement GUID or repairs a stack during transfer.
	bool TryBuildPhysicalWorldInstance(const FGridSpawnedItemRuntimeEntry& Entry, FGridItemInstance& Out)
	{
		const AGridItemActor* Actor = Entry.ItemActor.Get();
		if (!IsValid(Actor)) return false;
		const UWorld* World = Actor->GetWorld();
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		const UGridItemSubsystem* Registry = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
		bool bRequireCanonical = true;
#if WITH_DEV_AUTOMATION_TESTS
		bRequireCanonical = GI != nullptr;
#endif
		FGridItemInstance Candidate;
		Candidate.ItemLevel = Entry.ItemLevel;
		if (bRequireCanonical)
		{
			FGridResolvedItem Resolved;
			if (!Registry || !Registry->IsConfigured() ||
				!Entry.ObjectId.IsValid() || Actor->GetRuntimeObjectId() != Entry.ObjectId ||
				Entry.ItemDefinitionId.IsNone() || Actor->ItemDefinitionId != Entry.ItemDefinitionId ||
				Actor->CanonicalWorldItemLevel != Entry.ItemLevel ||
				!Registry->ResolveItem(Entry.ItemDefinitionId, Entry.ItemLevel, Resolved) ||
				!Resolved.IsValid() || Resolved.ItemId != Entry.ItemDefinitionId ||
				Resolved.ItemLevel != Entry.ItemLevel ||
				(Resolved.bIsArmament ? Entry.ItemLevel < 1 : Entry.ItemLevel != 0) ||
				Entry.Quantity < 1 || Entry.Quantity > FMath::Max(1, Resolved.MaxStack))
				return false;
			Candidate.RuntimeObjectId = Entry.ObjectId;
			Candidate.ItemDefinitionId = Entry.ItemDefinitionId;
			Candidate.Quantity = Entry.Quantity;
			Candidate.Weight = Resolved.WeightKg;
		}
		else
		{
			// Temporary registry-less automation fixture compatibility.
			Candidate.ItemDefinitionId = FGridWorldPickupIdentityReadModel::Resolve(Actor, Entry.ItemDefinitionId, Entry.ItemLevel);
			Candidate.RuntimeObjectId = Actor->GetRuntimeObjectId();
			if (!Candidate.RuntimeObjectId.IsValid()) Candidate.RuntimeObjectId = FGuid::NewGuid();
			Candidate.Quantity = FMath::Max(1, Entry.Quantity);
		}
		Candidate.bLightsEnabled = Actor->AreItemLightsEnabled();
		Candidate.ReadableContentAsset = Actor->ReadableContentAsset;
		Candidate.ReadableContentId = Actor->ReadableContentId;
		Candidate.ReadTitleOverride = Actor->ReadTitleOverride;
		Candidate.ReadTextOverride = Actor->ReadTextOverride;
		Candidate.LastWorldTransform = IsValid(Actor->MeshComponent)
			? Actor->MeshComponent->GetComponentTransform() : Actor->GetActorTransform();
		if (!Candidate.IsValid()) return false;
		Out = MoveTemp(Candidate);
		return true;
	}

	FString GetWorldItemEdgeText(EGridEdge Edge)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			return EdgeEnum->GetNameStringByValue(static_cast<int64>(Edge));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Edge));
	}

	const AGrimrockPartyPawn* ResolveWorldItemInteractionParty(const AGridLevelRuntimeActor* RuntimeActor)
	{
		if (!RuntimeActor || !RuntimeActor->GetWorld())
		{
			return nullptr;
		}

		for (TActorIterator<AGrimrockPartyPawn> It(RuntimeActor->GetWorld()); It; ++It)
		{
			const AGrimrockPartyPawn* PartyPawn = *It;
			if (IsValid(PartyPawn) && PartyPawn->LevelRuntimeActor == RuntimeActor)
			{
				return PartyPawn;
			}
		}
		return nullptr;
	}

	bool IsWithinWorldItemHandReach(
		const AGridLevelRuntimeActor* RuntimeActor, const AGrimrockPartyPawn* PartyPawn, int32 CellX, int32 CellY, const FVector& LocalOffset)
	{
		if (!RuntimeActor || !PartyPawn)
		{
			return false;
		}

		const float Reach = FMath::Max(0.0f, RuntimeActor->WorldItemPickupReach);
		const FVector TargetWorldLocation = RuntimeActor->GetCellCenterWorld(CellX, CellY, PartyPawn->GetActorLocation().Z) +
			FVector(LocalOffset.X, LocalOffset.Y, 0.0f);
		return FVector::DistSquared2D(PartyPawn->GetActorLocation(), TargetWorldLocation) <= FMath::Square(Reach);
	}
}

bool AGridLevelRuntimeActor::CanPartyPickupItemEntry(const FGridSpawnedItemRuntimeEntry& Entry, const AGrimrockPartyPawn* PartyPawn, bool bLogRejection) const
{
	if (!PartyPawn)
	{
		if (bLogRejection)
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("Grid item pickup rejected: missing party pawn."));
		}
		return false;
	}

	if (PartyPawn->LevelRuntimeActor != this)
	{
		if (bLogRejection)
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("Grid item pickup rejected: party runtime actor does not match item runtime actor. ItemCell=(%d,%d)."), Entry.Cell.X,
				Entry.Cell.Y);
		}
		return false;
	}

	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);

	// Free floor pickups use pure horizontal hand reach. Physical obstruction is owned by the
	// mouse Visibility hit, so a grating opening may be reachable even while movement is blocked.
	if (Entry.Edge == EGridEdge::None)
	{
		const AGridItemActor* ItemActor = Entry.ItemActor.Get();
		if (!IsValid(ItemActor))
		{
			if (bLogRejection)
			{
				UE_LOG(LogGridLevelRuntime, Warning, TEXT("Grid item pickup rejected: free pickup actor is missing. ItemCell=(%d,%d)."), Entry.Cell.X, Entry.Cell.Y);
			}
			return false;
		}

		const float PickupReach = FMath::Max(0.0f, WorldItemPickupReach);
		// A rolled weapon's handle pivot can remain out of reach while its
		// visible collision shape is within reach. The mouse still owns occlusion.
		const FVector PickupWorldLocation = ItemActor->GetClosestWorldPickupPoint(PartyPawn->GetActorLocation());
		const float DistanceSquared = FVector::DistSquared2D(PartyPawn->GetActorLocation(), PickupWorldLocation);
		if (DistanceSquared > FMath::Square(PickupReach))
		{
			if (bLogRejection)
			{
				UE_LOG(LogGridLevelRuntime, Warning,
					TEXT("Grid item pickup rejected: free pickup is out of reach. PartyCell=(%d,%d) ItemCell=(%d,%d) Distance=%.1f Reach=%.1f."),
					PartyCell.X, PartyCell.Y, Entry.Cell.X, Entry.Cell.Y, FMath::Sqrt(DistanceSquared), PickupReach);
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
			UE_LOG(LogGridLevelRuntime, Warning,
				TEXT("Grid item pickup rejected: item in party cell is not on the edge currently faced by the party. PartyCell=(%d,%d) Facing=%s ItemEdge=%s."),
				PartyCell.X, PartyCell.Y, *GetWorldItemEdgeText(PartyPawn->Facing), *GetWorldItemEdgeText(Entry.Edge));
		}
		return bFacesItemEdge;
	}

	if (PartyPawn->Facing == EGridEdge::None)
	{
		if (bLogRejection)
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("Grid item pickup rejected: party facing is None. PartyCell=(%d,%d) ItemCell=(%d,%d) ItemEdge=%s."), PartyCell.X,
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
			UE_LOG(LogGridLevelRuntime, Warning,
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
			UE_LOG(LogGridLevelRuntime, Warning,
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
	RefreshWorldItemPhysicsCells();
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

		FGridItemInstance ItemInstance;
		if (!TryBuildPhysicalWorldInstance(Entry, ItemInstance))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("Item pickup refused: contradictory or invalid physical identity."));
			return false;
		}
		const FName ItemDefinitionId = ItemInstance.ItemDefinitionId;

		if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory(ItemInstance))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"), *ItemDefinitionId.ToString(),
				*ItemInstance.RuntimeObjectId.ToString());
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

		UE_LOG(LogGridLevelRuntime, Verbose, TEXT("Picked up item %s from cell %d,%d."), *ItemDefinitionId.ToString(), CellX, CellY);
		return true;
	}

	return false;
}

bool AGridLevelRuntimeActor::TryPickupItemActor(AGridItemActor* ItemActor, AGrimrockPartyPawn* PartyPawn)
{
	RefreshWorldItemPhysicsCells();
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

		FGridItemInstance ItemInstance;
		if (!TryBuildPhysicalWorldInstance(Entry, ItemInstance))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("Item pickup refused: contradictory or invalid physical identity."));
			return false;
		}
		const FName ItemDefinitionId = ItemInstance.ItemDefinitionId;

		if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory(ItemInstance))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"), *ItemDefinitionId.ToString(),
				*ItemInstance.RuntimeObjectId.ToString());
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

		UE_LOG(LogGridLevelRuntime, Verbose, TEXT("Picked up item %s from clicked actor at cell %d,%d."), *ItemDefinitionId.ToString(), PickedCell.X, PickedCell.Y);
		return true;
	}

	return false;
}

bool AGridLevelRuntimeActor::TryRouteWorldItemThroughOpenPit(
	const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinitionAsset, int32 CellX, int32 CellY, const FVector& LocalOffset)
{
	FGridRelocationBehaviorParams Relocation;
	// A registered fixed-level item may travel between dungeon levels
	// using only its ItemId: pending inbound state already persists that ID.
	const UWorld* RouteWorld = GetWorld();
	UGameInstance* RouteGI = RouteWorld ? RouteWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* RouteRegistry = RouteGI ? RouteGI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridResolvedItem RouteCanonical;
	const bool bCanonicalIdentity = RouteRegistry &&
		RouteRegistry->HasCanonicalItem(ItemInstance.ItemDefinitionId);
	bool bRequireCanonical = true;
#if WITH_DEV_AUTOMATION_TESTS
	bRequireCanonical = RouteGI != nullptr;
#endif
	const bool bCanonicalRoute = bCanonicalIdentity &&
		RouteRegistry->ResolveItem(ItemInstance.ItemDefinitionId, ItemInstance.ItemLevel, RouteCanonical) &&
		RouteCanonical.IsValid() && RouteCanonical.ItemId == ItemInstance.ItemDefinitionId &&
		RouteCanonical.ItemLevel == ItemInstance.ItemLevel &&
		(RouteCanonical.bIsArmament ? ItemInstance.ItemLevel >= 1 : ItemInstance.ItemLevel == 0) &&
		ItemInstance.Quantity >= 1 && ItemInstance.Quantity <= FMath::Max(1, RouteCanonical.MaxStack);
	// A known canonical ItemId at an invalid persisted level (notably ARM N0)
	// must never fall back to an old UGridItemDefinitionAsset.
	if ((bRequireCanonical || bCanonicalIdentity) && !bCanonicalRoute)
	{
		UE_LOG(LogGridLevelRuntime, Warning,
			TEXT("GridPit ItemTransfer rejected Item=%s Level=%d Reason=InvalidCanonicalItem"),
			*ItemInstance.ItemDefinitionId.ToString(), ItemInstance.ItemLevel);
		return false;
	}
	if (!DungeonAsset || !ItemInstance.IsValid() ||
		(!bCanonicalRoute && !IsValid(ItemDefinitionAsset)) ||
		!FindOpenPitAtCell(CellX, CellY, Relocation))
	{
		return false;
	}

	if (Relocation.TargetLevelId == CurrentDungeonLevelId && CanRelocateThroughPitOnCurrentLevel(Relocation, CellX, CellY) && !bIsRoutingLocalPitItem)
	{
		TGuardValue<bool> LocalGuard(bIsRoutingLocalPitItem, true);
		const float MaxOffset = LevelAsset->CellSize * 0.35f;
		const FVector Target = GetCellCenterWorld(Relocation.TargetCellX, Relocation.TargetCellY, 12.f) +
			FVector(FMath::Clamp(LocalOffset.X, -MaxOffset, MaxOffset), FMath::Clamp(LocalOffset.Y, -MaxOffset, MaxOffset), 0.f);
		FGridSpawnedItemRuntimeEntry* Existing = SpawnedItemEntries.FindByPredicate(
			[&ItemInstance](const FGridSpawnedItemRuntimeEntry& Entry)
			{
				return Entry.ObjectId == ItemInstance.RuntimeObjectId;
			});
		if (Existing)
		{
			AGridItemActor* Actor = Existing->ItemActor.Get();
			if (!IsValid(Actor) || !IsValid(Actor->MeshComponent))
				return false;
			if (bRequireCanonical)
			{
				FGridItemInstance Physical;
				if (!TryBuildPhysicalWorldInstance(*Existing, Physical) ||
					Physical.ItemDefinitionId != ItemInstance.ItemDefinitionId ||
					Physical.ItemLevel != ItemInstance.ItemLevel || Physical.Quantity != ItemInstance.Quantity)
					return false;
			}
			const FVector Center = Actor->GetWorldPickupLocation();
			// A physical passage is already below the source floor. Bring the
			// entire shape above the destination floor, preserving its rotation.
			const FVector Delta(Target.X - Center.X, Target.Y - Center.Y, Target.Z - Actor->MeshComponent->Bounds.GetBox().Min.Z);
			FTransform MeshPose = Actor->MeshComponent->GetComponentTransform();
			MeshPose.AddToTranslation(Delta);
			Existing->Cell = FIntPoint(Relocation.TargetCellX, Relocation.TargetCellY);
			Actor->SetActorLocation(Actor->GetActorLocation() + Delta, false, nullptr, ETeleportType::TeleportPhysics);
			Actor->MeshComponent->SetWorldTransform(MeshPose, false, nullptr, ETeleportType::TeleportPhysics);
			Actor->MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Actor->MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			Actor->SetRuntimeCell(Relocation.TargetCellX, Relocation.TargetCellY);
			if (ActivationComponent)
			{
				ActivationComponent->RefreshPressurePlatesAtCell(CellX, CellY);
				ActivationComponent->RefreshPressurePlatesAtCell(Relocation.TargetCellX, Relocation.TargetCellY);
			}
		}
		else
		{
			FGridItemInstance LocalItem = ItemInstance;
			LocalItem.OwnerType = EGridItemOwnerType::World;
			if (!TryDropItemInstanceAtCell(LocalItem, ItemDefinitionAsset, Relocation.TargetCellX, Relocation.TargetCellY, EGridEdge::None,
					Target - GetCellCenterWorld(Relocation.TargetCellX, Relocation.TargetCellY, 12.f)))
				return false;
		}
		UE_LOG(LogGridLevelRuntime, Log, TEXT("GridPit ItemTransfer local Item=%s RuntimeId=%s Source=(%d,%d) Target=(%d,%d) Level=%s"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), CellX, CellY, Relocation.TargetCellX, Relocation.TargetCellY,
			*CurrentDungeonLevelId.ToString());
		return true;
	}
	if (Relocation.TargetLevelId.IsNone() || Relocation.TargetLevelId == CurrentDungeonLevelId)
	{
		UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridPit ItemTransfer rejected Item=%s RuntimeId=%s Source=(%d,%d) Reason=InvalidTargetLevel Target=%s"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), CellX, CellY, *Relocation.TargetLevelId.ToString());
		return false;
	}

	const FGridDungeonLevelEntry* TargetEntry = DungeonAsset->FindLevelEntry(Relocation.TargetLevelId);
	UGridLevelAsset* TargetLevelAsset = TargetEntry && TargetEntry->bEnabled ? TargetEntry->LevelAsset.Get() : nullptr;
	if (!TargetLevelAsset)
	{
		UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridPit ItemTransfer rejected Item=%s RuntimeId=%s Source=(%d,%d) Reason=InvalidTargetLevelAsset TargetLevel=%s"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), CellX, CellY, *Relocation.TargetLevelId.ToString());
		return false;
	}

	const int32 PreferredTargetX = Relocation.TargetCellX;
	const int32 PreferredTargetY = Relocation.TargetCellY;
	const bool bPreferredWalkable = TargetLevelAsset->IsValidCoord(PreferredTargetX, PreferredTargetY) &&
		TargetLevelAsset->GetCell(PreferredTargetX, PreferredTargetY).CellType != EGridCellType::Empty &&
		!TargetLevelAsset->GetCell(PreferredTargetX, PreferredTargetY).bBlocksOccupancy;
	const bool bPreferredContainsOpenPit = bPreferredWalkable && TargetLevelAsset->WorldObjectInstances.ContainsByPredicate(
		[this, &Relocation, PreferredTargetX, PreferredTargetY](const FGridWorldObjectInstance& Candidate)
		{
			return Candidate.CellX == PreferredTargetX && Candidate.CellY == PreferredTargetY && IsEffectivePitObject(Candidate) &&
				IsPitOpenForLevel(Relocation.TargetLevelId, Candidate);
		});
	if (bPreferredContainsOpenPit)
	{
		UE_LOG(LogGridLevelRuntime, Warning,
			TEXT("GridPit ItemTransfer rejected Item=%s RuntimeId=%s Reason=ChainedPitNotSupported TargetLevel=%s Target=(%d,%d)"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), *Relocation.TargetLevelId.ToString(),
			PreferredTargetX, PreferredTargetY);
		return false;
	}

	int32 LandingCellX = INDEX_NONE;
	int32 LandingCellY = INDEX_NONE;
	if (!ResolvePitLandingCell(Relocation.TargetLevelId, PreferredTargetX, PreferredTargetY, LandingCellX, LandingCellY))
	{
		UE_LOG(LogGridLevelRuntime, Warning,
			TEXT("GridPit ItemTransfer rejected Item=%s RuntimeId=%s Reason=NoUsableLandingCell TargetLevel=%s Requested=(%d,%d)"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), *Relocation.TargetLevelId.ToString(),
			PreferredTargetX, PreferredTargetY);
		return false;
	}

	if (LandingCellX != PreferredTargetX || LandingCellY != PreferredTargetY)
	{
		UE_LOG(LogGridLevelRuntime, Warning,
			TEXT("GridPit ItemTransfer landing fallback Item=%s RuntimeId=%s TargetLevel=%s Requested=(%d,%d) Resolved=(%d,%d)"),
			*ItemInstance.ItemDefinitionId.ToString(), *ItemInstance.RuntimeObjectId.ToString(), *Relocation.TargetLevelId.ToString(),
			PreferredTargetX, PreferredTargetY, LandingCellX, LandingCellY);
		Relocation.TargetCellX = LandingCellX;
		Relocation.TargetCellY = LandingCellY;
	}

	const float TargetCellSize = FMath::Max(1.0f, TargetLevelAsset->CellSize);
	const float MaxOffset = TargetCellSize * 0.35f;
	const FVector ClampedOffset(
		FMath::Clamp(LocalOffset.X, -MaxOffset, MaxOffset),
		FMath::Clamp(LocalOffset.Y, -MaxOffset, MaxOffset),
		0.0f);
	const FVector TargetWorldLocation = GetActorLocation() + GridOrigin +
		FVector((Relocation.TargetCellX + 0.5f) * TargetCellSize, (Relocation.TargetCellY + 0.5f) * TargetCellSize, 12.0f) + ClampedOffset;

	FGridRuntimeItemState ItemState;
	ItemState.ObjectId = ItemInstance.RuntimeObjectId;
	ItemState.ItemDefinitionId = ItemInstance.ItemDefinitionId;
	ItemState.ItemLevel = ItemInstance.ItemLevel;
	ItemState.Quantity = FMath::Max(1, ItemInstance.Quantity);
	ItemState.CellX = Relocation.TargetCellX;
	ItemState.CellY = Relocation.TargetCellY;
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
	// The physical ItemId/N-level is the sole source of world presentation
	// for every registered family. Never serialize DA_Weapon_* along with
	// a valid canonical ARM item crossing a pit.
	PendingState.ItemDefinitionAsset = bCanonicalRoute ? nullptr : ItemDefinitionAsset;

	// A pending landing must not replace any already-owned destination row.
	if (const FGridLevelRuntimeState* Previous = DungeonRuntimeState.LevelStates.Find(Relocation.TargetLevelId))
	{
		if (Previous->Items.Contains(ItemState.ObjectId) || Previous->PendingInboundItems.Contains(ItemState.ObjectId))
			return false;
		for (const auto& Pair : Previous->Receptacles)
		{
			if (Pair.Value.ContainedItems.ContainsByPredicate([&ItemState](const FGridRuntimeItemState& Contained)
				{ return Contained.ObjectId == ItemState.ObjectId; }))
				return false;
		}
	}
	FGridLevelRuntimeState& TargetState = DungeonRuntimeState.LevelStates.FindOrAdd(Relocation.TargetLevelId);
	TargetState.LevelId = Relocation.TargetLevelId;
	TargetState.PendingInboundItems.Add(ItemState.ObjectId, PendingState);

	UE_LOG(LogGridLevelRuntime, Log,
		TEXT("GridPit ItemTransfer queued Item=%s RuntimeId=%s SourceLevel=%s Source=(%d,%d) TargetLevel=%s Target=(%d,%d) Offset=%s"),
		*ItemState.ItemDefinitionId.ToString(), *ItemState.ObjectId.ToString(), *CurrentDungeonLevelId.ToString(), CellX, CellY,
		*Relocation.TargetLevelId.ToString(), Relocation.TargetCellX, Relocation.TargetCellY, *ClampedOffset.ToCompactString());
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
		if ((CurrentDungeonLevelId.IsNone() ? FName(TEXT("SingleLevel")) : CurrentDungeonLevelId) != RuntimeLevelId)
			break; // A prior notification may have changed the current level.
		const FGridLevelRuntimeState* ReadState = DungeonRuntimeState.LevelStates.Find(RuntimeLevelId);
		const FGridPendingWorldItemState* Pending = ReadState ? ReadState->PendingInboundItems.Find(RuntimeObjectId) : nullptr;
		if (!Pending)
		{
			continue;
		}

		const FGridPendingWorldItemState PendingCopy = *Pending;
		const FGridRuntimeItemState& ItemState = PendingCopy.ItemState;
		if (!RuntimeObjectId.IsValid() || RuntimeObjectId != ItemState.ObjectId ||
			ItemState.Quantity < 1 || ItemState.bIsContainedInReceptacle ||
			ItemState.ReceptacleObjectId.IsValid() || ItemState.bEmbeddedProjectile ||
			!LevelAsset->IsValidCoord(ItemState.CellX, ItemState.CellY))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridPit PendingItem apply skipped RuntimeId=%s Reason=InvalidCell Cell=(%d,%d)"),
				*RuntimeObjectId.ToString(), ItemState.CellX, ItemState.CellY);
			continue;
		}

		FGridItemInstance ItemInstance;
		ItemInstance.RuntimeObjectId = RuntimeObjectId;
		ItemInstance.ItemDefinitionId = ItemState.ItemDefinitionId;
		ItemInstance.ItemLevel = ItemState.ItemLevel;
		ItemInstance.Quantity = ItemState.Quantity;
		ItemInstance.bLightsEnabled = ItemState.bLightsEnabled;
		ItemInstance.ReadableContentAsset = ItemState.ReadableContentAsset;
		ItemInstance.ReadableContentId = ItemState.ReadableContentId;
		ItemInstance.ReadTitleOverride = ItemState.ReadTitleOverride;
		ItemInstance.ReadTextOverride = ItemState.ReadTextOverride;
		ItemInstance.LastWorldTransform = ItemState.Transform;

		const FVector CellCenter = GetCellCenterWorld(ItemState.CellX, ItemState.CellY, 12.0f);
		FVector LocalOffset = ItemState.Transform.GetLocation() - CellCenter;
		LocalOffset.Z = 0.0f;

		const UWorld* InboundWorld = GetWorld();
		UGameInstance* InboundGI = InboundWorld ? InboundWorld->GetGameInstance() : nullptr;
		const UGridItemSubsystem* InboundRegistry =
			InboundGI ? InboundGI->GetSubsystem<UGridItemSubsystem>() : nullptr;
		const bool bKnownCanonical = InboundRegistry &&
			InboundRegistry->HasCanonicalItem(ItemState.ItemDefinitionId);
		FGridResolvedItem InboundCanonical;
		if (bKnownCanonical &&
			!InboundRegistry->ResolveItem(ItemState.ItemDefinitionId, ItemState.ItemLevel, InboundCanonical))
		{
			UE_LOG(LogGridLevelRuntime, Warning,
				TEXT("GridPit PendingItem rejected Item=%s Level=%d Reason=InvalidCanonicalItem"),
				*ItemState.ItemDefinitionId.ToString(), ItemState.ItemLevel);
			continue; // Retain the queued snapshot for diagnostics, no legacy fallback.
		}
		// Every canonical ItemId (ARM N1-N20 included) lands by registry
		// alone. Ignore stale pointers even when present in a v27 snapshot.
		UGridItemDefinitionAsset* Definition = bKnownCanonical
			? nullptr : PendingCopy.ItemDefinitionAsset.Get();

		if (!TryDropItemInstanceAtCellInternal(ItemInstance, Definition, ItemState.CellX, ItemState.CellY, EGridEdge::None, LocalOffset, false))
		{
			UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridPit PendingItem apply failed RuntimeId=%s Item=%s Level=%s Cell=(%d,%d)"),
				*RuntimeObjectId.ToString(), *ItemState.ItemDefinitionId.ToString(), *RuntimeLevelId.ToString(), ItemState.CellX, ItemState.CellY);
			continue;
		}

		const FGridSpawnedItemRuntimeEntry* Spawned = SpawnedItemEntries.FindByPredicate(
			[RuntimeObjectId](const FGridSpawnedItemRuntimeEntry& Entry)
			{
				return Entry.ObjectId == RuntimeObjectId && IsValid(Entry.ItemActor.Get());
			});
		// This diagnostic distinguishes lost transfers, wrong-cell spawns and
		// actors with an unrenderable mesh in a real PIE/Chaos environment.
		if (Spawned && IsValid(Spawned->ItemActor.Get()))
		{
			const AGridItemActor* Actor = Spawned->ItemActor.Get();
			const FVector ActorMeshWorld = IsValid(Actor->MeshComponent)
				? Actor->MeshComponent->GetComponentLocation() : Actor->GetActorLocation();
			UE_LOG(LogGridLevelRuntime, Log,
				TEXT("GridPit PendingItem materialized Item=%s N%d RuntimeId=%s Level=%s ExpectedCell=(%d,%d) EntryCell=(%d,%d) Root=%s Mesh=%s HasMesh=%d"),
				*ItemState.ItemDefinitionId.ToString(), ItemState.ItemLevel, *RuntimeObjectId.ToString(),
				*RuntimeLevelId.ToString(), ItemState.CellX, ItemState.CellY,
				Spawned->Cell.X, Spawned->Cell.Y,
				*Actor->GetActorLocation().ToCompactString(), *ActorMeshWorld.ToCompactString(),
				IsValid(Actor->MeshComponent) && Actor->MeshComponent->GetStaticMesh() ? 1 : 0);
		}
		else
		{
			UE_LOG(LogGridLevelRuntime, Error,
				TEXT("GridPit PendingItem spawn bookkeeping missing Item=%s RuntimeId=%s Level=%s"),
				*ItemState.ItemDefinitionId.ToString(), *RuntimeObjectId.ToString(), *RuntimeLevelId.ToString());
			continue; // Do not acknowledge an arrival that has no physical row.
		}
		if (FGridLevelRuntimeState* MutableState = DungeonRuntimeState.LevelStates.Find(RuntimeLevelId))
		{
			MutableState->PendingInboundItems.Remove(RuntimeObjectId);
		}
		++AppliedCount;
		// Ownership is now exclusively in the live world. Publish links afterwards.
		if (ActivationComponent)
		{
			ActivationComponent->RefreshPressurePlatesAtCell(ItemState.CellX, ItemState.CellY);
		}
	}

	if (AppliedCount > 0)
	{
		UE_LOG(LogGridLevelRuntime, Verbose, TEXT("GridPit PendingItem apply Level=%s Applied=%d"), *RuntimeLevelId.ToString(), AppliedCount);
	}
	return AppliedCount;
}

int32 AGridLevelRuntimeActor::DropWorldItemsThroughOpenPitAtCell(int32 CellX, int32 CellY)
{
	if (!LevelAsset)
		return 0;
	const FIntPoint PitCell(CellX, CellY);
	int32 DroppedCount = 0;
	for (int32 EntryIndex = SpawnedItemEntries.Num() - 1; EntryIndex >= 0; --EntryIndex)
	{
		if (!SpawnedItemEntries.IsValidIndex(EntryIndex))
		{
			continue;
		}

		const FGridSpawnedItemRuntimeEntry Entry = SpawnedItemEntries[EntryIndex];
		AGridItemActor* ItemActor = Entry.ItemActor.Get();
		if (Entry.Edge != EGridEdge::None || !IsValid(ItemActor))
		{
			continue;
		}
		const UStaticMeshComponent* Mesh = ItemActor->MeshComponent;
		if (IsValid(Mesh) && Mesh->IsSimulatingPhysics() && Mesh->GetBodyInstance() && Mesh->GetBodyInstance()->IsValidBodyInstance())
			continue; // A supported overhang is not a fall. The open-pit monitor waits for physical passage.
		FIntPoint PhysicsCell = Entry.Cell;
		if (IsValid(ItemActor->MeshComponent))
		{
			int32 MeshCellX = INDEX_NONE, MeshCellY = INDEX_NONE;
			FVector MeshLocalOffset;
			if (TryResolveWorldCellFromImpactPoint(ItemActor->GetWorldPickupLocation(), MeshCellX, MeshCellY, MeshLocalOffset))
			{
				PhysicsCell = FIntPoint(MeshCellX, MeshCellY);
			}
		}
		if (Entry.Cell != PitCell && PhysicsCell != PitCell)
		{
			continue;
		}
		DroppedCount += TransferWorldItemThroughPit(ItemActor, CellX, CellY) ? 1 : 0;
	}
	return DroppedCount;
}

bool AGridLevelRuntimeActor::TransferWorldItemThroughPit(AGridItemActor* ItemActor, int32 CellX, int32 CellY)
{
	FGridRelocationBehaviorParams Relocation;
	if (!IsValid(ItemActor) || !FindOpenPitAtCell(CellX, CellY, Relocation))
		return false;
	const int32 EntryIndex = SpawnedItemEntries.IndexOfByPredicate(
		[ItemActor](const FGridSpawnedItemRuntimeEntry& Candidate)
		{
			return Candidate.ItemActor.Get() == ItemActor;
		});
	if (EntryIndex == INDEX_NONE)
		return false;
	const FGridSpawnedItemRuntimeEntry Entry = SpawnedItemEntries[EntryIndex];
	if (Entry.Edge != EGridEdge::None || !Entry.ObjectId.IsValid() || Entry.ObjectId != ItemActor->GetRuntimeObjectId())
		return false;

	FGridItemInstance ItemInstance;
	if (!TryBuildPhysicalWorldInstance(Entry, ItemInstance))
		return false;
	const FName ItemId = ItemInstance.ItemDefinitionId;
	const UWorld* RouteWorld = GetWorld();
	UGameInstance* RouteGI = RouteWorld ? RouteWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = RouteGI ? RouteGI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	UGridItemDefinitionAsset* Definition = Registry && Registry->HasCanonicalItem(ItemId)
		? nullptr : Entry.ItemDefinitionAsset.Get();

	// With simulating Chaos the mesh may travel while the actor root
	// remains at its original spawn. Transfer its actual horizontal pose.
	const FVector PhysicalLocation = ItemActor->GetWorldPickupLocation();
	FVector LocalOffset = PhysicalLocation - GetCellCenterWorld(CellX, CellY, 12.0f);
	LocalOffset.Z = 0.0f;
	if (!TryRouteWorldItemThroughOpenPit(ItemInstance, Definition, CellX, CellY, LocalOffset))
	{
		UE_LOG(LogGridLevelRuntime, Warning, TEXT("GridPit WorldItem remained on upper level: Item=%s N%d RuntimeId=%s Cell=(%d,%d)"), *ItemId.ToString(),
			Entry.ItemLevel, *ItemInstance.RuntimeObjectId.ToString(), CellX, CellY);
		return false;
	}

	if (Relocation.TargetLevelId == CurrentDungeonLevelId)
	{
		return true; // Existing actor moved in place; no orphaned inbound queue or destruction.
	}

	ItemActor->OnRemovedFromWorld();
	ItemActor->Destroy();
	SpawnedItemActors.RemoveAllSwap(
		[ItemActor](const TObjectPtr<AGridItemActor>& Candidate)
		{
			return Candidate.Get() == ItemActor;
		});
	SpawnedItemEntries.RemoveAtSwap(EntryIndex);
	if (ActivationComponent)
	{
		ActivationComponent->RefreshPressurePlatesAtCell(CellX, CellY);
	}

	return true;
}

void AGridLevelRuntimeActor::RefreshOpenPitItemMonitoring()
{
	OpenPitItemCells.Reset();
	UWorld* World = GetWorld();
	if (!World)
		return;
	if (World->IsGameWorld() && LevelAsset)
	{
		for (const FGridWorldObjectInstance& Object : LevelAsset->WorldObjectInstances)
			if (IsEffectivePitObject(Object) && IsPitOpen(Object.InstanceId))
				OpenPitItemCells.AddUnique(FIntPoint(Object.CellX, Object.CellY));
	}
	if (OpenPitItemCells.IsEmpty())
		World->GetTimerManager().ClearTimer(OpenPitItemTimer);
	else if (!World->GetTimerManager().IsTimerActive(OpenPitItemTimer))
		World->GetTimerManager().SetTimer(OpenPitItemTimer, this, &AGridLevelRuntimeActor::CheckOpenPitItemPassages, 0.05f, true);
}

void AGridLevelRuntimeActor::CheckOpenPitItemPassages()
{
	UWorld* World = GetWorld();
	if (!World || !LevelAsset)
		return;
	const FName SourceLevel = CurrentDungeonLevelId;
	const TArray<FIntPoint> Cells = OpenPitItemCells;
	TSet<FGuid> Transferred;
	for (const FIntPoint& Cell : Cells)
	{
		if (CurrentDungeonLevelId != SourceLevel)
			return;
		FGridRelocationBehaviorParams Relocation;
		if (!FindOpenPitAtCell(Cell.X, Cell.Y, Relocation))
			continue;
		// Narrow-phase shape overlap below the floor, not a bounds/centre test.
		// The deep passage also catches fast thrown/released bodies between samples.
		const float HalfWidth = FMath::Max(1.f, LevelAsset->CellSize * 0.5f - 1.f);
		const float Depth = FMath::Max(800.f, LevelAsset->CellSize * 4.f);
		const FVector Probe = GetCellCenterWorld(Cell.X, Cell.Y, -20.f - Depth * 0.5f);
		TArray<FOverlapResult> Hits;
		World->OverlapMultiByObjectType(Hits, Probe, FQuat::Identity, FCollisionObjectQueryParams(ECC_PhysicsBody),
			FCollisionShape::MakeBox(FVector(HalfWidth, HalfWidth, Depth * 0.5f)), FCollisionQueryParams(SCENE_QUERY_STAT(GridPitPassage), false, this));
		for (const FOverlapResult& Hit : Hits)
		{
			if (CurrentDungeonLevelId != SourceLevel)
				return;
			AGridItemActor* Item = Cast<AGridItemActor>(Hit.GetActor());
			UStaticMeshComponent* Mesh = IsValid(Item) ? Item->MeshComponent.Get() : nullptr;
			if (!IsValid(Mesh) || Hit.GetComponent() != Mesh || !Mesh->IsSimulatingPhysics() || Mesh->GetPhysicsLinearVelocity().Z >= -10.f ||
				Transferred.Contains(Item->GetRuntimeObjectId()))
				continue;
			const FVector Position = Mesh->GetComponentLocation();
			const FVector Center = Mesh->Bounds.Origin;
			const FVector Velocity = Mesh->GetPhysicsLinearVelocity();
			const FGuid Guid = Item->GetRuntimeObjectId();
			if (TransferWorldItemThroughPit(Item, Cell.X, Cell.Y))
			{
				Transferred.Add(Guid);
				UE_LOG(LogGridLevelRuntime, Log, TEXT("GridPit PhysicalPassage RuntimeId=%s Source=(%d,%d) Mesh=%s Center=%s Velocity=%s"), *Guid.ToString(),
					Cell.X, Cell.Y, *Position.ToCompactString(), *Center.ToCompactString(), *Velocity.ToCompactString());
			}
		}
	}
}

void AGridLevelRuntimeActor::RefreshWorldItemPhysicsCells()
{
	for (FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
	{
		AGridItemActor* Item = Entry.ItemActor.Get();
		if (!IsValid(Item) || !IsValid(Item->MeshComponent) || !Item->MeshComponent->IsSimulatingPhysics())
		{
			continue;
		}
		int32 X = INDEX_NONE, Y = INDEX_NONE;
		FVector LocalOffset;
		if (TryResolveWorldCellFromImpactPoint(Item->GetWorldPickupLocation(), X, Y, LocalOffset) && Entry.Cell != FIntPoint(X, Y))
		{
			Entry.Cell = FIntPoint(X,Y);
			Item->SetRuntimeCell(X,Y);
		}
	}
}

bool AGridLevelRuntimeActor::TryDropItemInstanceAtWorldTransform(
	const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinition, const FTransform& WorldMeshTransform)
{
	if (WorldMeshTransform.ContainsNaN())
	{
		return false;
	}
	int32 CellX = INDEX_NONE, CellY = INDEX_NONE;
	FVector LocalOffset = FVector::ZeroVector;
	if (!TryResolveWorldCellFromImpactPoint(WorldMeshTransform.GetLocation(), CellX, CellY, LocalOffset))
	{
		return false;
	}
	FGridRelocationBehaviorParams PitRelocation;
	const bool bOpenPit = FindOpenPitAtCell(CellX, CellY, PitRelocation);
	if (!TryDropItemInstanceAtCell(ItemInstance, ItemDefinition, CellX, CellY, EGridEdge::None, LocalOffset))
	{
		return false;
	}
	// An open pit routes the same GUID to the lower level instead of spawning
	// a local actor. That is SUCCESS, not a reason to drop the arrow again.
	if (bOpenPit)
	{
		return true;
	}

	const FGuid Id = ItemInstance.RuntimeObjectId;
	for (FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
	{
		if (Entry.ObjectId != Id)
		{
			continue;
		}
		AGridItemActor* Item = Entry.ItemActor.Get();
		if (IsValid(Item) && IsValid(Item->MeshComponent))
		{
			// The mesh owns the Chaos rigid body; the scene-root actor transform
			// does not follow simulated physics. Preserve the real impact mesh
			// rotation and location while keeping the pickup mesh's authored scale.
			Item->MeshComponent->SetWorldLocationAndRotation(WorldMeshTransform.GetLocation(),
				WorldMeshTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
			Item->MeshComponent->WakeRigidBody();
			UE_LOG(LogGridLevelRuntime, Log,
				TEXT("[GridProjectile] PhysicalRelease Item=%s RuntimeId=%s Cell=(%d,%d) Mesh=%s Physics=%d"),
				*ItemInstance.ItemDefinitionId.ToString(), *Id.ToString(), CellX, CellY,
				*Item->MeshComponent->GetComponentLocation().ToCompactString(),
				Item->MeshComponent->IsSimulatingPhysics() ? 1 : 0);
			return true;
		}
		return false;
	}
	return false;
}

bool AGridLevelRuntimeActor::TryDropItemInstanceAtCell(
	const FGridItemInstance& ItemInstance, int32 CellX, int32 CellY, EGridEdge Edge, const FVector& LocalOffset)
{
	// Inventory-origin convenience path: preserve the explicit source
	// inventory's definition for genuinely unregistered synthetic items.
	// Registered identities always pass nullptr and resolve via ItemSystem.
	const AGrimrockPartyPawn* Party = ResolveWorldItemInteractionParty(this);
	UGridItemDefinitionAsset* Definition = FGridLegacyItemLookup::Resolve(
		Party, ItemInstance.ItemDefinitionId);
	return TryDropItemInstanceAtCell(ItemInstance, Definition, CellX, CellY, Edge, LocalOffset);
}

bool AGridLevelRuntimeActor::TryDropItemInstanceAtCell(
	const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinitionAsset, int32 CellX, int32 CellY, EGridEdge Edge, const FVector& LocalOffset)
{
	return TryDropItemInstanceAtCellInternal(ItemInstance, ItemDefinitionAsset, CellX, CellY, Edge, LocalOffset, true);
}

bool AGridLevelRuntimeActor::TryDropItemInstanceAtCellInternal(
	const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinitionAsset, int32 CellX, int32 CellY, EGridEdge Edge,
	const FVector& LocalOffset, bool bNotifyAndRoute)
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

	if (ItemInstance.OwnerType == EGridItemOwnerType::Cursor)
	{
		const AGrimrockPartyPawn* PartyPawn = ResolveWorldItemInteractionParty(this);
		if (!IsWithinWorldItemHandReach(this, PartyPawn, CellX, CellY, LocalOffset))
		{
			UE_LOG(LogGridLevelRuntime, Verbose,
				TEXT("GridInventory WorldDrop Rejected Item=%s Reason=BeyondHandReach PartyCell=(%d,%d) TargetCell=(%d,%d) Reach=%.1f"),
				*ItemInstance.ItemDefinitionId.ToString(), PartyPawn ? PartyPawn->CurrentCellX : INDEX_NONE, PartyPawn ? PartyPawn->CurrentCellY : INDEX_NONE,
				CellX, CellY, FMath::Max(0.0f, WorldItemPickupReach));
			return false;
		}
	}

	// WorldDrop receives an already-physical item (inventory, projectile,
	// monster loot or pending pit transfer), never an authored level request.
	// Do not reinterpret a persisted registered ARM N0 as a fresh N1 weapon.
	const UWorld* DropWorld = GetWorld();
	UGameInstance* DropGameInstance = DropWorld ? DropWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = DropGameInstance
		? DropGameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	const bool bCanonicalIdentity = Registry &&
		Registry->HasCanonicalItem(ItemInstance.ItemDefinitionId);
	// One canonical world-item initializer for *all* registered families,
	// including ARM N1-N20. The mesh-only legacy ARM route skipped canonical
	// physics, sparkle and visual effects after a PIT02 transfer.
	FGridResolvedItem CanonicalDrop;
	bool bRequireCanonical = true;
#if WITH_DEV_AUTOMATION_TESTS
	bRequireCanonical = DropGameInstance != nullptr;
#endif
	const bool bResolvedCanonical = bCanonicalIdentity &&
		Registry->ResolveItem(ItemInstance.ItemDefinitionId, ItemInstance.ItemLevel, CanonicalDrop) &&
		CanonicalDrop.IsValid() && CanonicalDrop.ItemId == ItemInstance.ItemDefinitionId &&
		CanonicalDrop.ItemLevel == ItemInstance.ItemLevel &&
		(CanonicalDrop.bIsArmament ? ItemInstance.ItemLevel >= 1 : ItemInstance.ItemLevel == 0) &&
		ItemInstance.Quantity >= 1 && ItemInstance.Quantity <= FMath::Max(1, CanonicalDrop.MaxStack);
	if ((bRequireCanonical || bCanonicalIdentity) && !bResolvedCanonical)
	{
		UE_LOG(LogGridLevelRuntime, Warning,
			TEXT("GridInventory WorldDrop rejected canonical Item=%s Level=%d"),
			*ItemInstance.ItemDefinitionId.ToString(), ItemInstance.ItemLevel);
		return false; // A known ARM N0 must never inherit N1 or use legacy.
	}
	if (bRequireCanonical && SpawnedItemEntries.ContainsByPredicate([&ItemInstance](const FGridSpawnedItemRuntimeEntry& Existing)
		{ return Existing.ObjectId == ItemInstance.RuntimeObjectId && IsValid(Existing.ItemActor.Get()); }))
		return false;

	UGridItemDefinitionAsset* ItemDefinition = bCanonicalIdentity ? nullptr :
		(IsValid(ItemDefinitionAsset) ? ItemDefinitionAsset : nullptr);
	const FGridReceptacleItemMetadataReadView LevelRead =
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry, ItemDefinition, ItemInstance.ItemDefinitionId, ItemInstance.ItemLevel);
	if (!LevelRead.bResolved || (!LevelRead.bCanonicalIdentity && !IsValid(ItemDefinition)))
	{
		UE_LOG(LogGridLevelRuntime, Warning,
			TEXT("GridInventory WorldDrop Failed Item=%s InvalidPersistedItemLevel=%d"),
			*ItemInstance.ItemDefinitionId.ToString(), ItemInstance.ItemLevel);
		return false;
	}
	const int32 ResolvedItemLevel = LevelRead.FixedItemLevel;

	if (Edge == EGridEdge::None)
	{
		FGridRelocationBehaviorParams PitRelocation;
		if (FindOpenPitAtCell(CellX, CellY, PitRelocation))
		{
			if (!bNotifyAndRoute) return false; // Pending arrivals cannot silently route to another pit.
			FGridItemInstance ResolvedInstance = ItemInstance;
			ResolvedInstance.ItemLevel = ResolvedItemLevel;
			return TryRouteWorldItemThroughOpenPit(ResolvedInstance, ItemDefinition, CellX, CellY, LocalOffset);
		}
	}

	FTransform DropTransform;
	if (Edge == EGridEdge::None)
	{
		DropTransform = FTransform(FRotator::ZeroRotator, GetCellCenterWorld(CellX, CellY, 12.f) + LocalOffset, FVector::OneVector);
	}
	else
	{
		FGridLooseItemInstance PlacementData;
		PlacementData.CellX = CellX;
		PlacementData.CellY = CellY;
		PlacementData.SurfaceSide = Edge;
		if (!GridPlacementTransformResolver::ResolveLooseItem(*this, PlacementData, DropTransform))
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
	FGridItemInstance DroppedVisualItem = ItemInstance;
	DroppedVisualItem.RuntimeObjectId = RuntimeObjectId;
	DroppedVisualItem.ItemLevel = ResolvedItemLevel;
	if (bCanonicalIdentity)
	{
		// One full projection for ARM and non-ARM world pickups. The saved
		// light state is still applied explicitly below, after initialization.
		if (!ItemActor->InitializeFromCanonicalItem(DroppedVisualItem, Registry))
		{
			UE_LOG(LogGridLevelRuntime, Warning,
				TEXT("GridInventory WorldDrop Failed Item=%s Reason=CanonicalInitializationFailed"),
				*ItemInstance.ItemDefinitionId.ToString());
			ItemActor->Destroy();
			return false;
		}
	}
	else
	{
		if (IsValid(ItemDefinition))
		{
			ItemActor->InitializeFromItemDefinition(ItemDefinition, RuntimeObjectId);
		}
		else
		{
			ItemActor->InitializeFromItemDefinitionId(ItemInstance.ItemDefinitionId, RuntimeObjectId);
		}
		ItemActor->ApplyCanonicalWorldVisual(DroppedVisualItem);
	}
	ItemActor->SetRuntimeObjectId(RuntimeObjectId);
	ItemActor->SetRuntimeCell(CellX, CellY);
	ItemActor->ApplyWorldPhysicsInitialNudge();
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
	Entry.ItemDefinitionAsset = ItemDefinition;
	Entry.ItemDefinitionId = ItemInstance.ItemDefinitionId;
	Entry.ItemLevel = ResolvedItemLevel;
	Entry.Quantity = FMath::Max(1, ItemInstance.Quantity);

	SpawnedItemActors.Add(ItemActor);
	SpawnedItemEntries.Add(Entry);
	if (bNotifyAndRoute && ActivationComponent)
	{
		ActivationComponent->RefreshPressurePlatesAtCell(CellX, CellY);
	}

	UE_LOG(LogGridLevelRuntime, Verbose, TEXT("GridInventory WorldDrop Item=%s RuntimeId=%s Quantity=%d Cell=(%d,%d) Edge=%d Result=true"), *Entry.ItemDefinitionId.ToString(),
		*Entry.ObjectId.ToString(), Entry.Quantity, CellX, CellY, static_cast<int32>(Edge));
	return true;
}

bool AGridLevelRuntimeActor::TrySpawnThrownItemProjectile(
	const FGridItemInstance& ItemInstance, const FVector& StartWorldLocation, const FVector& LaunchVelocity, int32 SourceCellX, int32 SourceCellY)
{
	const AGrimrockPartyPawn* Party = ResolveWorldItemInteractionParty(this);
	UGridItemDefinitionAsset* ItemDefinition = FGridLegacyItemLookup::Resolve(Party, ItemInstance.ItemDefinitionId);
	return SpawnThrownItemProjectile(ItemInstance, ItemDefinition, StartWorldLocation, LaunchVelocity, SourceCellX, SourceCellY,
		false, const_cast<AGrimrockPartyPawn*>(Party)) != nullptr;
}

AGridThrownItemActor* AGridLevelRuntimeActor::SpawnThrownItemProjectile(const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinition,
	const FVector& StartWorldLocation, const FVector& LaunchVelocity, int32 SourceCellX, int32 SourceCellY, bool bRangedAmmunitionProjectile,
	AActor* LaunchSourceActor)
{
	// Ordinary hand throws remain unchanged. Only the authoritative ranged
	// attack may opt into a non-handheld, stackable ammunition projectile.
	// Specialized ARM projectiles have their full physics and visuals in
	// DA_ItemSystem. An obsolete ItemDefinition must not be their gate.
	const UWorld* SpawnWorld = GetWorld();
	UGameInstance* SpawnGI = SpawnWorld ? SpawnWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* SpawnRegistry = SpawnGI ? SpawnGI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	const bool bCanonicalIdentity = SpawnRegistry &&
		SpawnRegistry->HasCanonicalItem(ItemInstance.ItemDefinitionId);
	bool bRequireCanonical = true;
#if WITH_DEV_AUTOMATION_TESTS
	bRequireCanonical = !GIsAutomationTesting || SpawnGI != nullptr;
#endif
	FGridResolvedItem Canonical;
	const bool bCanonicalResolved = bCanonicalIdentity &&
		SpawnRegistry->ResolveItem(ItemInstance.ItemDefinitionId, ItemInstance.ItemLevel, Canonical) &&
		Canonical.IsValid() && Canonical.ItemId == ItemInstance.ItemDefinitionId &&
		Canonical.ItemLevel == ItemInstance.ItemLevel &&
		(Canonical.bIsArmament ? ItemInstance.ItemLevel >= 1 : ItemInstance.ItemLevel == 0) &&
		ItemInstance.Quantity == 1;
	if ((bRequireCanonical || bCanonicalIdentity) && !bCanonicalResolved)
		return nullptr;
	// A thrown item must retain a valid persisted physical level.
	// A rejected registered identity cannot launch from an old definition.
	const bool bValidCanonicalPhysicalLevel = !bCanonicalIdentity ||
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			SpawnRegistry, nullptr, ItemInstance.ItemDefinitionId, ItemInstance.ItemLevel).bResolved;
	const bool bCanonicalSpecialized = bCanonicalResolved && Canonical.bIsArmament &&
		Canonical.Weapon.Projectile.bEnabled &&
		FMath::IsFinite(Canonical.Weapon.Projectile.InitialSpeed) &&
		Canonical.Weapon.Projectile.InitialSpeed > KINDA_SMALL_NUMBER &&
		!Canonical.ProjectileMesh.IsNull();
	const bool bCanonicalGenericArm = bCanonicalResolved && Canonical.bIsArmament &&
		Canonical.Weapon.Projectile.bGenericThrowAuthored &&
		Canonical.Weapon.Projectile.bGenericThrowOneHanded &&
		FMath::IsFinite(Canonical.WeightKg) && Canonical.WeightKg >= 0.0f &&
		FMath::IsFinite(Canonical.Weapon.Projectile.InitialSpeed) &&
		Canonical.Weapon.Projectile.InitialSpeed > KINDA_SMALL_NUMBER && FMath::IsFinite(Canonical.Weapon.Projectile.Arc);
	const bool bCanonicalGenericNonArm = bCanonicalResolved && !Canonical.bIsArmament &&
		Canonical.NonArmHandPolicy == EGridNonArmamentHandPolicy::OneHanded &&
		FMath::IsFinite(Canonical.WeightKg) && Canonical.WeightKg >= 0.0f &&
		FMath::IsFinite(Canonical.GenericThrow.InitialSpeed) &&
		Canonical.GenericThrow.InitialSpeed > KINDA_SMALL_NUMBER && FMath::IsFinite(Canonical.GenericThrow.Arc);
	const bool bMatchingLegacy = !bRequireCanonical && !bCanonicalIdentity && IsValid(ItemDefinition) &&
		ItemDefinition->ItemDefinitionId == ItemInstance.ItemDefinitionId;
	const bool bValidProjectileSource = bRangedAmmunitionProjectile
		? (bCanonicalIdentity
			? (bCanonicalResolved && !Canonical.bIsArmament &&
				!Canonical.AmmoKindId.IsNone() && Canonical.MaxStack > 1 &&
				ItemInstance.ItemLevel == 0 && ItemInstance.Quantity == 1)
			: (bMatchingLegacy && ItemDefinition->bStackable && ItemInstance.Quantity == 1 &&
				!ItemDefinition->IsCombatThrowable()))
		: (bCanonicalIdentity
			? (bCanonicalSpecialized || bCanonicalGenericNonArm || bCanonicalGenericArm)
			: (bMatchingLegacy && ItemDefinition->IsPhysicallyThrowable()));
	if (!ItemInstance.IsValid() || !bValidCanonicalPhysicalLevel || !bValidProjectileSource ||
		LaunchVelocity.IsNearlyZero() || !IsWalkableCell(SourceCellX, SourceCellY))
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

	ThrownActor->InitializeThrownItem(this, ItemInstance, bCanonicalIdentity ? nullptr : ItemDefinition,
		LaunchVelocity, SourceCellX, SourceCellY);
	if (!IsValid(ThrownActor) || ThrownActor->IsActorBeingDestroyed())
	{
		return nullptr; // A failed canonical init destroyed the incomplete actor.
	}
	ThrownActor->ConfigureLaunchSourceActor(LaunchSourceActor ? LaunchSourceActor :
		const_cast<AGrimrockPartyPawn*>(ResolveWorldItemInteractionParty(this)));

	// A collision volume may extend beyond the actual door plane. A door on
	// the *rear* launch-cell boundary is not crossed by this shot. Filter it
	// before first movement; never ignore the forward door or the maze walls.
	for (TActorIterator<AGridDoorActor> It(World); It; ++It)
	{
		AGridDoorActor* Door = *It;
		if (IsValid(Door) && FGridProjectileGeometry::IsRearBoundaryDoor(
			SourceCellX, SourceCellY, LaunchVelocity, Door->CellX, Door->CellY, Door->Edge))
		{
			ThrownActor->CollisionComponent->IgnoreActorWhenMoving(Door, true);
			UE_LOG(LogGridLevelRuntime, Log,
				TEXT("[GridProjectile] IgnoreRearDoor Item=%s Door=%s SourceCell=(%d,%d) DoorCell=(%d,%d) DoorEdge=%d Heading=%d"),
				*ItemInstance.ItemDefinitionId.ToString(), *GetNameSafe(Door), SourceCellX, SourceCellY,
				Door->CellX, Door->CellY, static_cast<int32>(Door->Edge),
				static_cast<int32>(FGridProjectileGeometry::FlightCardinalEdge(LaunchVelocity)));
		}
	}
	if (bRangedAmmunitionProjectile)
	{
		ThrownActor->ConfigureAsRangedAmmunition();
		// A loose pickup cannot be a physical blocker for a bow projectile.
		// Preserve blocking WorldStatic and dynamic doors/monsters.
		for (TActorIterator<AGridItemActor> It(World); It; ++It)
		{
			AGridItemActor* Item = *It;
			if (IsValid(Item) && Item != ThrownActor && !Item->IsA<AGridThrownItemActor>())
			{
				ThrownActor->CollisionComponent->IgnoreActorWhenMoving(Item, true);
			}
		}
	}

	UE_LOG(LogGridLevelRuntime, Verbose, TEXT("GridInventory Throw Spawn Item=%s RuntimeId=%s SourceCell=(%d,%d) Speed=%.2f Result=true"),
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
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	float TotalWeight = 0.0f;

	for (const FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
	{
		if ((!bIncludeEdgeItems && Entry.Edge != EGridEdge::None) || !IsValid(Entry.ItemActor.Get()))
		{
			continue;
		}
		FIntPoint ActualCell = Entry.Cell;
		if (Entry.Edge == EGridEdge::None && IsValid(Entry.ItemActor->MeshComponent) &&
			Entry.ItemActor->MeshComponent->IsSimulatingPhysics())
		{
			int32 X = INDEX_NONE, Y = INDEX_NONE;
			FVector Offset;
			if (TryResolveWorldCellFromImpactPoint(Entry.ItemActor->GetWorldPickupLocation(), X, Y, Offset))
			{
				ActualCell = FIntPoint(X, Y);
			}
		}
		if (ActualCell != TargetCell)
		{
			continue;
		}

		const FName ItemId = Entry.ItemDefinitionId.IsNone()
			? Entry.ItemActor->GetItemDefinitionId() : Entry.ItemDefinitionId;
		const bool bCanonical = Registry && Registry->HasCanonicalItem(ItemId);
		// Read the registry for registered item weights, even if a stale
		// entry from a historical runtime build still holds an old pointer.
		UGridItemDefinitionAsset* Definition = bCanonical ? nullptr : Entry.ItemDefinitionAsset.Get();
		if (!bCanonical && !Definition)
		{
			continue; // Preserve historical behavior for unregistered missing definitions.
		}

		FGridItemInstance Instance;
		Instance.RuntimeObjectId = Entry.ObjectId.IsValid() ? Entry.ObjectId :
			Entry.ItemActor->GetRuntimeObjectId();
		Instance.ItemDefinitionId = ItemId;
		Instance.ItemLevel = Entry.ItemLevel;
		Instance.Quantity = FMath::Max(1, Entry.Quantity);
		Instance.Weight = Definition ? Definition->Weight : 0.0f;
		float UnitWeight = 0.0f;
		if (!FGridItemWeightReadModel::TryResolveUnitWeight(Registry, Instance, UnitWeight))
		{
			continue; // Invalid canonical levels never use copied definition mass.
		}
		const float Contribution = UnitWeight * Instance.Quantity;
		TotalWeight += Contribution;
		UE_LOG(LogGridLevelRuntime, Verbose, TEXT("GridPressurePlate WeightScan Cell=(%d,%d) Item=%s Quantity=%d UnitWeight=%.2f TotalContribution=%.2f"), CellX, CellY,
			*ItemId.ToString(), Instance.Quantity, UnitWeight, Contribution);
	}

	return TotalWeight;
}
