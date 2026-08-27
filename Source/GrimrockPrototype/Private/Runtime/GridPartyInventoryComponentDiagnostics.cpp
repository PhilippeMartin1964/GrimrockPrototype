#include "Runtime/GridPartyInventoryComponent.h"

#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	int32 GridPartyInventoryDiagnosticsCountOccupiedSlots(const FGridCharacterInventoryState& CharacterState)
	{
		int32 OccupiedCount = 0;
		for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
		{
			if (!Slot.IsEmpty())
			{
				++OccupiedCount;
			}
		}
		return OccupiedCount;
	}

	const TCHAR* GridPartyInventoryDiagnosticsGetEquipmentSlotName(EGridEquipmentSlot Slot)
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

	const TCHAR* GridPartyInventoryDiagnosticsGetOwnerTypeName(EGridItemOwnerType OwnerType)
	{
		switch (OwnerType)
		{
			case EGridItemOwnerType::World:
				return TEXT("World");
			case EGridItemOwnerType::Receptacle:
				return TEXT("Receptacle");
			case EGridItemOwnerType::CharacterInventory:
				return TEXT("CharacterInventory");
			case EGridItemOwnerType::EquipmentSlot:
				return TEXT("EquipmentSlot");
			case EGridItemOwnerType::Cursor:
				return TEXT("Cursor");
			case EGridItemOwnerType::HeldBySelectedCharacter:
				return TEXT("HeldBySelectedCharacter");
			case EGridItemOwnerType::Removed:
				return TEXT("Removed");
			default:
				return TEXT("None");
		}
	}

	const TCHAR* GridPartyInventoryDiagnosticsGetItemTypeName(EGridItemType ItemType)
	{
		switch (ItemType)
		{
			case EGridItemType::Torch:
				return TEXT("Torch");
			case EGridItemType::Weapon:
				return TEXT("Weapon");
			case EGridItemType::Shield:
				return TEXT("Shield");
			case EGridItemType::Armor:
				return TEXT("Armor");
			case EGridItemType::Jewelry:
				return TEXT("Jewelry");
			case EGridItemType::Key:
				return TEXT("Key");
			case EGridItemType::Gem:
				return TEXT("Gem");
			case EGridItemType::Potion:
				return TEXT("Potion");
			case EGridItemType::Scroll:
				return TEXT("Scroll");
			case EGridItemType::Book:
				return TEXT("Book");
			case EGridItemType::Food:
				return TEXT("Food");
			case EGridItemType::Component:
				return TEXT("Component");
			case EGridItemType::Quest:
				return TEXT("Quest");
			case EGridItemType::Misc:
				return TEXT("Misc");
			default:
				return TEXT("None");
		}
	}

	FString GridPartyInventoryDiagnosticsGetEquipmentSlotsText(const TArray<EGridEquipmentSlot>& Slots)
	{
		if (Slots.Num() == 0)
		{
			return TEXT("None");
		}

		FString Result;
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (Index > 0)
			{
				Result += TEXT(",");
			}
			Result += GridPartyInventoryDiagnosticsGetEquipmentSlotName(Slots[Index]);
		}
		return Result;
	}

	bool GridPartyInventoryDiagnosticsIsHandSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
	}

	bool GridPartyInventoryDiagnosticsIsExcludedPaperDollSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::Talisman || Slot == EGridEquipmentSlot::QuickSlot1 || Slot == EGridEquipmentSlot::QuickSlot2;
	}

	bool GridPartyInventoryDiagnosticsIsNewPaperDollSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::Face || Slot == EGridEquipmentSlot::Shirt || Slot == EGridEquipmentSlot::Bracers ||
			Slot == EGridEquipmentSlot::Earring1 || Slot == EGridEquipmentSlot::Earring2;
	}

	bool GridPartyInventoryDiagnosticsLooksPotentiallyEquippable(const UGridItemDefinitionAsset* Definition)
	{
		if (!Definition)
		{
			return false;
		}

		switch (Definition->ItemType)
		{
			case EGridItemType::Torch:
			case EGridItemType::Weapon:
			case EGridItemType::Shield:
			case EGridItemType::Armor:
			case EGridItemType::Jewelry:
				return true;
			default:
				break;
		}

		return !Definition->EquippedMesh.IsNull();
	}

	FString GridPartyInventoryDiagnosticsGetEquipmentStatBonusText(const FGridEquipmentStatBonus& Bonus)
	{
		return FString::Printf(TEXT("STR=%d DEX=%d CON=%d INT=%d WIS=%d CHA=%d MaxHealth=%d MaxMana=%d CarryWeight=%.1f Armor=%d"), Bonus.StrengthBonus,
			Bonus.DexterityBonus, Bonus.ConstitutionBonus, Bonus.IntelligenceBonus, Bonus.WisdomBonus, Bonus.CharismaBonus, Bonus.MaxHealthBonus,
			Bonus.MaxManaBonus, Bonus.CarryWeightBonus, Bonus.ArmorBonus);
	}

	FString GridPartyInventoryDiagnosticsGetDamageResistanceSetText(const FGridDamageResistanceSet& Resistances)
	{
		return FString::Printf(TEXT("Physical=%d Fire=%d Ice=%d Lightning=%d Poison=%d Holy=%d Necrotic=%d Arcane=%d"), Resistances.PhysicalResistance,
			Resistances.FireResistance, Resistances.IceResistance, Resistances.LightningResistance, Resistances.PoisonResistance, Resistances.HolyResistance,
			Resistances.NecroticResistance, Resistances.ArcaneResistance);
	}

	void GridPartyInventoryDiagnosticsForEachEquipmentItem(
		const FGridCharacterEquipmentState& EquipmentState, TFunctionRef<void(EGridEquipmentSlot, const FGridItemInstance&)> Visitor)
	{
		const EGridEquipmentSlot Slots[] = { EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand, EGridEquipmentSlot::Head, EGridEquipmentSlot::Chest,
			EGridEquipmentSlot::Legs, EGridEquipmentSlot::Feet, EGridEquipmentSlot::Amulet, EGridEquipmentSlot::Ring1, EGridEquipmentSlot::Ring2,
			EGridEquipmentSlot::Shoulders, EGridEquipmentSlot::Gloves, EGridEquipmentSlot::Belt, EGridEquipmentSlot::Cloak, EGridEquipmentSlot::Talisman,
			EGridEquipmentSlot::QuickSlot1, EGridEquipmentSlot::QuickSlot2, EGridEquipmentSlot::Face, EGridEquipmentSlot::Shirt, EGridEquipmentSlot::Bracers,
			EGridEquipmentSlot::Earring1, EGridEquipmentSlot::Earring2 };

		for (const EGridEquipmentSlot Slot : Slots)
		{
			if (const FGridItemInstance* Item = EquipmentState.GetSlot(Slot))
			{
				Visitor(Slot, *Item);
			}
		}
	}
}

