#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridEquipmentPreviewTypes.generated.h"

/** Why a read-only preview cannot be committed by the normal inventory equip action. */
UENUM(BlueprintType)
enum class EGridEquipmentPreviewStatus : uint8
{
    InvalidInput,
    UnsupportedSlot,
    InventoryFull,
    Ready
};

/** One physical item displaced by the hypothetical equip, never virtual OffHand duplicates. */
USTRUCT(BlueprintType)
struct FGridEquipmentPreviewDisplacedItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    EGridEquipmentSlot OriginalSlot = EGridEquipmentSlot::None;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    FName ItemDefinitionId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    FGuid RuntimeObjectId;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    FText DisplayName;

    /** This is remaining, already-absorbed-aware physical protection. */
    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    int32 RemainingPhysicalArmor = 0;
};

/** Numerical state of the character BEFORE versus AFTER a hypothetical equip. */
USTRUCT(BlueprintType)
struct FGridEquipmentPreviewDelta
{
    GENERATED_BODY()

    /** Presentation-neutral key for 04.3B: no localized labels hardcoded here. */
    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    FName StatId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    float Before = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    float After = 0.f;

    /** Always After-Before. Higher is not automatically beneficial for every stat. */
    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    float Delta = 0.f;
};

/**
 * INDUS04.3A data-only transaction projection.
 * bValidPreview means the hypothetical state was computed, even if InventoryFull.
 * bCanEquipNow means the exact inventory-backed transaction can currently succeed.
 * This is not an action and must never change equipment or inventory contents.
 */
USTRUCT(BlueprintType)
struct FGridEquipmentPreviewResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    bool bValidPreview = false;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    bool bCanEquipNow = false;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    EGridEquipmentPreviewStatus Status = EGridEquipmentPreviewStatus::InvalidInput;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    EGridEquipmentSlot RequestedSlot = EGridEquipmentSlot::None;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    EGridEquipmentSlot ActualTargetSlot = EGridEquipmentSlot::None;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    bool bWouldReleaseBothHands = false;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    bool bCandidateUsesTwoHands = false;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    int32 FreeInventorySlotsAfterSourceRemoval = 0;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    int32 RequiredSlotsForDisplacedItems = 0;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    TArray<FGridEquipmentPreviewDisplacedItem> DisplacedItems;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    FGridEquipmentStatBonus BeforeBonuses;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    FGridEquipmentStatBonus AfterBonuses;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    FGridDamageResistanceSet BeforeResistances;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    FGridDamageResistanceSet AfterResistances;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    int32 EffectivePhysicalArmorBefore = 0;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    int32 EffectivePhysicalArmorAfter = 0;

    /** All numeric fields, including zero deltas. HUD chooses which rows to show. */
    UPROPERTY(BlueprintReadOnly, Category="Inventory|Equipment|Preview")
    TArray<FGridEquipmentPreviewDelta> NumericDeltas;
};
