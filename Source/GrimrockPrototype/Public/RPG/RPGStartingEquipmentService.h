#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"

class URPGClassAsset;
class UGridItemSubsystem;

/**
 * ARMEMENT01.9.1: build a class kit into uncommitted character/equipment states.
 * No live party or item registry is mutated on failure. No item is duplicated
 * when a two-handed weapon virtually occupies OffHand.
 * Kits are built only from a configured ItemSystem.
 */
class GRIMROCKPROTOTYPE_API FRPGStartingEquipmentService
{
public:
	static bool TryBuildKit(
		const URPGClassAsset* ClassDefinition,
		FName KitId,
		FGridCharacterInventoryState& InOutCharacter,
		FGridCharacterEquipmentState& InOutEquipment,
		FString& OutError,
		const UGridItemSubsystem* CanonicalRegistry);
};