FString UGridPartyInventoryComponent::GetEquipmentDiagnosticsForCharacter(int32 CharacterIndex) const
{
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return TEXT("    Equipment: None");
	}

	const FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
	TArray<FString> OccupiedSlots;
	GridPartyInventoryDiagnosticsForEachEquipmentItem(EquipmentState,
		[&OccupiedSlots](EGridEquipmentSlot Slot, const FGridItemInstance& Item)
		{
			if (Item.IsValid())
			{
				OccupiedSlots.Add(FString::Printf(TEXT("%s=%s"), GridPartyInventoryDiagnosticsGetEquipmentSlotName(Slot), *Item.ItemDefinitionId.ToString()));
			}
		});

	return OccupiedSlots.Num() > 0 ? FString::Printf(TEXT("    Equipment: %s"), *FString::Join(OccupiedSlots, TEXT(" "))) : TEXT("    Equipment: None");
}


FString UGridPartyInventoryComponent::GetPartyInventoryDiagnostics() const
{
	FString Result;
	Result += TEXT("GridPartyInventory Diagnostics\n");
	Result += FString::Printf(TEXT("ActiveCharacters=%d MaxActiveCharacters=%d SelectedCharacter=%d CursorItem=%s CharacterPool=%d\n"),
		PartyInventoryState.ActiveCharacters.Num(), PartyInventoryState.MaxActiveCharacters, PartyInventoryState.SelectedCharacterIndex,
		HasCursorItem() ? *PartyInventoryState.CursorItem.ItemDefinitionId.ToString() : TEXT("None"), PartyInventoryState.CharacterPool.Num());

	for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
		FGridInventoryCharacterSummary Summary;
		GetCharacterSummary(CharacterIndex, Summary);
		Result += FString::Printf(TEXT("[%d] Name=%s Class=%s Level=%d Slots=%d/%d Weight=%.1f/%.1f Overloaded=%s\n"), CharacterIndex,
			*CharacterState.DisplayName.ToString(), *CharacterState.ClassId.ToString(), CharacterState.Level,
			GridPartyInventoryDiagnosticsCountOccupiedSlots(CharacterState), CharacterState.InventorySlots.Num(), Summary.CurrentWeight, Summary.MaxWeight,
			Summary.bOverloaded ? TEXT("true") : TEXT("false"));
		Result += GetEquipmentDiagnosticsForCharacter(CharacterIndex);
		Result += TEXT("\n");

		bool bWroteInventoryHeader = false;
		for (int32 SlotIndex = 0; SlotIndex < CharacterState.InventorySlots.Num(); ++SlotIndex)
		{
			const FGridInventorySlot& Slot = CharacterState.InventorySlots[SlotIndex];
			if (Slot.IsEmpty())
			{
				continue;
			}

			if (!bWroteInventoryHeader)
			{
				Result += TEXT("    Inventory:\n");
				bWroteInventoryHeader = true;
			}

			Result += FString::Printf(TEXT("      [%d] Item=%s Qty=%d Weight=%.1f Owner=%s\n"), SlotIndex, *Slot.Item.ItemDefinitionId.ToString(),
				Slot.Item.Quantity, Slot.Item.Weight, GridPartyInventoryDiagnosticsGetOwnerTypeName(Slot.Item.OwnerType));
		}
	}

	return Result;
}

