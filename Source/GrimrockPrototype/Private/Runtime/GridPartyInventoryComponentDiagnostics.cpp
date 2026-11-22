#include "Runtime/GridPartyInventoryComponent.h"

#include "GridEquipmentSlotUtils.h"
#include "GridInventoryDefinitionDiagnosticReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "CoreGlobals.h"
#include "GridInventoryDiagnosticWeightReadModel.h"
#include "GridEquipmentContributionReadModel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridInventory, Log, All);

namespace
{
	const UGridItemSubsystem* GridPartyInventoryDiagnosticsRegistry(const UGridPartyInventoryComponent* Inventory)
	{
		const UWorld* World = IsValid(Inventory) ? Inventory->GetWorld() : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	}

    const UGridItemDefinitionAsset* ResolveDiagnosticHistoricalDefinitionForAutomation(
        const UGridPartyInventoryComponent* Inventory, const UGridItemSubsystem* Registry, FName ItemId)
    {
#if WITH_DEV_AUTOMATION_TESTS
        if (!Registry && GIsAutomationTesting && IsValid(Inventory))
        {
            return Inventory->FindItemDefinition(ItemId);
        }
#else
        (void)Inventory;
        (void)Registry;
        (void)ItemId;
#endif
        return nullptr;
    }

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
			Result += GridEquipmentSlotUtils::GetLogName(Slots[Index]);
		}
		return Result;
	}

	bool GridPartyInventoryDiagnosticsIsExcludedPaperDollSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::Talisman || Slot == EGridEquipmentSlot::QuickSlot1 || Slot == EGridEquipmentSlot::QuickSlot2;
	}

	bool GridPartyInventoryDiagnosticsIsNewPaperDollSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::Shirt || Slot == EGridEquipmentSlot::Bracers;
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
}

FString UGridPartyInventoryComponent::GetEquipmentDiagnosticsForCharacter(int32 CharacterIndex) const
{
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return TEXT("    Equipment: None");
	}

	const FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
	TArray<FString> OccupiedSlots;
	GridEquipmentSlotUtils::ForEachEquipmentItem(EquipmentState,
		[&OccupiedSlots](EGridEquipmentSlot Slot, const FGridItemInstance& Item)
		{
			if (Item.IsValid())
			{
				OccupiedSlots.Add(FString::Printf(TEXT("%s=%s"), GridEquipmentSlotUtils::GetLogName(Slot), *Item.ItemDefinitionId.ToString()));
			}
		});

	return OccupiedSlots.Num() > 0 ? FString::Printf(TEXT("    Equipment: %s"), *FString::Join(OccupiedSlots, TEXT(" "))) : TEXT("    Equipment: None");
}

FString UGridPartyInventoryComponent::GetPartyInventoryDiagnostics() const
{
	const UGridItemSubsystem* Registry = GridPartyInventoryDiagnosticsRegistry(this);
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

			const FString UnitWeightText = FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(Registry, Slot.Item);
			Result += FString::Printf(TEXT("      [%d] Item=%s Qty=%d Weight=%s Owner=%s\n"), SlotIndex, *Slot.Item.ItemDefinitionId.ToString(),
				Slot.Item.Quantity, *UnitWeightText, GridPartyInventoryDiagnosticsGetOwnerTypeName(Slot.Item.OwnerType));
		}
	}

	return Result;
}

void UGridPartyInventoryComponent::LogPartyInventoryDiagnostics() const
{
	UE_LOG(LogGridInventory, Log, TEXT("%s"), *GetPartyInventoryDiagnostics());
}

FString UGridPartyInventoryComponent::GetItemDefinitionDiagnostics() const
{
	TArray<FGridInventoryDefinitionDiagnosticRow> Rows;
	FString Result = TEXT("GridItemDefinition Diagnostics\n");
	if (!FGridInventoryDefinitionDiagnosticReadModel::Build(GridPartyInventoryDiagnosticsRegistry(this), Rows))
		return Result + TEXT("ItemSystem=UNCONFIGURED\n");
	Result += FString::Printf(TEXT("CanonicalDefinitions=%d\n"), Rows.Num());
	for (const auto& Row : Rows)
	{
		if (!Row.bResolved)
		{
			Result += FString::Printf(TEXT("Id=%s Weight=INVALID Warning=CanonicalResolveFailed\n"), *Row.ItemId.ToString());
			continue;
		}
		Result += FString::Printf(TEXT("Id=%s Kind=%s Weight=%s Slots=%s\n"),
			*Row.ItemId.ToString(), Row.bArmament ? TEXT("ARM") : (Row.bAmmo ? TEXT("Ammo") : TEXT("NonARM")),
			*Row.WeightText, *GridPartyInventoryDiagnosticsGetEquipmentSlotsText(Row.Slots));
	}
	return Result;
}

