#pragma once

#include "CoreMinimal.h"
#include "GridItemCategory.generated.h"

/** Category used by inventory presentation and dungeon link conditions. */
UENUM(BlueprintType)
enum class EGridItemType : uint8
{
	None,
	Torch,
	Weapon,
	Shield,
	Armor,
	Jewelry,
	Key,
	Gem,
	Potion,
	Scroll,
	Book,
	Food,
	Component,
	Quest,
	Misc
};
