#include "Runtime/GridWallLockActor.h"

#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

DEFINE_LOG_CATEGORY(LogGridWallLock);

namespace
{
	FString JoinItemDefinitionIds(const TArray<FName>& ItemDefinitionIds)
	{
		TArray<FString> Names;
		Names.Reserve(ItemDefinitionIds.Num());
		for (const FName ItemDefinitionId : ItemDefinitionIds)
		{
			Names.Add(ItemDefinitionId.ToString());
		}
		return Names.Num() > 0 ? FString::Join(Names, TEXT(", ")) : TEXT("None");
	}

	const TCHAR* GetMessageSource(const FText& Message)
	{
		return Message.IsEmpty() ? TEXT("Fallback") : TEXT("Behavior.Lock");
	}
}

void AGridWallLockActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	Super::InitializeGridObject(ObjectData, Mesh, WorldTransform);

	const FGridLockBehaviorParams& LockParams = ObjectData.Behavior.Lock;
	bIsUnlocked = LockParams.bStartsUnlocked;
	bConsumeKeyOnUnlock = LockParams.bConsumeKeyOnUnlock;
	LockedMessage = LockParams.LockedMessage;
	UnlockedMessage = LockParams.UnlockedMessage;
	MissingKeyMessage = LockParams.MissingKeyMessage;

	VisualPlacementMode = EGridReceptacleVisualPlacementMode::AttachedSocket;
	bSimulatePhysicsWhenPlaced = false;
	MaxContainedItems = 1;
	SetCanRemoveItem(false);

	AcceptedKeyDefinitionIds.Reset();
	for (const UGridItemDefinitionAsset* AcceptedKeyItem : LockParams.AcceptedKeyItems)
	{
		if (AcceptedKeyItem && !AcceptedKeyItem->ItemDefinitionId.IsNone())
		{
			AcceptedKeyDefinitionIds.AddUnique(AcceptedKeyItem->ItemDefinitionId);
		}
	}
	for (const FName AcceptedKeyId : LockParams.AcceptedKeyIds)
	{
		if (!AcceptedKeyId.IsNone())
		{
			AcceptedKeyDefinitionIds.AddUnique(AcceptedKeyId);
		}
	}

	UE_LOG(LogGridWallLock, Log,
		TEXT(
			"GridWallLock Init ObjectId=%s StartsUnlocked=%s ConsumeKey=%s AcceptedKeyDefinitionIds=[%s] LockedMessage=%s UnlockedMessage=%s MissingKeyMessage=%s"),
		*ObjectId.ToString(), bIsUnlocked ? TEXT("true") : TEXT("false"), bConsumeKeyOnUnlock ? TEXT("true") : TEXT("false"),
		*JoinItemDefinitionIds(AcceptedKeyDefinitionIds), GetMessageSource(LockedMessage), GetMessageSource(UnlockedMessage),
		GetMessageSource(MissingKeyMessage));
}