void UGridPartyInventoryComponent::LogItemDefinitionDiagnostics() const
{
	UE_LOG(LogGridInventory, Log, TEXT("%s"), *GetItemDefinitionDiagnostics());
}

void UGridPartyInventoryComponent::LogEquipmentCompatibilityDiagnostics() const
{
	TArray<FGridInventoryDefinitionDiagnosticRow> Rows;
	if (!FGridInventoryDefinitionDiagnosticReadModel::Build(GridPartyInventoryDiagnosticsRegistry(this), Rows))
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridEquipmentCompatibility ItemSystem=UNCONFIGURED"));
		return;
	}
	int32 UnresolvedCount = 0;
	int32 PotentiallyEquippableWithoutSlotsCount = 0;
	int32 LightWithoutHandSlotCount = 0;
	int32 ExcludedPaperDollSlotCount = 0;
	int32 NewPaperDollSlotUsageCount = 0;
	for (const auto& Row : Rows)
	{
		if (!Row.bResolved)
		{
			++UnresolvedCount;
			UE_LOG(LogGridInventory, Warning, TEXT("GridEquipmentCompatibility Item=%s Warning=CanonicalResolveFailed"), *Row.ItemId.ToString());
			continue;
		}
		const FString SlotsText = GridPartyInventoryDiagnosticsGetEquipmentSlotsText(Row.Slots);
		if (Row.Slots.IsEmpty() && Row.bLooksEquippable)
		{
			++PotentiallyEquippableWithoutSlotsCount;
			UE_LOG(LogGridInventory, Warning, TEXT("GridEquipmentCompatibility Item=%s Warning=PotentiallyEquippableWithoutSlots"), *Row.ItemId.ToString());
		}
		if (Row.bHasLightEmitter && !Row.Slots.Contains(EGridEquipmentSlot::MainHand) && !Row.Slots.Contains(EGridEquipmentSlot::OffHand))
		{
			++LightWithoutHandSlotCount;
			UE_LOG(LogGridInventory, Warning, TEXT("GridEquipmentCompatibility Item=%s Warning=LightWithoutMainHandOrOffHand Slots=%s"), *Row.ItemId.ToString(), *SlotsText);
		}
		for (EGridEquipmentSlot Slot : Row.Slots)
		{
			if (GridPartyInventoryDiagnosticsIsExcludedPaperDollSlot(Slot))
			{
				++ExcludedPaperDollSlotCount;
				UE_LOG(LogGridInventory, Warning, TEXT("GridEquipmentCompatibility Item=%s Warning=PaperDollExcludedSlot Slot=%s"), *Row.ItemId.ToString(), GridEquipmentSlotUtils::GetLogName(Slot));
			}
			else if (GridPartyInventoryDiagnosticsIsNewPaperDollSlot(Slot))
			{
				++NewPaperDollSlotUsageCount;
				UE_LOG(LogGridInventory, Log, TEXT("GridEquipmentCompatibility Item=%s UsesNewPaperDollSlot=%s"), *Row.ItemId.ToString(), GridEquipmentSlotUtils::GetLogName(Slot));
			}
		}
	}
	UE_LOG(LogGridInventory, Log, TEXT("GridEquipmentCompatibility Summary CanonicalDefinitions=%d Unresolved=%d PotentiallyEquippableWithoutSlots=%d LightWithoutHandSlot=%d ExcludedPaperDollSlots=%d NewPaperDollSlotUses=%d"),
		Rows.Num(), UnresolvedCount, PotentiallyEquippableWithoutSlotsCount, LightWithoutHandSlotCount, ExcludedPaperDollSlotCount, NewPaperDollSlotUsageCount);
}

