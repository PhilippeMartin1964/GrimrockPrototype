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

	/** Color of the persistent outline around the currently selected inventory filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Filter|Selection")
	FLinearColor SelectedFilterFrameColor = FLinearColor(1.0f, 0.65f, 0.10f, 1.0f);

	/** Thickness in Slate units of the persistent selected-filter outline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Filter|Selection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "12.0"))
	float SelectedFilterFrameThickness = 3.0f;

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

	UImage* CreateFilterSelectionFrame(UButton* Button, FName OverlayName, FName FrameName);
	void RefreshInventoryFilterSelectionFrames();
	void RefreshInventorySortPresentation();

	UPROPERTY(Transient)
	TObjectPtr<UImage> FilterSelectionFrameAll;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FilterSelectionFrameEquipment;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FilterSelectionFrameConsumables;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FilterSelectionFrameMagic;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FilterSelectionFrameIngredients;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FilterSelectionFrameBooksAndKeys;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FilterSelectionFrameMisc;
};