bool AGridWallLockActor::TryInteractWithParty(AGrimrockPartyPawn* PartyPawn)
{
	if (!PartyPawn || !PartyPawn->PartyInventoryComponent || !PartyPawn->LevelRuntimeActor)
	{
		UE_LOG(LogGridWallLock, Warning, TEXT("GridWallLock UnlockFailed MissingKey ObjectId=%s Reason=%s"), *ObjectId.ToString(),
			!PartyPawn ? TEXT("MissingPartyPawn") : (!PartyPawn->PartyInventoryComponent ? TEXT("MissingPartyInventory") : TEXT("MissingRuntimeActor")));
		return false;
	}

	if (bIsUnlocked)
	{
		const FText Message = GetEffectiveAlreadyUnlockedMessage();
		ShowFeedback(PartyPawn, Message);
		UE_LOG(LogGridWallLock, Log, TEXT("GridWallLock AlreadyUnlocked ObjectId=%s Message=%s"), *ObjectId.ToString(), *Message.ToString());
		return true;
	}

	UGridPartyInventoryComponent* InventoryComponent = PartyPawn->PartyInventoryComponent;
	if (InventoryComponent->HasCursorItem())
	{
		const FGridItemInstance CursorItem = InventoryComponent->GetCursorItem();
		UE_LOG(LogGridWallLock, Log, TEXT("GridWallLock CursorKeyAttempt ObjectId=%s CursorItemDefinitionId=%s"), *ObjectId.ToString(),
			*CursorItem.ItemDefinitionId.ToString());

		if (!IsAcceptedKey(CursorItem.ItemDefinitionId))
		{
			const FText Message = GetEffectiveMissingKeyMessage();
			ShowFeedback(PartyPawn, Message);
			UE_LOG(LogGridWallLock, Log, TEXT("GridWallLock UnlockFailed MissingKey ObjectId=%s AcceptedKeys=[%s] Cursor=%s Inventory=NotScanned"),
				*ObjectId.ToString(), *JoinItemDefinitionIds(AcceptedKeyDefinitionIds), *CursorItem.ItemDefinitionId.ToString());
			return false;
		}

		if (!AttachInsertedKeyVisual(PartyPawn, CursorItem))
		{
			UE_LOG(LogGridWallLock, Warning, TEXT("GridWallLock UnlockFailed MissingKey ObjectId=%s AcceptedKeys=[%s] Cursor=%s Reason=VisualSpawnFailed"),
				*ObjectId.ToString(), *JoinItemDefinitionIds(AcceptedKeyDefinitionIds), *CursorItem.ItemDefinitionId.ToString());
			return false;
		}

		if (CursorItem.Quantity > 1)
		{
			FGridItemInstance RemainingCursorItem = CursorItem;
			RemainingCursorItem.Quantity -= 1;
			InventoryComponent->SetCursorItem(RemainingCursorItem);
		}
		else
		{
			InventoryComponent->ClearCursorItem();
		}

		return CompleteUnlock(PartyPawn, CursorItem.ItemDefinitionId, TEXT("Cursor"));
	}

	const FText Message = FText::FromString(TEXT("Cette serrure nécessite une clé compatible."));
	ShowFeedback(PartyPawn, Message);
	UE_LOG(LogGridWallLock, Log, TEXT("GridWallLock DirectInteract InventoryAutoUnlockBlocked ObjectId=%s AcceptedKeys=[%s] Message=%s"), *ObjectId.ToString(),
		*JoinItemDefinitionIds(AcceptedKeyDefinitionIds), *Message.ToString());
	return false;
}

bool AGridWallLockActor::CanAcceptKeyDefinition(FName ItemDefinitionId) const
{
	return IsAcceptedKey(ItemDefinitionId);
}

bool AGridWallLockActor::TryUnlockWithContextItem(AGrimrockPartyPawn* PartyPawn, const FGridItemInstance& KeyItem)
{
	if (!PartyPawn || !PartyPawn->LevelRuntimeActor || !KeyItem.IsValid() || bIsUnlocked || !IsAcceptedKey(KeyItem.ItemDefinitionId))
	{
		return false;
	}

	if (!AttachInsertedKeyVisual(PartyPawn, KeyItem))
	{
		return false;
	}

	return CompleteUnlock(PartyPawn, KeyItem.ItemDefinitionId, TEXT("ContextAction"));
}

bool AGridWallLockActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
	const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	return PartyPawn && RuntimeActor && HitComponent == MeshComponent && RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn);
}

void AGridWallLockActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
{
	if (!CanInteract_Implementation(InstigatorPawn, HitComponent))
	{
		return;
	}

	TryInteractWithParty(GridInteractionUtils::ResolvePartyPawn(InstigatorPawn));
}

void AGridWallLockActor::InteractWithHit_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult)
{
	(void)HitResult;
	Interact_Implementation(InstigatorPawn, HitComponent);
}

EGridInteractionCursor AGridWallLockActor::GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const
{
	return HitComponent == MeshComponent ? EGridInteractionCursor::Use : EGridInteractionCursor::Default;
}

FText AGridWallLockActor::GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent != MeshComponent)
	{
		return FText::GetEmpty();
	}

	return bIsUnlocked ? GetEffectiveAlreadyUnlockedMessage() : GetEffectiveLockedMessage();
}

FText AGridWallLockActor::GetEffectiveLockedMessage() const
{
	return LockedMessage.IsEmpty() ? FText::FromString(TEXT("La serrure est verrouillée.")) : LockedMessage;
}

FText AGridWallLockActor::GetEffectiveUnlockSuccessMessage() const
{
	return UnlockedMessage.IsEmpty() ? FText::FromString(TEXT("La serrure s'ouvre avec un déclic métallique.")) : UnlockedMessage;
}

FText AGridWallLockActor::GetEffectiveAlreadyUnlockedMessage() const
{
	return FText::FromString(TEXT("La serrure est déjà déverrouillée."));
}

