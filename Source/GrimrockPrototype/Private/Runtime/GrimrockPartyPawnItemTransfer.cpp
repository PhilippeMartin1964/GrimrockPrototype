#include "Runtime/GrimrockPartyPawn.h"

#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridThrownItemActor.h"

namespace
{
	bool GridPartyPawnItemTransferIsHandEquipmentSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
	}

	const TCHAR* GridPartyPawnItemTransferGetEquipmentSlotName(EGridEquipmentSlot Slot)
	{
		switch (Slot)
		{
			case EGridEquipmentSlot::None:
				return TEXT("None");
			case EGridEquipmentSlot::MainHand:
				return TEXT("MainHand");
			case EGridEquipmentSlot::OffHand:
				return TEXT("OffHand");
			case EGridEquipmentSlot::Head:
				return TEXT("Head");
			case EGridEquipmentSlot::Chest:
				return TEXT("Chest");
			case EGridEquipmentSlot::Legs:
				return TEXT("Legs");
			case EGridEquipmentSlot::Feet:
				return TEXT("Feet");
			case EGridEquipmentSlot::Amulet:
				return TEXT("Amulet");
			case EGridEquipmentSlot::Ring1:
				return TEXT("Ring1");
			case EGridEquipmentSlot::Ring2:
				return TEXT("Ring2");
			case EGridEquipmentSlot::Shoulders:
				return TEXT("Shoulders");
			case EGridEquipmentSlot::Gloves:
				return TEXT("Gloves");
			case EGridEquipmentSlot::Belt:
				return TEXT("Belt");
			case EGridEquipmentSlot::Cloak:
				return TEXT("Cloak");
			case EGridEquipmentSlot::Talisman:
				return TEXT("Talisman");
			case EGridEquipmentSlot::QuickSlot1:
				return TEXT("QuickSlot1");
			case EGridEquipmentSlot::QuickSlot2:
				return TEXT("QuickSlot2");
			case EGridEquipmentSlot::Face:
				return TEXT("Visage");
			case EGridEquipmentSlot::Shirt:
				return TEXT("Chemise");
			case EGridEquipmentSlot::Bracers:
				return TEXT("Brassards");
			case EGridEquipmentSlot::Earring1:
				return TEXT("Bijou d'oreille I");
			case EGridEquipmentSlot::Earring2:
				return TEXT("Bijou d'oreille II");
			default:
				return TEXT("Unsupported");
		}
	}
}

bool AGrimrockPartyPawn::EquipSelectedCharacterItemFromInventorySlot(int32 InventorySlotIndex, EGridEquipmentSlot TargetSlot)
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Equip Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName());
		return false;
	}

	const bool bEquipped =
		PartyInventoryComponent->EquipItemFromInventorySlot(PartyInventoryComponent->GetSelectedCharacterIndex(), InventorySlotIndex, TargetSlot);

	if (bEquipped && GridPartyPawnItemTransferIsHandEquipmentSlot(TargetSlot))
	{
		SyncHeldVisualFromSelectedCharacterEquipment();
	}
	return bEquipped;
}

bool AGrimrockPartyPawn::UnequipSelectedCharacterItemToInventory(EGridEquipmentSlot SourceSlot)
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Unequip Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName());
		return false;
	}

	const int32 CharacterIndex = PartyInventoryComponent->GetSelectedCharacterIndex();
	FGridItemInstance PreviouslyEquippedItem;
	const bool bHadHandItem = GridPartyPawnItemTransferIsHandEquipmentSlot(SourceSlot) &&
		PartyInventoryComponent->GetEquippedItem(CharacterIndex, SourceSlot, PreviouslyEquippedItem);

	const bool bUnequipped = PartyInventoryComponent->UnequipItemToInventory(CharacterIndex, SourceSlot);
	if (bUnequipped && GridPartyPawnItemTransferIsHandEquipmentSlot(SourceSlot))
	{
		if (bHadHandItem && PreviouslyEquippedItem.ItemDefinitionId == GetHeldItemDefinitionId())
		{
			ClearHeldItem();
			UE_LOG(LogTemp, Log, TEXT("GridInventory HeldVisual Clear Unequipped Character=%d Slot=%s Item=%s"), CharacterIndex,
				GridPartyPawnItemTransferGetEquipmentSlotName(SourceSlot), *PreviouslyEquippedItem.ItemDefinitionId.ToString());
		}

		SyncHeldVisualFromSelectedCharacterEquipment();
	}

	return bUnequipped;
}

