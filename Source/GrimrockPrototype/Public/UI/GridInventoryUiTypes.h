#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
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
enum class EGridInventoryFilterCategory : uint8
{
	All UMETA(DisplayName = "Tous"),
	Equipment UMETA(DisplayName = "Équipement"),
	Consumables UMETA(DisplayName = "Consommables"),
	Magic UMETA(DisplayName = "Magie"),
	Ingredients UMETA(DisplayName = "Ingrédients"),
	BooksAndKeys UMETA(DisplayName = "Livres et clés"),
	Misc UMETA(DisplayName = "Divers")
};

UENUM(BlueprintType)
enum class EGridInventorySortMode : uint8
{
	PhysicalOrder UMETA(DisplayName = "Ordre"),
	Name UMETA(DisplayName = "Nom"),
	Type UMETA(DisplayName = "Type"),
	Weight UMETA(DisplayName = "Poids")
};

inline FText GetGridInventorySortModeDisplayName(EGridInventorySortMode SortMode)
{
	switch (SortMode)
	{
		case EGridInventorySortMode::Name:
			return NSLOCTEXT("GridInventorySort", "Name", "Nom");
		case EGridInventorySortMode::Type:
			return NSLOCTEXT("GridInventorySort", "Type", "Type");
		case EGridInventorySortMode::Weight:
			return NSLOCTEXT("GridInventorySort", "Weight", "Poids");
		case EGridInventorySortMode::PhysicalOrder:
		default:
			return NSLOCTEXT("GridInventorySort", "PhysicalOrder", "Ordre");
	}
}

/**
 * UI-FILTER01.1 presentation-only projection of the canonical gameplay item type.
 * This does not create a second item taxonomy or mutate item definitions.
 */
inline EGridInventoryFilterCategory ResolveGridInventoryFilterCategory(EGridItemType ItemType)
{
	switch (ItemType)
	{
		case EGridItemType::Torch:
		case EGridItemType::Weapon:
		case EGridItemType::Shield:
		case EGridItemType::Armor:
		case EGridItemType::Jewelry:
			return EGridInventoryFilterCategory::Equipment;

		case EGridItemType::Potion:
		case EGridItemType::Food:
			return EGridInventoryFilterCategory::Consumables;

		case EGridItemType::Scroll:
		case EGridItemType::Gem:
			return EGridInventoryFilterCategory::Magic;

		case EGridItemType::Component:
			return EGridInventoryFilterCategory::Ingredients;

		case EGridItemType::Key:
		case EGridItemType::Book:
			return EGridInventoryFilterCategory::BooksAndKeys;

		case EGridItemType::Quest:
		case EGridItemType::Misc:
		case EGridItemType::None:
		default:
			return EGridInventoryFilterCategory::Misc;
	}
}

inline bool DoesGridItemTypeMatchInventoryFilter(EGridItemType ItemType, EGridInventoryFilterCategory FilterCategory)
{
	return FilterCategory == EGridInventoryFilterCategory::All || ResolveGridInventoryFilterCategory(ItemType) == FilterCategory;
}

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
	bool bHasComparison = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	EGridItemTooltipDeltaState DeltaState = EGridItemTooltipDeltaState::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText ValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FText DeltaText;

	/** Stable presentation key for Blueprint icon lookup; independent of the localized Label. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	FName StatId = NAME_None;
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
	TArray<FGridItemTooltipStatLine> StatLines;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Tooltip")
	TArray<FGridItemTooltipEquipmentComparison> EquipmentComparisons;
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
