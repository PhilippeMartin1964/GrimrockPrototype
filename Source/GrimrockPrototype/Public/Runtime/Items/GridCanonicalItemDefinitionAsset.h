#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridNonArmamentThrowPolicy.h"
#include "Runtime/GridLightEmitterTypes.h"
#include "Runtime/GridItemVisualEffectsTypes.h"
#include "GridCanonicalItemDefinitionAsset.generated.h"

class UTexture2D;
class UStaticMesh;
class UMaterialInterface;

/** Only an action identity and a presentation identity are authored per owner. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridItemActionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Actions")
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Actions")
	FName PresentationId = NAME_None;
};

/**
 * ARM-CLEAN06.1 canonical five-field base. Its temporary class name avoids
 * changing the serialized pre-migration UGridItemDefinitionAsset.
 * Replaced by the normative name only at the verified binary cutover.
 */
UCLASS(Abstract, BlueprintType)
class GRIMROCKPROTOTYPE_API UGridCanonicalItemDefinitionAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0"))
	float WeightKg = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<FName> Tags;

	bool HasValidIdentity() const;
};

/** Resolved non-ARM hand policy. No legacy Auto inference survives cutover. */
UENUM(BlueprintType)
enum class EGridNonArmamentHandPolicy : uint8
{
    NotHandHeld,
    OneHanded,
    TwoHanded
};

/** Exactly one additional editable property; no catalogue, icon or N1 copy. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridArmamentDefinitionAsset : public UGridCanonicalItemDefinitionAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armament")
	FName ArchetypeId = NAME_None;
};

/** Visual data for NON-armament items, never a fallback for a weapon. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridNonArmamentDefinitionAsset : public UGridCanonicalItemDefinitionAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/** Explicit per-owner action bindings; mechanics/presentations stay in
	 * the two global catalogues. Existing consumable UseActions remain until
	 * their verified Editor cutover (never duplicate bindings per item). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Actions")
	TArray<FGridItemActionBinding> ActionBindings;

	/** ARM-CLEAN06.4L: NON-armament presentation belongs to this item,
	 * not to an ARM attack/weapon visual catalogue. The old source remains
	 * untouched until the verified binary migration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Light")
	FGridLightEmitterConfig LightEmitter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual|Persistent")
	FGridItemVisualEffectsConfig VisualEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual|Sparkle")
	bool bEnableWorldSparkle = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual|Sparkle")
	TSoftObjectPtr<UMaterialInterface> WorldSparkleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual|Sparkle")
	FLinearColor WorldSparkleColor = FLinearColor(1.0f, 0.78f, 0.42f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual|Sparkle", meta = (ClampMin = "0.0"))
	float WorldSparkleIntensity = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual|Sparkle", meta = (ClampMin = "0.0"))
	float WorldSparkleSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visual|Sparkle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WorldSparkleVariation = 0.35f;

	/** ARM-CLEAN06.4M: source of default readable text for non-ARM items.
	 * Instance overrides and reusable UGridReadableContentAsset remain
	 * distinct instance-level authorities. No ItemType inference at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Reading", meta = (MultiLine = "true"))
	FText ReadText;

	/** True for legacy books/scrolls even when the authored text is empty.
	 * This is an explicit reader capability, not a second content value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Reading")
	bool bReadable = false;

	/** World pickup Chaos policy; numeric mass is always WeightKg per unit,
	 * never Quantity * WeightKg, and never a duplicate stored mass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|World Physics")
	bool bUseItemWeightAsWorldPhysicsMass = false;

	/** Deterministic first-placement tilt only; never reapplied at Save/Continue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|World Physics", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float WorldPhysicsInitialTiltDegrees = 0.0f;

	/** ARM-CLEAN06.4N: generic non-ARM hand toss data only. No weapon
	 * catalogue, action permission or runtime fallback is introduced. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Generic Throw")
	FGridNonArmamentThrowPolicy GenericThrow;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridConsumableDefinitionAsset : public UGridNonArmamentDefinitionAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consumable")
	TArray<FGridItemActionBinding> UseActions;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridAmmoDefinitionAsset : public UGridNonArmamentDefinitionAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	FName AmmoKindId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "1"))
	int32 MaxStack = 1;
};

/** Only generic non-ARM stacks. Consumables and ammo retain their OWN
 * existing MaxStack, never a second capacity or an inferred ItemType. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridGenericStackableDefinitionAsset : public UGridNonArmamentDefinitionAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Stack", meta = (ClampMin = "1"))
    int32 MaxStack = 1;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridOtherEquipmentDefinitionAsset : public UGridNonArmamentDefinitionAsset
{
	GENERATED_BODY()

public:
	/** Exact stack capacity for equipped non-ARM throwables. A weapon ARCHEtype
	 * is not inferred from a stack, and non-stackable gear retains capacity 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Stack", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<EGridEquipmentSlot> EquipmentSlots;

	/** Only for non-ARM armor/jewelry; never duplicates catalogued ARM defenses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGridEquipmentStatBonus Bonuses;

	/** Explicit effective hand policy; Auto is resolved from real source
	 * slots only by the one-shot Editor converter, never at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EGridNonArmamentHandPolicy HandPolicy = EGridNonArmamentHandPolicy::NotHandHeld;

	/** Non-ARM equipment resistances are not ARM combat catalogue defenses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGridDamageResistanceSet Resistances;

	/** Optional held/paperdoll mesh, distinct from non-ARM WorldMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Visual")
	TSoftObjectPtr<UStaticMesh> EquippedMesh;

	/** Mirrors explicit source slot behavior, including the special hand rule. */
	bool CanEquipToSlot(EGridEquipmentSlot Slot) const
	{
		if (Slot == EGridEquipmentSlot::None) { return false; }
		if ((Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand) &&
			(HandPolicy == EGridNonArmamentHandPolicy::OneHanded ||
			 HandPolicy == EGridNonArmamentHandPolicy::TwoHanded))
		{
			return true;
		}
		return EquipmentSlots.Contains(Slot);
	}

	/** Inventory/action visibility hides the virtual OffHand of two-handed
	 * gear; direct CanEquipToSlot remains identical to the legacy definition. */
	TArray<EGridEquipmentSlot> GetAvailableSlots() const
	{
		TArray<EGridEquipmentSlot> Slots;
		if (CanEquipToSlot(EGridEquipmentSlot::MainHand)) { Slots.Add(EGridEquipmentSlot::MainHand); }
		if (HandPolicy != EGridNonArmamentHandPolicy::TwoHanded &&
			CanEquipToSlot(EGridEquipmentSlot::OffHand))
		{
			Slots.Add(EGridEquipmentSlot::OffHand);
		}
		for (const EGridEquipmentSlot Slot : EquipmentSlots)
		{
			if (Slot != EGridEquipmentSlot::None && Slot != EGridEquipmentSlot::MainHand &&
				Slot != EGridEquipmentSlot::OffHand && !Slots.Contains(Slot) && CanEquipToSlot(Slot))
			{
				Slots.Add(Slot);
			}
		}
		return Slots;
	}

	TSoftObjectPtr<UStaticMesh> GetEffectiveEquippedMesh() const
	{
		return EquippedMesh.IsNull() ? WorldMesh : EquippedMesh;
	}
};
