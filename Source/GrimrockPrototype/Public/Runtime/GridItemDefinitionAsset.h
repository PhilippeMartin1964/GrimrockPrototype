#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridItemDefinitionAsset.generated.h"

class UStaticMesh;
class UTexture2D;

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

UENUM(BlueprintType)
enum class EGridItemHandUsage : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	NotHandHeld UMETA(DisplayName = "Not Hand Held"),
	OneHanded UMETA(DisplayName = "One Handed"),
	TwoHanded UMETA(DisplayName = "Two Handed")
};

UENUM(BlueprintType)
enum class EGridThrowVisualMode : uint8
{
	Stable UMETA(DisplayName = "Stable"),
	Tumble UMETA(DisplayName = "Tumble"),
	Spin UMETA(DisplayName = "Spin")
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridItemDefinitionAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading", meta = (MultiLine = "true"))
	FText ReadText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EGridItemType ItemType = EGridItemType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float Weight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	bool bStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (EditCondition = "bStackable", ClampMin = "1"))
	int32 MaxStackSize = 1;

	/**
	 * Physical hand requirement. Auto infers OneHanded from legacy throwable
	 * data or MainHand/OffHand compatibility; otherwise it resolves to NotHandHeld.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EGridItemHandUsage HandUsage = EGridItemHandUsage::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<EGridEquipmentSlot> CompatibleEquipmentSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Stats")
	FGridEquipmentStatBonus EquipmentStatBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Resistances")
	FGridDamageResistanceSet EquipmentResistanceBonus;

	/** Generic combat actions granted while this item is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Combat Actions")
	TArray<FGridCombatActionDefinition> CombatActions;

	/**
     * Enables one inventory-backed combat action for a potion or scroll.
     * Runtime identity, source policy and a minimum source cost of one are
     * normalized from ItemDefinitionId by BuildQuickItemCombatActionDefinition.
     */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Quick Item")
	bool bProvidesQuickItemCombatAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Quick Item", meta = (EditCondition = "bProvidesQuickItemCombatAction"))
	FGridCombatActionDefinition QuickItemCombatAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Offense|Presentation")
	bool bProvidesAttackPresentation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Offense|Presentation", meta = (EditCondition = "bProvidesAttackPresentation"))
	FGridPlayerAttackPresentationProfile PlayerAttackPresentationProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> EquippedMesh;

	/**
	 * Legacy serialized flag kept only to preserve existing throwable-weapon
	 * assets. New authoring uses HandUsage/Weight for physical throwing and
	 * bCombatThrowWeapon for combat projectile semantics.
	 */
	UPROPERTY()
	bool bThrowable = false;

	/**
	 * True only when combat actions from this item consume/launch the item as
	 * a recoverable projectile. Independent from generic puzzle/exploration throws.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Throw")
	bool bCombatThrowWeapon = false;

	/** Nominal launch speed before the selected character's Strength/weight scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw", meta = (ClampMin = "0.0"))
	float ThrowSpeed = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw", meta = (ClampMin = "0.0"))
	float ThrowArc = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw", meta = (ClampMin = "0.0"))
	float ThrowLifeSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw", meta = (ClampMin = "0.0"))
	float ThrowImpactDropOffset = 12.0f;

	/** Aligns the item mesh with the projectile frame, which follows velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual")
	FRotator ThrowVisualRelativeRotation = FRotator(-90.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual")
	FVector ThrowVisualRelativeScale = FVector(1.5f);

	/** Stable = no extra rotation, Tumble = slow multi-axis rotation, Spin = fast roll. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual")
	EGridThrowVisualMode ThrowVisualMode = EGridThrowVisualMode::Tumble;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual")
	FVector ThrowVisualTumbleAxis = FVector(0.65f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual", meta = (ClampMin = "0.0"))
	float ThrowVisualTumbleDegreesPerSecond = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual", meta = (ClampMin = "0.0"))
	float ThrowVisualSpinDegreesPerSecond = 1080.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Torch")
	bool bCanEmitLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Torch", meta = (EditCondition = "bCanEmitLight"))
	bool bDefaultLightEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Torch", meta = (EditCondition = "bCanEmitLight"))
	float LightRadius = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
	TArray<FName> ItemTags;

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool CanEquipToSlot(EGridEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Throw")
	EGridItemHandUsage GetEffectiveHandUsage() const;

	UFUNCTION(BlueprintPure, Category = "Throw")
	bool IsPhysicallyThrowable() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Throw")
	bool IsCombatThrowable() const;

	/** Global physical rule: 0.25 kg of throwable mass per point of effective Strength. */
	UFUNCTION(BlueprintPure, Category = "Throw")
	float GetMaxThrowableWeightForStrength(int32 Strength) const;

	UFUNCTION(BlueprintPure, Category = "Throw")
	bool CanBeThrownByStrength(int32 Strength) const;

	UFUNCTION(BlueprintPure, Category = "Throw")
	float GetThrowSpeedScaleForStrength(int32 Strength) const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Combat Actions")
	bool CanProvideAttackFromSlot(EGridEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Combat Actions")
	bool HasValidCombatActions() const;

	/** Builds the normalized Use_<ItemDefinitionId> catalogue definition. */
	bool BuildQuickItemCombatActionDefinition(FGridCombatActionDefinition& OutDefinition) const;

	/**
     * Builds an inventory-backed action for the hotbar. In addition to the
     * configured potion/scroll action, throwable weapons receive a normalized
     * quick-throw action without needing a duplicate data-asset definition.
     */
	bool BuildInventoryCombatActionDefinition(FGridCombatActionDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Offense|Presentation")
	bool HasValidPlayerAttackPresentation() const;

	/** Uses the dedicated equipped mesh when assigned, otherwise WorldMesh. */
	UStaticMesh* LoadHeldMesh() const;
};
