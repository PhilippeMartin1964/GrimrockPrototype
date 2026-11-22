#include "Runtime/GridPartyInventoryComponent.h"

#include "GridEquipmentSlotUtils.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridInventory, Log, All);

namespace
{
	bool IsCanonicalWorldTransferItem(const UGridPartyInventoryComponent* Inventory, FName ItemId)
	{
		const UWorld* World = Inventory->GetWorld();
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		const UGridItemSubsystem* Registry = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
		return Registry && Registry->ContainsItem(ItemId);
	}
}

bool UGridPartyInventoryComponent::TryExtractOneEquippedItemForWorldTransfer(
	int32 CharacterIndex, EGridEquipmentSlot SourceSlot, FName ExpectedItemDefinitionId, FGridItemInstance& OutWorldItem)
{
	OutWorldItem = FGridItemInstance();
	EnsureEquipmentCountMatchesActiveCharacters();
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex) ||
		!GridEquipmentSlotUtils::IsHandSlot(SourceSlot) || ExpectedItemDefinitionId.IsNone())
	{
		return false;
	}

	const EGridEquipmentSlot PhysicalSource = ResolveEquipmentSourceSlot(CharacterIndex, SourceSlot);
	FGridItemInstance* EquippedItem = PartyInventoryState.ActiveEquipment[CharacterIndex].GetMutableSlot(PhysicalSource);
	if (!EquippedItem || !EquippedItem->IsValid() || EquippedItem->ItemDefinitionId != ExpectedItemDefinitionId)
	{
		return false;
	}

	if (!CanAcceptSingleInventoryItem(*EquippedItem) ||
		(IsCanonicalWorldTransferItem(this, EquippedItem->ItemDefinitionId) &&
			!CanEquipItemToSlot(CharacterIndex, *EquippedItem, PhysicalSource)))
	{
		return false; // An invalid source must remain owned, not become a world item.
	}

	OutWorldItem = *EquippedItem;
	OutWorldItem.Quantity = 1;
	OutWorldItem.OwnerType = EGridItemOwnerType::World;
	OutWorldItem.OwnerGuid = FGuid();
	OutWorldItem.OwnerCharacterIndex = INDEX_NONE;
	OutWorldItem.EquipmentSlot = EGridEquipmentSlot::None;

	const int32 QuantityBefore = FMath::Max(1, EquippedItem->Quantity);
	if (QuantityBefore > 1)
	{
		--EquippedItem->Quantity;
		OutWorldItem.RuntimeObjectId = FGuid::NewGuid();
	}
	else
	{
		*EquippedItem = FGridItemInstance();
	}
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogGridInventory, Verbose, TEXT("GridInventory EquipmentWorldTransfer Extract Character=%d Slot=%s Item=%s RuntimeId=%s Quantity=%d->%d Result=true"),
		CharacterIndex, GridEquipmentSlotUtils::GetLogName(SourceSlot), *OutWorldItem.ItemDefinitionId.ToString(),
		*OutWorldItem.RuntimeObjectId.ToString(), QuantityBefore, QuantityBefore - 1);
	return true;
}

bool UGridPartyInventoryComponent::TryRestoreExtractedItemToEquipment(int32 CharacterIndex, EGridEquipmentSlot TargetSlot, const FGridItemInstance& WorldItem)
{
	EnsureEquipmentCountMatchesActiveCharacters();
	if (!CanAcceptSingleInventoryItem(WorldItem) || WorldItem.Quantity != 1 ||
		WorldItem.OwnerType != EGridItemOwnerType::World || !IsValidCharacterIndex(CharacterIndex) ||
		!PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex) || !GridEquipmentSlotUtils::IsHandSlot(TargetSlot))
	{
		return false;
	}

	const bool bTwoHanded = IsTwoHandedItem(WorldItem);
	const EGridEquipmentSlot ActualTarget = bTwoHanded ? EGridEquipmentSlot::MainHand : TargetSlot;
	if ((IsCanonicalWorldTransferItem(this, WorldItem.ItemDefinitionId) &&
		!CanEquipItemToSlot(CharacterIndex, WorldItem, TargetSlot)) ||
		(bTwoHanded && PartyInventoryState.ActiveEquipment[CharacterIndex].OffHand.IsValid()) ||
		(ActualTarget == EGridEquipmentSlot::OffHand && IsTwoHandedEquipped(CharacterIndex)))
	{
		return false;
	}
	FGridItemInstance* EquippedItem = PartyInventoryState.ActiveEquipment[CharacterIndex].GetMutableSlot(ActualTarget);
	if (!EquippedItem)
	{
		return false;
	}

	const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (EquippedItem->IsValid())
	{
		if (EquippedItem->ItemDefinitionId != WorldItem.ItemDefinitionId ||
			EquippedItem->ItemLevel != WorldItem.ItemLevel ||
			EquippedItem->RuntimeObjectId == WorldItem.RuntimeObjectId ||
			EquippedItem->Quantity == MAX_int32 ||
			EquippedItem->bLightsEnabled != WorldItem.bLightsEnabled ||
			EquippedItem->AbsorbedEquipmentPhysicalArmor != WorldItem.AbsorbedEquipmentPhysicalArmor ||
			EquippedItem->AbsorbedEquipmentMagicalArmor != WorldItem.AbsorbedEquipmentMagicalArmor ||
			EquippedItem->ReadableContentAsset != WorldItem.ReadableContentAsset ||
			EquippedItem->ReadableContentId != WorldItem.ReadableContentId ||
			!EquippedItem->ReadTitleOverride.EqualTo(WorldItem.ReadTitleOverride) ||
			!EquippedItem->ReadTextOverride.EqualTo(WorldItem.ReadTextOverride))
		{
			return false;
		}
		FGridItemInstance Merged = *EquippedItem;
		++Merged.Quantity;
		if (!CanAcceptSingleInventoryItem(Merged)) return false;
		EquippedItem->Quantity = Merged.Quantity;
	}
	else
	{
		*EquippedItem = WorldItem;
		EquippedItem->OwnerType = EGridItemOwnerType::EquipmentSlot;
		EquippedItem->OwnerGuid = CharacterState.CharacterId;
		EquippedItem->OwnerCharacterIndex = CharacterIndex;
		EquippedItem->EquipmentSlot = ActualTarget;
	}
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogGridInventory, Verbose, TEXT("GridInventory EquipmentWorldTransfer Restored Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex,
		GridEquipmentSlotUtils::GetLogName(TargetSlot), *WorldItem.ItemDefinitionId.ToString(), *WorldItem.RuntimeObjectId.ToString());
	return true;
}