bool AGrimrockPartyPawn::TryTakeSelectedCharacterEquipmentSlotToCursor(EGridEquipmentSlot SourceSlot)
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take Equipment Relay Failed Pawn=%s Slot=%s Reason=NoPartyInventoryComponent"), *GetName(),
			GridPartyPawnItemTransferGetEquipmentSlotName(SourceSlot));
		return false;
	}

	const bool bTaken = PartyInventoryComponent->TryTakeSelectedCharacterEquipmentSlotToCursor(SourceSlot);
	if (bTaken)
	{
		SyncHeldVisualFromSelectedCharacterEquipment();
	}

	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Take Equipment Relay Pawn=%s Slot=%s Result=%s"), *GetName(),
		GridPartyPawnItemTransferGetEquipmentSlotName(SourceSlot), bTaken ? TEXT("true") : TEXT("false"));
	return bTaken;
}

bool AGrimrockPartyPawn::TryTakeSelectedCharacterMainHandToCursor()
{
	return TryTakeSelectedCharacterEquipmentSlotToCursor(EGridEquipmentSlot::MainHand);
}

bool AGrimrockPartyPawn::TryTakeSelectedCharacterOffHandToCursor()
{
	return TryTakeSelectedCharacterEquipmentSlotToCursor(EGridEquipmentSlot::OffHand);
}

bool AGrimrockPartyPawn::TryEquipCursorItemToSelectedCharacterSlot(EGridEquipmentSlot TargetSlot)
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Equip Failed Pawn=%s Slot=%s Reason=NoPartyInventoryComponent"), *GetName(),
			GridPartyPawnItemTransferGetEquipmentSlotName(TargetSlot));
		return false;
	}

	const bool bEquipped = PartyInventoryComponent->TryEquipCursorItemToSelectedCharacterSlot(TargetSlot);
	if (bEquipped && GridPartyPawnItemTransferIsHandEquipmentSlot(TargetSlot))
	{
		SyncHeldVisualFromSelectedCharacterEquipment();
	}

	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Equip Relay Pawn=%s Slot=%s Result=%s"), *GetName(),
		GridPartyPawnItemTransferGetEquipmentSlotName(TargetSlot), bEquipped ? TEXT("true") : TEXT("false"));
	return bEquipped;
}

bool AGrimrockPartyPawn::TryEquipCursorItemToSelectedCharacterMainHand()
{
	return TryEquipCursorItemToSelectedCharacterSlot(EGridEquipmentSlot::MainHand);
}

bool AGrimrockPartyPawn::TryEquipCursorItemToSelectedCharacterOffHand()
{
	return TryEquipCursorItemToSelectedCharacterSlot(EGridEquipmentSlot::OffHand);
}

bool AGrimrockPartyPawn::HasCursorItem() const
{
	return PartyInventoryComponent && PartyInventoryComponent->HasCursorItem();
}

bool AGrimrockPartyPawn::GetCursorItem(FGridItemInstance& OutItem) const
{
	OutItem = FGridItemInstance();
	if (!PartyInventoryComponent || !PartyInventoryComponent->HasCursorItem())
	{
		return false;
	}

	OutItem = PartyInventoryComponent->GetCursorItem();
	return true;
}

bool AGrimrockPartyPawn::DebugTakeInventorySlotToCursor(int32 CharacterIndex, int32 InventorySlotIndex)
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take Relay Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName());
		return false;
	}

	const bool bTaken = PartyInventoryComponent->TryTakeInventorySlotToCursor(CharacterIndex, InventorySlotIndex);
	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Take Relay Pawn=%s Character=%d Slot=%d Result=%s"), *GetName(), CharacterIndex, InventorySlotIndex,
		bTaken ? TEXT("true") : TEXT("false"));
	return bTaken;
}

bool AGrimrockPartyPawn::DebugPlaceCursorItemInSelectedInventory()
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place Relay Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName());
		return false;
	}

	const bool bPlaced = PartyInventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory();
	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Place Relay Pawn=%s Result=%s"), *GetName(), bPlaced ? TEXT("true") : TEXT("false"));
	return bPlaced;
}

