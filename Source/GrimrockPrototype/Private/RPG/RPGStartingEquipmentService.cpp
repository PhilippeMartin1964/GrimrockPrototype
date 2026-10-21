#include "RPG/RPGStartingEquipmentService.h"

#include "RPG/RPGClassAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"

bool FRPGStartingEquipmentService::TryBuildKit(
	const URPGClassAsset* ClassDefinition,
	FName KitId,
	FGridCharacterInventoryState& InOutCharacter,
	FGridCharacterEquipmentState& InOutEquipment,
	TArray<UGridItemDefinitionAsset*>& OutDefinitions,
	FString& OutError)
{
	OutDefinitions.Reset();
	OutError.Reset();
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
	TArray<UGridItemDefinitionAsset*> CandidateDefinitions;
	TMap<FName, UGridItemDefinitionAsset*> DefinitionById;
	bool bTwoHandedHandReserved = false;

	for (const FRPGStartingEquipmentEntry& Entry : Kit->Items)
	{
		UGridItemDefinitionAsset* Definition = Entry.ItemDefinition.Get();
		if (!IsValid(Definition) || Definition->ItemDefinitionId.IsNone() ||
			Entry.ItemLevel < 0 || Entry.ItemLevel > 20 || Entry.Quantity < 1)
		{
			OutError = TEXT("Starting kit contains an invalid item definition, level or quantity.");
			return false;
		}
		const FName ItemId = Definition->ItemDefinitionId;
		if (UGridItemDefinitionAsset** Previous = DefinitionById.Find(ItemId))
		{
			if (*Previous != Definition)
			{
				OutError = TEXT("Starting kit maps one item ID to multiple definitions.");
				return false;
			}
		}
		else
		{
			DefinitionById.Add(ItemId, Definition);
			CandidateDefinitions.Add(Definition);
		}
		if ((!Definition->bStackable && Entry.Quantity != 1) ||
			(Definition->bStackable && Entry.Quantity > FMath::Max(1, Definition->MaxStackSize)) ||
			(Entry.TargetSlot != EGridEquipmentSlot::None && Entry.Quantity != 1))
		{
			OutError = TEXT("Starting kit quantity is incompatible with the item or equipment slot.");
			return false;
		}

		int32 FixedLevel = 0;
		float FixedWeight = Definition->Weight;
		if (Definition->bUseWeaponCatalogue)
		{
			FixedLevel = Entry.ItemLevel == 0 ? Definition->DefaultWeaponItemLevel : Entry.ItemLevel;
			const FGridWeaponLevelProfile* Profile = nullptr;
			if (!Definition->TryGetWeaponCatalogueProfile(FixedLevel, Profile) || !Profile)
			{
				OutError = FString::Printf(TEXT("Starting kit item %s has no catalogue level %d."), *ItemId.ToString(), FixedLevel);
				return false;
			}
			FixedWeight = Profile->WeightKg;
		}
		else if (Entry.ItemLevel != 0)
		{
			OutError = FString::Printf(TEXT("Starting kit item %s is Legacy and has no levels."), *ItemId.ToString());
			return false;
		}

		EGridEquipmentSlot PhysicalSlot = Entry.TargetSlot;
		if (PhysicalSlot != EGridEquipmentSlot::None)
		{
			if (!Definition->CanEquipToSlot(PhysicalSlot) || !CandidateEquipment.GetMutableSlot(PhysicalSlot))
			{
				OutError = FString::Printf(TEXT("Starting kit item %s is incompatible with slot %d."), *ItemId.ToString(), int32(PhysicalSlot));
				return false;
			}
			const bool bTwoHanded = Definition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded;
			if (bTwoHanded && PhysicalSlot != EGridEquipmentSlot::MainHand && PhysicalSlot != EGridEquipmentSlot::OffHand)
			{
				OutError = TEXT("A two-handed item must occupy the hands.");
				return false;
			}
			if (bTwoHanded)
			{
				PhysicalSlot = EGridEquipmentSlot::MainHand;
			}
			if ((bTwoHanded && (CandidateEquipment.MainHand.IsValid() || CandidateEquipment.OffHand.IsValid())) ||
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
		Item.DisplayName = Definition->DisplayName;
		Item.Quantity = Entry.Quantity;
		Item.Weight = FixedWeight;
		Item.OwnerGuid = Candidate.CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.bLightsEnabled = Definition->IsLightEnabledByDefault();

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
			if (Definition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded)
			{
				bTwoHandedHandReserved = true;
			}
		}
	}

	Candidate.StartingEquipmentKitId = KitId;
	InOutCharacter = MoveTemp(Candidate);
	InOutEquipment = MoveTemp(CandidateEquipment);
	OutDefinitions = MoveTemp(CandidateDefinitions);
	return true;
}
