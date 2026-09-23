#pragma once

#include "CoreMinimal.h"
#include "UI/GridInventoryWidget.h"
#include "GridInventoryBagWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

/**
 * Independent right-side inventory bag window.
 *
 * Gameplay authority remains UGridPartyInventoryComponent through the shared
 * UGridInventoryWidget projection/interaction layer.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridInventoryBagWidget : public UGridInventoryWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|UI")
	TObjectPtr<UButton> Button_CloseInventoryBag;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter")
	TObjectPtr<UButton> Button_FilterAll;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter")
	TObjectPtr<UButton> Button_FilterEquipment;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter")
	TObjectPtr<UButton> Button_FilterConsumables;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter")
	TObjectPtr<UButton> Button_FilterMagic;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter")
	TObjectPtr<UButton> Button_FilterIngredients;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter")
	TObjectPtr<UButton> Button_FilterBooksAndKeys;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter")
	TObjectPtr<UButton> Button_FilterMisc;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter|Selection")
	TObjectPtr<UImage> Image_FilterAllSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter|Selection")
	TObjectPtr<UImage> Image_FilterEquipmentSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter|Selection")
	TObjectPtr<UImage> Image_FilterConsumablesSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter|Selection")
	TObjectPtr<UImage> Image_FilterMagicSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter|Selection")
	TObjectPtr<UImage> Image_FilterIngredientsSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter|Selection")
	TObjectPtr<UImage> Image_FilterBooksAndKeysSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Filter|Selection")
	TObjectPtr<UImage> Image_FilterMiscSelectionFrame;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Sort")
	TObjectPtr<UButton> Button_SortInventory;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Sort")
	TObjectPtr<UTextBlock> Text_SortInventory;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void HandleInventoryFilterCategoryChanged() override;
	virtual void HandleInventorySortModeChanged() override;

private:
	UFUNCTION()
	void HandleFilterAllClicked();

	UFUNCTION()
	void HandleFilterEquipmentClicked();

	UFUNCTION()
	void HandleFilterConsumablesClicked();

	UFUNCTION()
	void HandleFilterMagicClicked();

	UFUNCTION()
	void HandleFilterIngredientsClicked();

	UFUNCTION()
	void HandleFilterBooksAndKeysClicked();

	UFUNCTION()
	void HandleFilterMiscClicked();

	UFUNCTION()
	void HandleSortInventoryClicked();

	void RefreshInventoryFilterSelectionFrames();
	void RefreshInventorySortPresentation();
};