void UGridPartyInventoryComponent::LogSelectedCharacterEquipmentStatBonusDiagnostics() const
{
	const int32 CharacterIndex = PartyInventoryState.SelectedCharacterIndex;
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridEquipmentStatBonus Diagnostics Character=%d Result=false Reason=InvalidCharacterOrEquipment"), CharacterIndex);
		return;
	}

	const FGridEquipmentStatBonus TotalBonus = ComputeCharacterEquipmentStatBonus(CharacterIndex);
	UE_LOG(LogGridInventory, Log, TEXT("GridEquipmentStatBonus Diagnostics Character=%d Total=%s"), CharacterIndex,
		*GridPartyInventoryDiagnosticsGetEquipmentStatBonusText(TotalBonus));

	GridEquipmentSlotUtils::ForEachEquipmentItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
		[this](EGridEquipmentSlot Slot, const FGridItemInstance& Item)
		{
			if (!Item.IsValid())
			{
				return;
			}

			// Use the same per-instance projection as ComputeCharacterEquipmentStatBonus.
			// This includes canonical nominal armor at the actual N1-N20 level.
			const FGridEquipmentStatBonus Bonus = GetEquipmentBonusesForInstance(Item);
			if (!Bonus.HasAnyBonus())
			{
				const UGridItemSubsystem* Registry = GridPartyInventoryDiagnosticsRegistry(this);
				if (!(Registry && Registry->ContainsItem(Item.ItemDefinitionId)) &&
                    !ResolveDiagnosticHistoricalDefinitionForAutomation(this, Registry, Item.ItemDefinitionId))
				{
					UE_LOG(LogGridInventory, Warning, TEXT("GridEquipmentStatBonus Item=%s Slot=%s Warning=MissingDefinition"), *Item.ItemDefinitionId.ToString(),
						GridEquipmentSlotUtils::GetLogName(Slot));
				}
				return;
			}
			UE_LOG(LogGridInventory, Log, TEXT("GridEquipmentStatBonus Item=%s Slot=%s Bonus=%s"), *Item.ItemDefinitionId.ToString(),
				GridEquipmentSlotUtils::GetLogName(Slot), *GridPartyInventoryDiagnosticsGetEquipmentStatBonusText(Bonus));
		});
}

void UGridPartyInventoryComponent::LogSelectedCharacterResistanceDiagnostics() const
{
	const int32 CharacterIndex = PartyInventoryState.SelectedCharacterIndex;
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridDamageResistance Diagnostics Character=%d Result=false Reason=InvalidCharacterOrEquipment"), CharacterIndex);
		return;
	}

	const FGridDamageResistanceSet TotalResistances = ComputeCharacterEquipmentResistances(CharacterIndex);
	UE_LOG(LogGridInventory, Log, TEXT("GridDamageResistance Diagnostics Character=%d Total=%s"), CharacterIndex,
		*GridPartyInventoryDiagnosticsGetDamageResistanceSetText(TotalResistances));

	GridEquipmentSlotUtils::ForEachEquipmentItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
		[this](EGridEquipmentSlot Slot, const FGridItemInstance& Item)
		{
			if (!Item.IsValid())
			{
				return;
			}

			const UGridItemSubsystem* Registry = GridPartyInventoryDiagnosticsRegistry(this);
            const UGridItemDefinitionAsset* Definition =
                ResolveDiagnosticHistoricalDefinitionForAutomation(this, Registry, Item.ItemDefinitionId);
			if (!Definition && !(Registry && Registry->ContainsItem(Item.ItemDefinitionId)))
			{
				UE_LOG(LogGridInventory, Warning, TEXT("GridDamageResistance Item=%s Slot=%s Warning=MissingDefinition"), *Item.ItemDefinitionId.ToString(),
					GridEquipmentSlotUtils::GetLogName(Slot));
				return;
			}

			// Same canonical projection as gameplay, including non-ARM equipment.
			const FGridDamageResistanceSet Resistances =
				FGridEquipmentContributionReadModel::ResolveResistances(Registry, Definition, Item);
			if (Resistances.IsEmpty())
			{
				return;
			}
			UE_LOG(LogGridInventory, Log, TEXT("GridDamageResistance Item=%s Slot=%s Resistances=%s"), *Item.ItemDefinitionId.ToString(),
				GridEquipmentSlotUtils::GetLogName(Slot), *GridPartyInventoryDiagnosticsGetDamageResistanceSetText(Resistances));
		});
}

void UGridPartyInventoryComponent::LogInventoryOwnershipDiagnostics() const
{
	FString Error;
	if (ValidateInventoryOwnership(Error))
	{
		UE_LOG(LogGridInventory, Log, TEXT("GridInventory Ownership OK"));
		return;
	}

	UE_LOG(LogGridInventory, Error, TEXT("GridInventory Ownership ERROR %s"), *Error);
}
