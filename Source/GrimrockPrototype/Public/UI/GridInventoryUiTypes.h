#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridInventoryUiTypes.generated.h"

UENUM(BlueprintType)
enum class EInventoryTopTab : uint8
{
	Inventory = 0,
	Skills = 1,
	Journal = 2,
	Map = 3,
	Recipes = 4,
	Codex = 5,
	Spellbook = 6
};


UENUM(BlueprintType)
enum class EGridItemTooltipDeltaState : uint8
{
	Neutral,
	Positive,
	Negative
};

USTRUCT(BlueprintType)
struct FGridItemTooltipStatLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	float ItemValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	float EquippedValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	float Delta = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bIntegerValue = true;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bHasComparison = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	EGridItemTooltipDeltaState DeltaState = EGridItemTooltipDeltaState::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText ValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText DeltaText;
};

USTRUCT(BlueprintType)
struct FGridItemTooltipEquipmentComparison
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	EGridEquipmentSlot EquipmentSlot = EGridEquipmentSlot::None;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText SlotLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bCanEquipToSlot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bHasEquippedItem = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText EquippedItemName;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	TArray<FGridItemTooltipStatLine> StatLines;
};

USTRUCT(BlueprintType)
struct FGridItemTooltipView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FName ItemDefinitionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText ItemType;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	float UnitWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	float TotalWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText WeightText;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText CompatibleSlotsText;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bEquippable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bReadable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bProvidesLight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bLightEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	bool bCanAssignToHotbar = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText UsageSummary;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	TArray<FGridItemTooltipStatLine> StatLines;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	TArray<FGridItemTooltipEquipmentComparison> EquipmentComparisons;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText StatSummary;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText ComparisonSummary;
};

UENUM(BlueprintType)
enum class EGridInventoryUiSlotType : uint8
{
	Inventory,
	Equipment,
	MainHand,
	OffHand,
	Cursor
};

inline const TCHAR* GetGridInventoryUiSlotTypeName(EGridInventoryUiSlotType SlotType)
{
	switch (SlotType)
	{
		case EGridInventoryUiSlotType::Inventory:
			return TEXT("Inventory");
		case EGridInventoryUiSlotType::Equipment:
			return TEXT("Equipment");
		case EGridInventoryUiSlotType::MainHand:
			return TEXT("MainHand");
		case EGridInventoryUiSlotType::OffHand:
			return TEXT("OffHand");
		case EGridInventoryUiSlotType::Cursor:
			return TEXT("Cursor");
		default:
			return TEXT("Unknown");
	}
}
