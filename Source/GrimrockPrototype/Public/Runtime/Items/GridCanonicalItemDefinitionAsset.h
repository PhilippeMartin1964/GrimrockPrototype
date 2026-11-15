#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/GridInventoryTypes.h"
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

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridOtherEquipmentDefinitionAsset : public UGridNonArmamentDefinitionAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<EGridEquipmentSlot> EquipmentSlots;

	/** Only for non-ARM armor/jewelry; never duplicates catalogued ARM defenses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGridEquipmentStatBonus Bonuses;
};
