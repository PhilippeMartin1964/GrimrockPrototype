#include "RPG/RPGStartingEquipmentService.h"

#include "RPG/RPGClassAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridStartingKitEquipmentPolicyReadModel.h"

bool FRPGStartingEquipmentService::TryBuildKit(
	const URPGClassAsset* ClassDefinition,
	FName KitId,
	FGridCharacterInventoryState& InOutCharacter,
	FGridCharacterEquipmentState& InOutEquipment,
	FString& OutError,
	const UGridItemSubsystem* CanonicalRegistry)
{
	OutError.Reset();
	if (!IsValid(CanonicalRegistry) || !CanonicalRegistry->IsConfigured())
	{
		OutError = TEXT("Starting kit requires the configured canonical ItemSystem.");
		return false;
	}
	if (KitId.IsNone())
	{
		return true; // Existing unconfigured class behaviour stays unchanged.
	}
	if (!IsValid(ClassDefinition) || !InOutCharacter.CharacterId.IsValid())
	{
		OutError = TEXT("Invalid class or character for starting equipment.");
		return false;
	}
	const FRPGStartingEquipmentKit* Kit = ClassDefinition->FindStartingEquipmentKit(KitId);
	if (!Kit || Kit->Items.IsEmpty())
	{
		OutError = FString::Printf(TEXT("Starting kit %s is not defined."), *KitId.ToString());
		return false;
	}

	// Stage everything locally. Partial grants must never leak on failure.
	FGridCharacterInventoryState Candidate = InOutCharacter;
	FGridCharacterEquipmentState CandidateEquipment = InOutEquipment;
	bool bTwoHandedHandReserved = false;

	for (const FRPGStartingEquipmentEntry& Entry : Kit->Items)
	{
		const FName ItemId = Entry.ItemId;
		if (!CanonicalRegistry->ContainsItem(ItemId))
		{
			OutError = TEXT("Starting kit item is not registered in the canonical ItemSystem.");
			return false;
		}
		if (Entry.ItemLevel < 0 || Entry.ItemLevel > 20 || Entry.Quantity < 1)
		{
			OutError = TEXT("Starting kit contains an invalid item definition, canonical ID, level or quantity.");
			return false;
		}

		const int32 FixedLevel = Entry.ItemLevel;
		FGridResolvedItem NativeItem;
		if (!CanonicalRegistry->ResolveItem(ItemId, FixedLevel, NativeItem) || !NativeItem.IsValid() ||
			!FMath::IsFinite(NativeItem.WeightKg) || NativeItem.WeightKg < 0.0f ||
			(!NativeItem.bIsArmament && FixedLevel != 0))
		{
			OutError = TEXT("Starting kit canonical item requires an explicit valid physical level.");
			return false;
		}
		const int32 StackCapacity = FMath::Max(1, NativeItem.MaxStack);
		const bool bStackable = StackCapacity > 1;
		if ((!bStackable && Entry.Quantity != 1) ||
			(bStackable && Entry.Quantity > StackCapacity) ||
			(Entry.TargetSlot != EGridEquipmentSlot::None && Entry.Quantity != 1))
		{
			OutError = TEXT("Starting kit quantity is incompatible with the item or equipment slot.");
			return false;
		}

		EGridEquipmentSlot PhysicalSlot = Entry.TargetSlot;
		bool bEntryTwoHanded = false;
		if (PhysicalSlot != EGridEquipmentSlot::None)
		{
			const FGridStartingKitEquipmentPolicyReadView Hand =
				FGridStartingKitEquipmentPolicyReadModel::ResolveCanonical(CanonicalRegistry, ItemId, FixedLevel, PhysicalSlot);
			if (!Hand.bResolved || !CandidateEquipment.GetMutableSlot(Hand.PhysicalSlot))
			{
				OutError = FString::Printf(TEXT("Starting kit item %s is incompatible with slot %d."), *ItemId.ToString(), int32(PhysicalSlot));
				return false; // No partial kit commit.
			}
			bEntryTwoHanded = Hand.bTwoHanded;
			PhysicalSlot = Hand.PhysicalSlot;
			if ((bEntryTwoHanded && (CandidateEquipment.MainHand.IsValid() || CandidateEquipment.OffHand.IsValid())) ||
				(bTwoHandedHandReserved &&
					(PhysicalSlot == EGridEquipmentSlot::MainHand || PhysicalSlot == EGridEquipmentSlot::OffHand)))
			{
				OutError = TEXT("Starting kit hands conflict with a two-handed item.");
				return false;
			}
		}

		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemId;
		Item.ItemLevel = FixedLevel;
		Item.Quantity = Entry.Quantity;
		Item.OwnerGuid = Candidate.CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.bLightsEnabled = NativeItem.LightEmitter.bDefaultEnabled;

		if (PhysicalSlot == EGridEquipmentSlot::None)
		{
			FGridInventorySlot* Destination = Candidate.InventorySlots.FindByPredicate(
				[](const FGridInventorySlot& Slot) { return Slot.IsEmpty(); });
			if (!Destination)
			{
				OutError = TEXT("Not enough bag slots for the starting kit.");
				return false;
			}
			Item.OwnerType = EGridItemOwnerType::CharacterInventory;
			Item.EquipmentSlot = EGridEquipmentSlot::None;
			Destination->bOccupied = true;
			Destination->Item = Item;
		}
		else
		{
			FGridItemInstance* Equipped = CandidateEquipment.GetMutableSlot(PhysicalSlot);
			if (!Equipped || Equipped->IsValid())
			{
				OutError = TEXT("Two starting items target the same equipment slot.");
				return false;
			}
			Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
			Item.EquipmentSlot = PhysicalSlot;
			*Equipped = Item;
			if (bEntryTwoHanded)
			{
				bTwoHandedHandReserved = true;
			}
		}
	}

	Candidate.StartingEquipmentKitId = KitId;
	InOutCharacter = MoveTemp(Candidate);
	InOutEquipment = MoveTemp(CandidateEquipment);
	return true;
}