bool AGrimrockPartyPawn::TryPlaceCursorItemInReceptacle(AGridReceptacleActor* ReceptacleActor)
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place ToReceptacle Failed Item=None Receptacle=%s Reason=NoPartyInventoryComponent"),
			ReceptacleActor ? *ReceptacleActor->GetName() : TEXT("None"));
		return false;
	}

	if (!ReceptacleActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place ToReceptacle Failed Item=%s Receptacle=None Reason=NoReceptacle"),
			PartyInventoryComponent->HasCursorItem() ? *PartyInventoryComponent->GetCursorItem().ItemDefinitionId.ToString() : TEXT("None"));
		return false;
	}

	if (!PartyInventoryComponent->HasCursorItem())
	{
		UE_LOG(
			LogTemp, Warning, TEXT("GridInventory Cursor Place ToReceptacle Failed Item=None Receptacle=%s Reason=NoCursorItem"), *ReceptacleActor->GetName());
		return false;
	}

	if (!LevelRuntimeActor || !LevelRuntimeActor->CanPartyInteractWithEdgeObject(ReceptacleActor->CellX, ReceptacleActor->CellY, ReceptacleActor->Edge, this))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place ToReceptacle Failed Item=%s Receptacle=%s Reason=EdgeNotFacingParty"),
			*PartyInventoryComponent->GetCursorItem().ItemDefinitionId.ToString(), *ReceptacleActor->GetName());
		return false;
	}

	const FGridItemInstance CursorItem = PartyInventoryComponent->GetCursorItem();
	FGridItemInstance SingleItem;
	if (!BuildSingleItemInstanceFromCursor(SingleItem))
	{
		return false;
	}

	FGridItemInstance AcceptedItem;
	if (!ReceptacleActor->TryInsertItemInstanceFromCursor(SingleItem, AcceptedItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place ToReceptacle Failed Item=%s RuntimeId=%s Receptacle=%s Reason=ReceptacleRejected"),
			*CursorItem.ItemDefinitionId.ToString(), *CursorItem.RuntimeObjectId.ToString(), *ReceptacleActor->GetName());
		return false;
	}

	ConsumeOneCursorItemAfterSuccessfulAction();
	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor ConsumedOne AfterReceptacle Item=%s RuntimeId=%s Receptacle=%s"), *AcceptedItem.ItemDefinitionId.ToString(),
		*AcceptedItem.RuntimeObjectId.ToString(), *ReceptacleActor->GetName());
	PartyInventoryComponent->LogInventoryOwnershipDiagnostics();

	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Place ToReceptacle Item=%s RuntimeId=%s Receptacle=%s Result=true"),
		*AcceptedItem.ItemDefinitionId.ToString(), *AcceptedItem.RuntimeObjectId.ToString(), *ReceptacleActor->GetName());
	return true;
}

bool AGrimrockPartyPawn::TryDropCursorItemAtCell(int32 CellX, int32 CellY, EGridEdge Edge, const FVector& LocalOffset)
{
	if (!PartyInventoryComponent || !PartyInventoryComponent->HasCursorItem() || !LevelRuntimeActor)
	{
		return false;
	}

	FGridItemInstance DroppedItem;
	if (!BuildSingleItemInstanceFromCursor(DroppedItem) || !LevelRuntimeActor->TryDropItemInstanceAtCell(DroppedItem, CellX, CellY, Edge, LocalOffset))
	{
		return false;
	}

	ConsumeOneCursorItemAfterSuccessfulAction();
	PartyInventoryComponent->LogInventoryOwnershipDiagnostics();
	return true;
}

