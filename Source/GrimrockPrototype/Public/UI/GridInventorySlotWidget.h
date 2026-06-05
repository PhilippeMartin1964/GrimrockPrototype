#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridInventorySlotWidget.generated.h"

class UTexture2D;
class UGridInventoryWidget;
class UDragDropOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams (
    FOnGridInventorySlotClicked,
    EGridInventoryUiSlotType, SlotType,
    int32, SlotIndex);

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridInventorySlotWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slot")
    EGridInventoryUiSlotType SlotType = EGridInventoryUiSlotType::Inventory;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slot")
    int32 InventorySlotIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slot")
    FGridItemInstance CachedItem;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slot")
    bool bHasItem = false;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slot")
    TObjectPtr<UGridInventoryWidget> OwningInventoryWidget;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory|Drag")
    bool bDragEnabled = true;

    UPROPERTY (BlueprintAssignable, Category = "Inventory|Slot")
    FOnGridInventorySlotClicked OnSlotClicked;

    UPROPERTY (Transient)
    TObjectPtr<UTexture2D> CachedIconTexture = nullptr;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    void InitializeInventorySlot (EGridInventoryUiSlotType InSlotType, int32 InInventorySlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    void SetItem (const FGridItemInstance& InItem);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    void ClearItem ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    bool HasItem () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    FGridItemInstance GetCachedItem () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    FString GetDisplayNameText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    FString GetQuantityText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    FText GetTooltipText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    UTexture2D* GetIconTexture () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    void HandleClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slot")
    void SetOwnerInventoryWidget (UGridInventoryWidget* InOwnerInventoryWidget);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Drag")
    bool CanStartDrag () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Drag")
    UGridInventoryDragDropOperation* CreateDragDropOperation () const;

    UFUNCTION (BlueprintCallable, BlueprintNativeEvent, Category = "Inventory|Slot")
    void RefreshSlotVisual ();

protected:
    virtual FReply NativeOnMouseButtonDown (
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual void NativeOnDragDetected (
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent,
        UDragDropOperation*& OutOperation) override;

    virtual bool NativeOnDrop (
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;
};