FText AGridWallLockActor::GetEffectiveMissingKeyMessage() const
{
	return MissingKeyMessage.IsEmpty() ? FText::FromString(TEXT("Il vous manque la clé adéquate.")) : MissingKeyMessage;
}

bool AGridWallLockActor::IsAcceptedKey(FName ItemDefinitionId) const
{
	return !ItemDefinitionId.IsNone() && AcceptedKeyDefinitionIds.Contains(ItemDefinitionId);
}

bool AGridWallLockActor::AttachInsertedKeyVisual(AGrimrockPartyPawn* PartyPawn, const FGridItemInstance& CursorItem)
{
	if (!PartyPawn || !PartyPawn->LevelRuntimeActor || !ItemAttachPoint)
	{
		return false;
	}

	UGridItemDefinitionAsset* ItemDefinition =
		PartyPawn->PartyInventoryComponent ? PartyPawn->PartyInventoryComponent->FindItemDefinition(CursorItem.ItemDefinitionId) : nullptr;
	if (!ItemDefinition)
	{
		ItemDefinition = PartyPawn->LevelRuntimeActor->ResolveRuntimeItemDefinition(CursorItem.ItemDefinitionId);
	}

	const FGuid VisualRuntimeObjectId = CursorItem.Quantity > 1 ? FGuid::NewGuid() : CursorItem.RuntimeObjectId;
	const int32 ItemIndex = AddContainedItem(CursorItem.ItemDefinitionId, ItemDefinition, nullptr, false, 1, VisualRuntimeObjectId);
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	AGridItemActor* InsertedKeyActor = GetContainedItemActor(ItemIndex);
	if (!InsertedKeyActor)
	{
		FGridContainedReceptacleItem FailedVisualItem;
		RemoveContainedItemAtIndex(ItemIndex, FailedVisualItem);
		return false;
	}

	InsertedKeyActor->AttachToComponent(ItemAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	InsertedKeyActor->SetActorRelativeTransform(FTransform::Identity);
	InsertedKeyActor->ConfigureAsAttachedItem();
	InsertedKeyActor->SetActorEnableCollision(false);
	if (InsertedKeyActor->MeshComponent)
	{
		InsertedKeyActor->MeshComponent->SetSimulatePhysics(false);
		InsertedKeyActor->MeshComponent->SetEnableGravity(false);
		InsertedKeyActor->MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetCanRemoveItem(false);
	ExecuteInsertionLinks();
	UE_LOG(LogGridWallLock, Log, TEXT("GridWallLock ReceptacleEventsEmitted ObjectId=%s Item=%s Events=ItemInserted,ItemChanged"), *ObjectId.ToString(),
		*CursorItem.ItemDefinitionId.ToString());
	return true;
}

bool AGridWallLockActor::CompleteUnlock(AGrimrockPartyPawn* PartyPawn, FName MatchingKeyId, const TCHAR* Source)
{
	if (!PartyPawn || !PartyPawn->LevelRuntimeActor || MatchingKeyId.IsNone())
	{
		return false;
	}

	bIsUnlocked = true;
	const FText Message = GetEffectiveUnlockSuccessMessage();
	ShowFeedback(PartyPawn, Message);

	UE_LOG(LogGridWallLock, Log, TEXT("GridWallLock UnlockSuccess ObjectId=%s MatchingKeyId=%s Source=%s Message=%s"), *ObjectId.ToString(),
		*MatchingKeyId.ToString(), Source, *Message.ToString());

	const bool bLinkExecuted = PartyPawn->LevelRuntimeActor->ExecuteLinksFromRuntimeObject(ObjectId, EGridObjectEvent::Activated);
	UE_LOG(LogGridWallLock, Log, TEXT("GridWallLock ActivatedEventEmitted ObjectId=%s MatchingKeyId=%s Source=%s LinkExecuted=%s"), *ObjectId.ToString(),
		*MatchingKeyId.ToString(), Source, bLinkExecuted ? TEXT("true") : TEXT("false"));
	return true;
}

void AGridWallLockActor::ShowFeedback(AGrimrockPartyPawn* PartyPawn, const FText& Message) const
{
	if (PartyPawn && PartyPawn->LevelRuntimeActor && !Message.IsEmpty())
	{
		PartyPawn->LevelRuntimeActor->ShowInteractionFeedback(Message);
	}
}