bool AGrimrockPartyPawn::TryThrowOneCursorItem(const FVector& LaunchDirection, EGridItemThrowMode ThrowMode)
{
	if (!PartyInventoryComponent || !LevelRuntimeActor || !PartyInventoryComponent->HasCursorItem())
	{
		return false;
	}

	const FGridItemInstance CursorItem = PartyInventoryComponent->GetCursorItem();
	UGridItemDefinitionAsset* ItemDefinition = LevelRuntimeActor->ResolveRuntimeItemDefinition(CursorItem.ItemDefinitionId);
	if (!ItemDefinition || !ItemDefinition->bThrowable)
	{
		return false;
	}

	FVector ThrowDirection = LaunchDirection.GetSafeNormal();
	if (ThrowDirection.IsNearlyZero())
	{
		ThrowDirection = Camera ? Camera->GetForwardVector() : GetActorForwardVector();
	}
	const bool bShortToss = ThrowMode == EGridItemThrowMode::ShortToss;
	const float SpeedScale = bShortToss ? FMath::Max(0.0f, ShortThrowSpeedScale) : 1.0f;
	const float ArcScale = bShortToss ? FMath::Max(0.0f, ShortThrowArcScale) : 1.0f;
	ThrowDirection = (ThrowDirection + FVector::UpVector * FMath::Max(0.0f, ItemDefinition->ThrowArc) * ArcScale).GetSafeNormal();

	FGridItemInstance ThrownItem;
	if (!BuildSingleItemInstanceFromCursor(ThrownItem))
	{
		return false;
	}

	ThrownItem.Weight = ItemDefinition->Weight;
	ThrownItem.OwnerType = EGridItemOwnerType::World;
	ThrownItem.OwnerGuid = FGuid();
	ThrownItem.OwnerCharacterIndex = INDEX_NONE;
	ThrownItem.EquipmentSlot = EGridEquipmentSlot::None;

	const FVector StartLocation = (Camera ? Camera->GetComponentLocation() : GetActorLocation()) + ThrowDirection * 60.0f;
	// TODO: Scale throw speed, accuracy and damage with the selected character's ranged/throwing skill.
	const FVector LaunchVelocity = ThrowDirection * FMath::Max(0.0f, ItemDefinition->ThrowSpeed) * SpeedScale;
	if (!LevelRuntimeActor->TrySpawnThrownItemProjectile(ThrownItem, StartLocation, LaunchVelocity, CurrentCellX, CurrentCellY))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Throw Failed Item=%s Quantity=%d Reason=ProjectileSpawnFailed"), *CursorItem.ItemDefinitionId.ToString(),
			CursorItem.Quantity);
		return false;
	}

	ConsumeOneCursorItemAfterSuccessfulAction();

	UE_LOG(LogTemp, Log, TEXT("GridInventory Throw Item=%s RuntimeId=%s Mode=%s CursorQuantityBefore=%d CursorQuantityAfter=%d Result=true"),
		*ThrownItem.ItemDefinitionId.ToString(), *ThrownItem.RuntimeObjectId.ToString(), bShortToss ? TEXT("ShortToss") : TEXT("Throw"), CursorItem.Quantity,
		FMath::Max(0, CursorItem.Quantity - 1));
	PartyInventoryComponent->LogInventoryOwnershipDiagnostics();
	return true;
}

