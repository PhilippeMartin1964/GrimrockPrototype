#pragma once

#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "UObject/StrongObjectPtr.h"

// Synchronous, instance-local reservations. No state survives the stack frame.
// Public mutation entry points reject reserved characters; transfer-owned
// insertion uses a private entry point, never a flag inherited by callbacks.
class FGridPartyTransactionScope
{
public:
	FGridPartyTransactionScope(UGridPartyInventoryComponent* InInventory, const TArray<int32>& Indices, AGridReceptacleActor* InReceptacle = nullptr)
		: Inventory(InInventory)
		, Receptacle(InReceptacle)
	{
		if (!IsValid(InInventory) || (InReceptacle && (!IsValid(InReceptacle) || InReceptacle->bItemTransferInProgress)))
			return;
		for (const int32 Index : Indices)
		{
			if (!InInventory->IsValidCharacterIndex(Index))
				return;
			const FGuid Id = InInventory->PartyInventoryState.ActiveCharacters[Index].CharacterId;
			if (!Id.IsValid() || CharacterIds.Contains(Id) || InInventory->ReservedTransactionCharacters.Contains(Id) || ResolveUnique(Id) != Index)
				return;
			CharacterIds.Add(Id);
		}
		for (const FGuid& Id : CharacterIds)
			InInventory->ReservedTransactionCharacters.Add(Id);
		if (InReceptacle)
			InReceptacle->bItemTransferInProgress = true;
		bAcquired = true;
	}

	~FGridPartyTransactionScope()
	{
		if (!bAcquired)
			return;
		for (const FGuid& Id : CharacterIds)
			Inventory->ReservedTransactionCharacters.Remove(Id);
		for (const FGuid& Id : ItemIds)
			Inventory->ReservedTransactionItems.Remove(Id);
		if (Receptacle.IsValid())
			Receptacle->bItemTransferInProgress = false;
	}

	FGridPartyTransactionScope(const FGridPartyTransactionScope&) = delete;
	FGridPartyTransactionScope& operator=(const FGridPartyTransactionScope&) = delete;
	bool IsAcquired() const
	{
		return bAcquired;
	}
	bool ReserveItem(const FGuid& Id)
	{
		if (!bAcquired || !Id.IsValid() || Inventory->ReservedTransactionItems.Contains(Id))
			return false;
		Inventory->ReservedTransactionItems.Add(Id);
		ItemIds.Add(Id);
		return true;
	}
	const FGuid& GetCharacterId(int32 Ordinal = 0) const
	{
		return CharacterIds[Ordinal];
	}
	int32 ResolveIndex(int32 Ordinal = 0) const
	{
		return bAcquired ? ResolveUnique(CharacterIds[Ordinal]) : INDEX_NONE;
	}

private:
	int32 ResolveUnique(const FGuid& Id) const
	{
		if (!Inventory.IsValid() || !IsValid(Inventory.Get()))
			return INDEX_NONE;
		int32 Found = INDEX_NONE;
		for (int32 I = 0; I < Inventory->PartyInventoryState.ActiveCharacters.Num(); ++I)
		{
			if (Inventory->PartyInventoryState.ActiveCharacters[I].CharacterId == Id)
			{
				if (Found != INDEX_NONE)
					return INDEX_NONE;
				Found = I;
			}
		}
		return Found;
	}

	TStrongObjectPtr<UGridPartyInventoryComponent> Inventory;
	TStrongObjectPtr<AGridReceptacleActor> Receptacle;
	TArray<FGuid> CharacterIds;
	TArray<FGuid> ItemIds;
	bool bAcquired = false;
};