void UGridPartyInventoryComponent::LogPartyInventoryDiagnostics() const
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *GetPartyInventoryDiagnostics());
}

FString UGridPartyInventoryComponent::GetItemDefinitionDiagnostics() const
{
	FString Result;
	Result += TEXT("GridItemDefinition Diagnostics\n");
	Result += FString::Printf(TEXT("RuntimeDefinitions=%d\n"), RuntimeItemDefinitionsById.Num());

	TArray<FName> DefinitionIds;
	RuntimeItemDefinitionsById.GetKeys(DefinitionIds);
	DefinitionIds.Sort(FNameLexicalLess());
	for (int32 Index = 0; Index < DefinitionIds.Num(); ++Index)
	{
		const FName DefinitionId = DefinitionIds[Index];
		const TObjectPtr<UGridItemDefinitionAsset>* DefinitionEntry = RuntimeItemDefinitionsById.Find(DefinitionId);
		const UGridItemDefinitionAsset* Definition = DefinitionEntry ? DefinitionEntry->Get() : nullptr;
		if (!Definition)
		{
			Result += FString::Printf(TEXT("[%d] Id=%s Asset=None Warning=NullDefinition\n"), Index, *DefinitionId.ToString());
			continue;
		}

		Result += FString::Printf(TEXT("[%d] Asset=%s Id=%s Type=%s Weight=%.1f Slots=%s\n"), Index, *Definition->GetName(),
			*Definition->ItemDefinitionId.ToString(), GridPartyInventoryDiagnosticsGetItemTypeName(Definition->ItemType), Definition->Weight,
			*GridPartyInventoryDiagnosticsGetEquipmentSlotsText(Definition->CompatibleEquipmentSlots));
	}

	return Result;
}

void UGridPartyInventoryComponent::LogItemDefinitionDiagnostics() const
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *GetItemDefinitionDiagnostics());
}