AGridThrownItemActor* AGrimrockPartyPawn::TryLaunchEquippedItemForAttack(
	int32 CharacterIndex, EGridEquipmentSlot SourceSlot, FName ExpectedItemDefinitionId, const FVector& TargetWorldLocation, const FIntPoint& SourceCell)
{
	if (!PartyInventoryComponent || !LevelRuntimeActor || !GridPartyPawnItemTransferIsHandEquipmentSlot(SourceSlot) || ExpectedItemDefinitionId.IsNone() ||
		TargetWorldLocation.ContainsNaN())
	{
		return nullptr;
	}

	FGridItemInstance EquippedItem;
	if (!PartyInventoryComponent->GetEquippedItem(CharacterIndex, SourceSlot, EquippedItem) || EquippedItem.ItemDefinitionId != ExpectedItemDefinitionId)
	{
		return nullptr;
	}

	UGridItemDefinitionAsset* ItemDefinition = ResolveEquippedItemDefinition(EquippedItem);
	if (!ItemDefinition || !ItemDefinition->bThrowable || ItemDefinition->ThrowSpeed <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}

	FVector StartLocation = Camera ? Camera->GetComponentLocation() : GetActorLocation();
	FVector ThrowDirection = (TargetWorldLocation - StartLocation).GetSafeNormal();
	if (ThrowDirection.IsNearlyZero())
	{
		ThrowDirection = Camera ? Camera->GetForwardVector() : GetActorForwardVector();
	}
	const FVector ViewRight = Camera ? Camera->GetRightVector() : GetActorRightVector();
	StartLocation += ThrowDirection * 60.0f + ViewRight * 18.0f - FVector::UpVector * 15.0f;
	ThrowDirection = (TargetWorldLocation - StartLocation).GetSafeNormal();
	ThrowDirection = (ThrowDirection + FVector::UpVector * FMath::Max(0.0f, ItemDefinition->ThrowArc)).GetSafeNormal();

	FGridItemInstance WorldItem;
	if (!PartyInventoryComponent->TryExtractOneEquippedItemForWorldTransfer(CharacterIndex, SourceSlot, ExpectedItemDefinitionId, WorldItem))
	{
		return nullptr;
	}
	WorldItem.Weight = ItemDefinition->Weight;

	AGridThrownItemActor* ThrownActor = LevelRuntimeActor->SpawnThrownItemProjectile(
		WorldItem, ItemDefinition, StartLocation, ThrowDirection * FMath::Max(0.0f, ItemDefinition->ThrowSpeed), SourceCell.X, SourceCell.Y);
	if (!ThrownActor)
	{
		const bool bRestored = PartyInventoryComponent->TryRestoreExtractedItemToEquipment(CharacterIndex, SourceSlot, WorldItem);
		UE_LOG(LogTemp, Error, TEXT("GridPlayerAttack Throw Failed Item=%s Character=%d Slot=%s Restored=%s"), *ExpectedItemDefinitionId.ToString(),
			CharacterIndex, GridPartyPawnItemTransferGetEquipmentSlotName(SourceSlot), bRestored ? TEXT("true") : TEXT("false"));
		return nullptr;
	}

	SyncHeldVisualFromSelectedCharacterEquipment();
	UE_LOG(LogTemp, Log, TEXT("GridPlayerAttack Throw Launched Item=%s RuntimeId=%s Character=%d Slot=%s Target=(%.2f,%.2f,%.2f) Result=true"),
		*WorldItem.ItemDefinitionId.ToString(), *WorldItem.RuntimeObjectId.ToString(), CharacterIndex,
		GridPartyPawnItemTransferGetEquipmentSlotName(SourceSlot), TargetWorldLocation.X, TargetWorldLocation.Y, TargetWorldLocation.Z);
	return ThrownActor;
}

AGridThrownItemActor* AGrimrockPartyPawn::TryLaunchInventoryItemForAttack(
	int32 CharacterIndex, FName ExpectedItemDefinitionId, const FVector& TargetWorldLocation, const FIntPoint& SourceCell)
{
	if (!PartyInventoryComponent || !LevelRuntimeActor || ExpectedItemDefinitionId.IsNone() || TargetWorldLocation.ContainsNaN() ||
		!PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return nullptr;
	}

	const FGridCharacterInventoryState& Character = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	const FGridInventorySlot* SourceSlot = Character.InventorySlots.FindByPredicate(
		[ExpectedItemDefinitionId](const FGridInventorySlot& Candidate)
		{
			return !Candidate.IsEmpty() && Candidate.Item.ItemDefinitionId == ExpectedItemDefinitionId;
		});
	UGridItemDefinitionAsset* ItemDefinition = PartyInventoryComponent->FindItemDefinition(ExpectedItemDefinitionId);
	if (!SourceSlot || !IsValid(ItemDefinition) || !ItemDefinition->bThrowable || ItemDefinition->ThrowSpeed <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}

	FVector StartLocation = Camera ? Camera->GetComponentLocation() : GetActorLocation();
	FVector ThrowDirection = (TargetWorldLocation - StartLocation).GetSafeNormal();
	if (ThrowDirection.IsNearlyZero())
	{
		ThrowDirection = Camera ? Camera->GetForwardVector() : GetActorForwardVector();
	}
	const FVector ViewRight = Camera ? Camera->GetRightVector() : GetActorRightVector();
	StartLocation += ThrowDirection * 60.0f + ViewRight * 18.0f - FVector::UpVector * 15.0f;
	ThrowDirection = (TargetWorldLocation - StartLocation).GetSafeNormal();
	ThrowDirection = (ThrowDirection + FVector::UpVector * FMath::Max(0.0f, ItemDefinition->ThrowArc)).GetSafeNormal();

	FGridItemInstance WorldItem = SourceSlot->Item;
	WorldItem.RuntimeObjectId = FGuid::NewGuid();
	WorldItem.Quantity = 1;
	WorldItem.Weight = ItemDefinition->Weight;
	WorldItem.OwnerType = EGridItemOwnerType::World;
	WorldItem.OwnerGuid = FGuid();
	WorldItem.OwnerCharacterIndex = INDEX_NONE;
	WorldItem.EquipmentSlot = EGridEquipmentSlot::None;
	AGridThrownItemActor* ThrownActor = LevelRuntimeActor->SpawnThrownItemProjectile(
		WorldItem, ItemDefinition, StartLocation, ThrowDirection * FMath::Max(0.0f, ItemDefinition->ThrowSpeed), SourceCell.X, SourceCell.Y);
	if (ThrownActor)
	{
		UE_LOG(LogTemp, Log, TEXT("GridPlayerAttack InventoryThrow VisualLaunched Item=%s RuntimeId=%s Character=%d"), *WorldItem.ItemDefinitionId.ToString(),
			*WorldItem.RuntimeObjectId.ToString(), CharacterIndex);
	}
	return ThrownActor;
}

