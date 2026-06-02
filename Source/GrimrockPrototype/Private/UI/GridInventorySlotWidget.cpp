#include "UI/GridInventorySlotWidget.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
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
        return TEXT ("Empty");
    }

    return CachedItem.ItemDefinitionId.ToString ();
}

FString UGridInventorySlotWidget::GetQuantityText () const
{
    if (!bHasItem || CachedItem.Quantity <= 1)
    {
        return FString ();
    }

    return FString::Printf (TEXT ("%d"), CachedItem.Quantity);
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

void UGridInventorySlotWidget::RefreshSlotVisual_Implementation ()
{
}
