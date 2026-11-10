#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridEquippedWeaponActionIconPalette.generated.h"

class UGridItemDefinitionAsset;
class UTexture2D;

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

/** One reusable icon set for all ARM001-ARM082 equipment action categories. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridEquippedWeaponActionIconPalette : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon Action Icons")
	TMap<EGridEquippedWeaponActionIconKind, TSoftObjectPtr<UTexture2D>> Icons;

	TSoftObjectPtr<UTexture2D> FindIcon(EGridEquippedWeaponActionIconKind Kind) const;
};

/**
 * Pure read-only ARMEMENT01 category lookup. No hidden combat permissions:
 * the action MUST already exist in the authoritative combat catalogue.
 * The workbook assigns consecutive ARM IDs by family; the only boundaries
 * maintained here are the visual category boundaries (not per-weapon logic).
 */
struct GRIMROCKPROTOTYPE_API FGridEquippedWeaponActionIconResolver
{
	static EGridEquippedWeaponActionIconKind Classify(
		const FGridAvailableCombatAction& Action, const UGridItemDefinitionAsset* Definition);
};