bool AGrimrockPartyPawn::BuildSingleItemInstanceFromCursor(FGridItemInstance& OutSingleItem) const
{
	OutSingleItem = FGridItemInstance();
	if (!PartyInventoryComponent || !PartyInventoryComponent->HasCursorItem())
	{
		return false;
	}

	const FGridItemInstance& CursorItem = PartyInventoryComponent->GetCursorItem();
	if (!CursorItem.IsValid())
	{
		return false;
	}

	OutSingleItem = CursorItem;
	OutSingleItem.RuntimeObjectId = FGuid::NewGuid();
	OutSingleItem.Quantity = 1;
	return true;
}

void AGrimrockPartyPawn::ConsumeOneCursorItemAfterSuccessfulAction()
{
	if (!PartyInventoryComponent || !PartyInventoryComponent->HasCursorItem())
	{
		return;
	}

	const FGridItemInstance CursorItem = PartyInventoryComponent->GetCursorItem();
	if (CursorItem.Quantity > 1)
	{
		FGridItemInstance RemainingCursorItem = CursorItem;
		RemainingCursorItem.Quantity -= 1;
		PartyInventoryComponent->SetCursorItem(RemainingCursorItem);
		return;
	}

	PartyInventoryComponent->ClearCursorItem();
}

bool AGrimrockPartyPawn::DebugPlaceCursorItemInFrontReceptacle()
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GridInventory Cursor DebugPlaceInFront Failed Cell=(%d,%d) Edge=%d Item=None Receptacle=None Reason=NoPartyInventoryComponent"), CurrentCellX,
			CurrentCellY, static_cast<int32>(Facing));
		return false;
	}

	if (!LevelRuntimeActor)
	{
		LevelRuntimeActor = Cast<AGridLevelRuntimeActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
	}

	const FString CursorItemText =
		PartyInventoryComponent->HasCursorItem() ? PartyInventoryComponent->GetCursorItem().ItemDefinitionId.ToString() : FString(TEXT("None"));

	if (!LevelRuntimeActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor DebugPlaceInFront Failed Cell=(%d,%d) Edge=%d Item=%s Receptacle=None Reason=NoLevelRuntimeActor"),
			CurrentCellX, CurrentCellY, static_cast<int32>(Facing), *CursorItemText);
		return false;
	}

	AGridReceptacleActor* ReceptacleActor = LevelRuntimeActor->FindReceptacleAtEdge(CurrentCellX, CurrentCellY, Facing);
	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor DebugPlaceInFront Cell=(%d,%d) Edge=%d Item=%s Receptacle=%s"), CurrentCellX, CurrentCellY,
		static_cast<int32>(Facing), *CursorItemText, *GetNameSafe(ReceptacleActor));

	if (!ReceptacleActor)
	{
		return false;
	}

	return TryPlaceCursorItemInReceptacle(ReceptacleActor);
}