void UGridPartyInventoryComponent::LogEquipmentCompatibilityDiagnostics() const
{
	UE_LOG(LogTemp, Log, TEXT("GridEquipmentCompatibility Diagnostics RuntimeDefinitions=%d"), RuntimeItemDefinitionsById.Num());

	TArray<FName> DefinitionIds;
	RuntimeItemDefinitionsById.GetKeys(DefinitionIds);
	DefinitionIds.Sort(FNameLexicalLess());

	int32 PotentiallyEquippableWithoutSlotsCount = 0;
	int32 LightWithoutHandSlotCount = 0;
	int32 ExcludedPaperDollSlotCount = 0;
	int32 NewPaperDollSlotUsageCount = 0;

	for (const FName DefinitionId : DefinitionIds)
	{
		const TObjectPtr<UGridItemDefinitionAsset>* DefinitionEntry = RuntimeItemDefinitionsById.Find(DefinitionId);
		const UGridItemDefinitionAsset* Definition = DefinitionEntry ? DefinitionEntry->Get() : nullptr;
		if (!Definition)
		{
			UE_LOG(LogTemp, Warning, TEXT("GridEquipmentCompatibility Item=%s Warning=NullDefinition"), *DefinitionId.ToString());
			continue;
		}

		const FString SlotsText = GridPartyInventoryDiagnosticsGetEquipmentSlotsText(Definition->CompatibleEquipmentSlots);
		if (Definition->CompatibleEquipmentSlots.Num() == 0 && GridPartyInventoryDiagnosticsLooksPotentiallyEquippable(Definition))
		{
			++PotentiallyEquippableWithoutSlotsCount;
			UE_LOG(LogTemp, Warning, TEXT("GridEquipmentCompatibility Item=%s Warning=PotentiallyEquippableWithoutSlots Type=%s Slots=%s"),
				*Definition->ItemDefinitionId.ToString(), GridPartyInventoryDiagnosticsGetItemTypeName(Definition->ItemType), *SlotsText);
		}

		if (Definition->bCanEmitLight)
		{
			bool bHasHandSlot = false;
			for (const EGridEquipmentSlot Slot : Definition->CompatibleEquipmentSlots)
			{
				if (GridPartyInventoryDiagnosticsIsHandSlot(Slot))
				{
					bHasHandSlot = true;
					break;
				}
			}

			if (!bHasHandSlot)
			{
				++LightWithoutHandSlotCount;
				UE_LOG(LogTemp, Warning, TEXT("GridEquipmentCompatibility Item=%s Warning=LightWithoutMainHandOrOffHand Slots=%s"),
					*Definition->ItemDefinitionId.ToString(), *SlotsText);
			}
		}

		for (const EGridEquipmentSlot Slot : Definition->CompatibleEquipmentSlots)
		{
			if (GridPartyInventoryDiagnosticsIsExcludedPaperDollSlot(Slot))
			{
				++ExcludedPaperDollSlotCount;
				UE_LOG(LogTemp, Warning, TEXT("GridEquipmentCompatibility Item=%s Warning=PaperDollExcludedSlot Slot=%s Slots=%s"),
					*Definition->ItemDefinitionId.ToString(), GridPartyInventoryDiagnosticsGetEquipmentSlotName(Slot), *SlotsText);
			}
			else if (GridPartyInventoryDiagnosticsIsNewPaperDollSlot(Slot))
			{
				++NewPaperDollSlotUsageCount;
				UE_LOG(LogTemp, Log, TEXT("GridEquipmentCompatibility Item=%s UsesNewPaperDollSlot=%s Slots=%s"), *Definition->ItemDefinitionId.ToString(),
					GridPartyInventoryDiagnosticsGetEquipmentSlotName(Slot), *SlotsText);
			}
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("GridEquipmentCompatibility Summary PotentiallyEquippableWithoutSlots=%d LightWithoutHandSlot=%d ExcludedPaperDollSlots=%d NewPaperDollSlotUses=%d"),
		PotentiallyEquippableWithoutSlotsCount, LightWithoutHandSlotCount, ExcludedPaperDollSlotCount, NewPaperDollSlotUsageCount);
}

void UGridPartyInventoryComponent::LogSelectedCharacterEquipmentStatBonusDiagnostics() const
{
	const int32 CharacterIndex = PartyInventoryState.SelectedCharacterIndex;
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridEquipmentStatBonus Diagnostics Character=%d Result=false Reason=InvalidCharacterOrEquipment"), CharacterIndex);
		return;
	}

	const FGridEquipmentStatBonus TotalBonus = ComputeCharacterEquipmentStatBonus(CharacterIndex);
	UE_LOG(LogTemp, Log, TEXT("GridEquipmentStatBonus Diagnostics Character=%d Total=%s"), CharacterIndex,
		*GridPartyInventoryDiagnosticsGetEquipmentStatBonusText(TotalBonus));

	GridPartyInventoryDiagnosticsForEachEquipmentItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
		[this](EGridEquipmentSlot Slot, const FGridItemInstance& Item)
		{
			if (!Item.IsValid())
			{
				return;
			}

			const UGridItemDefinitionAsset* Definition = FindItemDefinition(Item.ItemDefinitionId);
			if (!Definition)
			{
				UE_LOG(LogTemp, Warning, TEXT("GridEquipmentStatBonus Item=%s Slot=%s Warning=MissingDefinition"), *Item.ItemDefinitionId.ToString(),
					GridPartyInventoryDiagnosticsGetEquipmentSlotName(Slot));
				return;
			}

			if (!Definition->EquipmentStatBonus.HasAnyBonus())
			{
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("GridEquipmentStatBonus Item=%s Slot=%s Bonus=%s"), *Definition->ItemDefinitionId.ToString(),
				GridPartyInventoryDiagnosticsGetEquipmentSlotName(Slot),
				*GridPartyInventoryDiagnosticsGetEquipmentStatBonusText(Definition->EquipmentStatBonus));
		});
}

