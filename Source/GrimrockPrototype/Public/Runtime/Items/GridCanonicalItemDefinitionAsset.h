#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridCanonicalItemDefinitionAsset.generated.h"

class UTexture2D;
class UStaticMesh;

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
