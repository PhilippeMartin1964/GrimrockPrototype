#pragma once

#include "CoreMinimal.h"
#include "GridEquippedWeaponActionIconTypes.generated.h"

/**
 * Icons represent an available intrinsic EQUIPPED weapon action, never an
 * inventory item, consumable, spellbook spell or saved character ability.
 * ARMEMENT01 categories are stable; the concrete textures are authored in UE.
 */
UENUM(BlueprintType)
enum class EGridEquippedWeaponActionIconKind : uint8
{
	None,
	MeleeOneHand,
	MeleeDualWield,
	MeleeTwoHanded,
	MeleePolearm,
	RangedBow,
	RangedCrossbow,
	RangedOther,
	ThrownWeapon,
	MagicOneHand,
	MagicTwoHanded,
	DefenseBlock,
	DefenseParry
};
