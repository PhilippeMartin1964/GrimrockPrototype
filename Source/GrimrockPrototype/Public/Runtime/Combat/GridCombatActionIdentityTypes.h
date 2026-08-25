#pragma once

#include "CoreMinimal.h"
#include "GridCombatActionIdentityTypes.generated.h"

/** Declared owner of a player combat action definition. */
UENUM(BlueprintType)
enum class EGridCombatActionSourcePolicy : uint8
{
	None UMETA(DisplayName = "None"),
	Universal UMETA(DisplayName = "Universal"),
	Equipment UMETA(DisplayName = "Equipment"),
	Ability UMETA(DisplayName = "Ability"),
	Spell UMETA(DisplayName = "Spell"),
	QuickItem UMETA(DisplayName = "Quick Item")
};
