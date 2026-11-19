#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Diagnostics only. Reads the canonical catalogue, never the inventory's historical cache. */
struct FGridCanonicalInventoryDefinitionDiagnosticRow
{
	FName ItemId = NAME_None;
	bool bResolved = false;
	bool bArmament = false;
	bool bAmmo = false;
	bool bLooksEquippable = false;
	bool bHasLightEmitter = false;
	TArray<EGridEquipmentSlot> Slots;
	FString WeightText = TEXT("INVALID");
};

struct FGridCanonicalInventoryDefinitionDiagnosticReadModel
{
	static bool Build(const UGridItemSubsystem* Registry,
		TArray<FGridCanonicalInventoryDefinitionDiagnosticRow>& OutRows)
	{
		OutRows.Reset();
		if (!IsValid(Registry) || !Registry->IsConfigured()) return false;
		TArray<FName> Ids;
		Registry->GetCanonicalItemIds(Ids);
		for (FName Id : Ids)
		{
			auto& Row = OutRows.AddDefaulted_GetRef();
			Row.ItemId = Id;
			FGridResolvedItem Item, Last;
			if (Registry->ResolveItem(Id, 0, Item) && Item.IsValid() && !Item.bIsArmament)
			{
				Row.WeightText = FString::Printf(TEXT("N0:%.1f"), Item.WeightKg);
			}
			else if (Registry->ResolveItem(Id, 1, Item) && Item.IsValid() && Item.bIsArmament &&
				Registry->ResolveItem(Id, 20, Last) && Last.IsValid() && Last.bIsArmament)
			{
				Row.WeightText = FString::Printf(TEXT("N1:%.1f,N20:%.1f"), Item.WeightKg, Last.WeightKg);
			}
			else continue; // Never omit an invalid registered identity or substitute an old definition.
			Row.bResolved = true;
			Row.bArmament = Item.bIsArmament;
			Row.bAmmo = !Item.AmmoKindId.IsNone();
			Row.bHasLightEmitter = Item.LightEmitter.HasEmitter();
			if (Item.bIsArmament)
			{
				if (Item.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand))
					Row.Slots.Add(EGridEquipmentSlot::MainHand);
				if (Item.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand))
					Row.Slots.Add(EGridEquipmentSlot::OffHand);
			}
			else Row.Slots = Item.NonArmEquipmentSlots;
			Row.bLooksEquippable = Row.bArmament || !Row.Slots.IsEmpty() || !Item.EquippedMesh.IsNull();
		}
		return true;
	}
};