void UGridPartyInventoryComponent::LogSelectedCharacterResistanceDiagnostics() const
{
	const int32 CharacterIndex = PartyInventoryState.SelectedCharacterIndex;
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridDamageResistance Diagnostics Character=%d Result=false Reason=InvalidCharacterOrEquipment"), CharacterIndex);
		return;
	}

	const FGridDamageResistanceSet TotalResistances = ComputeCharacterEquipmentResistances(CharacterIndex);
	UE_LOG(LogTemp, Log, TEXT("GridDamageResistance Diagnostics Character=%d Total=%s"), CharacterIndex,
		*GridPartyInventoryDiagnosticsGetDamageResistanceSetText(TotalResistances));

	GridPartyInventoryDiagnosticsForEachEquipmentItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
		[this](EGridEquipmentSlot Slot, const FGridItemInstance& Item)
		{
			if (!Item.IsValid())
			{
				return;
			}

			const UGridItemDefinitionAsset* Definition = FindItemDefinition(Item.ItemDefinitionId);
			if (!Definition)
			{
				UE_LOG(LogTemp, Warning, TEXT("GridDamageResistance Item=%s Slot=%s Warning=MissingDefinition"), *Item.ItemDefinitionId.ToString(),
					GridPartyInventoryDiagnosticsGetEquipmentSlotName(Slot));
				return;
			}

			if (Definition->EquipmentResistanceBonus.IsEmpty())
			{
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("GridDamageResistance Item=%s Slot=%s Resistances=%s"), *Definition->ItemDefinitionId.ToString(),
				GridPartyInventoryDiagnosticsGetEquipmentSlotName(Slot),
				*GridPartyInventoryDiagnosticsGetDamageResistanceSetText(Definition->EquipmentResistanceBonus));
		});
}

void UGridPartyInventoryComponent::LogInventoryOwnershipDiagnostics() const
{
	FString Error;
	if (ValidateInventoryOwnership(Error))
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory Ownership OK"));
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("GridInventory Ownership ERROR %s"), *Error);
}
