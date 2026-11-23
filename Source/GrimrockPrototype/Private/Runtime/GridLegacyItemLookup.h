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
	static UGridItemDefinitionAsset* Resolve(const AGrimrockPartyPawn* Party, FName ItemId)
	{
#if WITH_DEV_AUTOMATION_TESTS
		// Only worldless Automation fixtures may consult the historical
		// inventory definition map. An unknown ID in a GameInstance-owned
		// world must never gain gameplay metadata from an old DataAsset.
		const UWorld* World = Party ? Party->GetWorld() : nullptr;
		if (!Party || ItemId.IsNone() || !GIsAutomationTesting || !World ||
			World->GetGameInstance() != nullptr)
		{
			return nullptr;
		}
		UGridItemDefinitionAsset* Old = Party->PartyInventoryComponent
			? Party->PartyInventoryComponent->FindItemDefinition(ItemId) : nullptr;
		return IsValid(Old) && Old->ItemDefinitionId == ItemId ? Old : nullptr;
#else
		(void)Party;
		(void)ItemId;
		return nullptr;
#endif
	}
};
