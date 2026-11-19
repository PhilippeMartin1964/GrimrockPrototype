#pragma once

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Temporary explicit inventory source only. Never loads recipes or scans the level. */
struct FGridLegacyItemLookup
{
	static bool IsRegistered(const AGrimrockPartyPawn* Party, FName ItemId)
	{
		const UWorld* World = Party ? Party->GetWorld() : nullptr;
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		const UGridItemSubsystem* Registry = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
		return Registry && Registry->HasCanonicalItem(ItemId);
	}

	static UGridItemDefinitionAsset* Resolve(const AGrimrockPartyPawn* Party, FName ItemId)
	{
		if (!Party || ItemId.IsNone() || IsRegistered(Party, ItemId))
		{
			return nullptr;
		}
		UGridItemDefinitionAsset* Old = Party->PartyInventoryComponent
			? Party->PartyInventoryComponent->FindItemDefinition(ItemId) : nullptr;
		if (IsValid(Old) && Old->ItemDefinitionId == ItemId)
		{
			return Old;
		}
		return nullptr;
	}
};
