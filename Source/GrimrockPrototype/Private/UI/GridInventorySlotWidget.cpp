#include "UI/GridInventorySlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "InputCoreTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UI/GridInventoryWidget.h"

void UGridInventorySlotWidget::InitializeInventorySlot (
    EGridInventoryUiSlotType InSlotType,
    int32 InInventorySlotIndex)
{
    SlotType = InSlotType;
    InventorySlotIndex = InInventorySlotIndex;
}

void UGridInventorySlotWidget::SetItem (const FGridItemInstance& InItem)
{
    CachedItem = InItem;
    bHasItem = InItem.IsValid ();
    RefreshSlotVisual ();
}

void UGridInventorySlotWidget::ClearItem ()
{
    CachedItem = FGridItemInstance ();
    bHasItem = false;
    RefreshSlotVisual ();
}

bool UGridInventorySlotWidget::HasItem () const
{
    return bHasItem;
}

FGridItemInstance UGridInventorySlotWidget::GetCachedItem () const
{
    return CachedItem;
}

FString UGridInventorySlotWidget::GetDisplayNameText () const
{
    if (!bHasItem || CachedItem.ItemDefinitionId.IsNone ())
    {
        return FString ();
    }

    return CachedItem.ItemDefinitionId.ToString ();
}

FString UGridInventorySlotWidget::GetQuantityText () const
{
    if (!bHasItem)
    {
        return FString ();
    }

    return FString::Printf (TEXT ("%d"), FMath::Max (1, CachedItem.Quantity));
}

FText UGridInventorySlotWidget::GetTooltipText () const
{
    if (!bHasItem)
    {
        return FText::FromString (TEXT ("Empty"));
    }

    return FText::FromString (FString::Printf (
        TEXT ("%s\nQty: %d\nWeight: %.1f"),
        *CachedItem.ItemDefinitionId.ToString (),
        CachedItem.Quantity,
        CachedItem.Weight));
}

UTexture2D* UGridInventorySlotWidget::GetIconTexture () const
{
    if (!bHasItem || CachedItem.ItemDefinitionId.IsNone ())
    {
        return nullptr;
    }

    const UGridInventoryWidget* InventoryWidget = OwningInventoryWidget.Get ();
    const UGridPartyInventoryComponent* InventoryComponent = InventoryWidget ? InventoryWidget->InventoryComponent : nullptr;
    const UGridItemDefinitionAsset* Definition = InventoryComponent
        ? InventoryComponent->FindItemDefinition (CachedItem.ItemDefinitionId)
        : nullptr;
    return Definition ? Definition->Icon.LoadSynchronous () : nullptr;
}

void UGridInventorySlotWidget::HandleClicked ()
{
    OnSlotClicked.Broadcast (SlotType, InventorySlotIndex);
}

void UGridInventorySlotWidget::SetOwnerInventoryWidget (UGridInventoryWidget* InOwnerInventoryWidget)
{
    OwningInventoryWidget = InOwnerInventoryWidget;
}

bool UGridInventorySlotWidget::CanStartDrag () const
{
    return bDragEnabled && bHasItem && CachedItem.IsValid ();
}

UGridInventoryDragDropOperation* UGridInventorySlotWidget::CreateDragDropOperation () const
{
    if (!CanStartDrag ())
    {
        return nullptr;
    }

    UGridInventoryDragDropOperation* Operation = NewObject<UGridInventoryDragDropOperation> ();
    if (!Operation)
    {
        return nullptr;
    }

    Operation->InitializeFromSlot (SlotType, InventorySlotIndex, CachedItem);
    Operation->DefaultDragVisual = nullptr;
    Operation->Pivot = EDragPivot::MouseDown;
    return Operation;
}

void UGridInventorySlotWidget::RefreshSlotVisual_Implementation ()
{
}

FReply UGridInventorySlotWidget::NativeOnMouseButtonDown (
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton () == EKeys::LeftMouseButton && CanStartDrag ())
    {
        return UWidgetBlueprintLibrary::DetectDragIfPressed (
            InMouseEvent,
            this,
            EKeys::LeftMouseButton).NativeReply;
    }

    return Super::NativeOnMouseButtonDown (InGeometry, InMouseEvent);
}

void UGridInventorySlotWidget::NativeOnDragDetected (
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent,
    UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected (InGeometry, InMouseEvent, OutOperation);

    UGridInventoryDragDropOperation* Operation = CreateDragDropOperation ();
    if (!Operation)
    {
        return;
    }

    OutOperation = Operation;
    UE_LOG (LogTemp, Verbose, TEXT ("GridInventory UI DragStarted Type=%s Slot=%d Item=%s RuntimeId=%s"),
        GetGridInventoryUiSlotTypeName (SlotType),
        InventorySlotIndex,
        *CachedItem.ItemDefinitionId.ToString (),
        *CachedItem.RuntimeObjectId.ToString ());
}

bool UGridInventorySlotWidget::NativeOnDrop (
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    UGridInventoryDragDropOperation* Operation = Cast<UGridInventoryDragDropOperation> (InOperation);
    if (!Operation || !OwningInventoryWidget)
    {
        return Super::NativeOnDrop (InGeometry, InDragDropEvent, InOperation);
    }

    return OwningInventoryWidget->HandleSlotDrop (
        Operation->SourceSlotType,
        Operation->SourceSlotIndex,
        SlotType,
        InventorySlotIndex);
}
