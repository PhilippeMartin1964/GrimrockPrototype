#include "UI/GridInventorySlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/SizeBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "InputCoreTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UI/GridInventoryWidget.h"

namespace
{
    constexpr float GridInventoryFallbackSlotSize = 132.0f;

    float GetGridInventoryFixedSlotSize (const UGridInventorySlotWidget* SlotWidget)
    {
        const UGridInventoryWidget* InventoryWidget = SlotWidget
            ? SlotWidget->OwningInventoryWidget.Get ()
            : nullptr;

        return FMath::Max (
            1.0f,
            InventoryWidget
                ? InventoryWidget->InventorySlotSize
                : GridInventoryFallbackSlotSize);
    }
}

void UGridInventorySlotWidget::InitializeInventorySlot (
    EGridInventoryUiSlotType InSlotType,
    int32 InInventorySlotIndex)
{
    SlotType = InSlotType;
    InventorySlotIndex = InInventorySlotIndex;
    bFixedSlotLayoutApplied = false;
    ApplyFixedSlotLayout ();
}

void UGridInventorySlotWidget::NativePreConstruct ()
{
    Super::NativePreConstruct ();
    ApplyFixedSlotLayout ();
}

void UGridInventorySlotWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    ApplyFixedSlotLayout ();
}

void UGridInventorySlotWidget::NativeTick (
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick (MyGeometry, InDeltaTime);

    if (!bFixedSlotLayoutApplied)
    {
        ApplyFixedSlotLayout ();
    }
}

void UGridInventorySlotWidget::ApplyFixedSlotLayout ()
{
    const float FixedSlotSize = GetGridInventoryFixedSlotSize (this);

    if (SizeBox_Root)
    {
        SizeBox_Root->SetWidthOverride (FixedSlotSize);
        SizeBox_Root->SetHeightOverride (FixedSlotSize);
        SizeBox_Root->SetMinDesiredWidth (FixedSlotSize);
        SizeBox_Root->SetMinDesiredHeight (FixedSlotSize);
        SizeBox_Root->SetMaxDesiredWidth (FixedSlotSize);
        SizeBox_Root->SetMaxDesiredHeight (FixedSlotSize);
    }

    SetRenderScale (FVector2D (1.0f, 1.0f));

    bool bGridLayoutApplied = SlotType != EGridInventoryUiSlotType::Inventory;
    if (UUniformGridPanel* UniformGridPanel = Cast<UUniformGridPanel> (GetParent ()))
    {
        UniformGridPanel->SetMinDesiredSlotWidth (FixedSlotSize);
        UniformGridPanel->SetMinDesiredSlotHeight (FixedSlotSize);
        bGridLayoutApplied = true;
    }

    bool bGridSlotApplied = SlotType != EGridInventoryUiSlotType::Inventory;
    if (UUniformGridSlot* UniformGridSlot = Cast<UUniformGridSlot> (Slot))
    {
        UniformGridSlot->SetHorizontalAlignment (HAlign_Left);
        UniformGridSlot->SetVerticalAlignment (VAlign_Top);
        bGridSlotApplied = true;
    }

    bFixedSlotLayoutApplied = SizeBox_Root && bGridLayoutApplied && bGridSlotApplied;
}

void UGridInventorySlotWidget::SetItem (const FGridItemInstance& InItem)
{
    CachedItem = InItem;
    bHasItem = true;
    CachedIconTexture = nullptr;
    if (OwningInventoryWidget && OwningInventoryWidget->InventoryComponent)
    {
        if (const UGridItemDefinitionAsset* Definition = OwningInventoryWidget->InventoryComponent->FindItemDefinition (CachedItem.ItemDefinitionId))
        {
            CachedIconTexture = Definition->Icon.LoadSynchronous ();
        }
    }

    RefreshSlotVisual ();
}

void UGridInventorySlotWidget::ClearItem ()
{
    CachedItem = FGridItemInstance ();
    bHasItem = false;
    CachedIconTexture = nullptr;
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

const UGridItemDefinitionAsset* UGridInventorySlotWidget::GetItemDefinition () const
{
    if (!bHasItem || CachedItem.ItemDefinitionId.IsNone ())
    {
        return nullptr;
    }

    const UGridInventoryWidget* InventoryWidget = OwningInventoryWidget.Get ();
    const UGridPartyInventoryComponent* InventoryComponent =
        InventoryWidget ? InventoryWidget->InventoryComponent : nullptr;

    return InventoryComponent
        ? InventoryComponent->FindItemDefinition (CachedItem.ItemDefinitionId)
        : nullptr;
}

FString UGridInventorySlotWidget::GetDisplayNameText () const
{
    if (!bHasItem || CachedItem.ItemDefinitionId.IsNone ())
    {
        return FString ();
    }

    const UGridItemDefinitionAsset* Definition = GetItemDefinition ();
    if (Definition && !Definition->DisplayName.IsEmpty ())
    {
        return Definition->DisplayName.ToString ();
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

FText UGridInventorySlotWidget::GetItemTypeDisplayText () const
{
    const UGridItemDefinitionAsset* Definition = GetItemDefinition ();
    if (!Definition)
    {
        return FText::GetEmpty ();
    }

    switch (Definition->ItemType)
    {
    case EGridItemType::Torch:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeTorch", "Source de lumière");
    case EGridItemType::Weapon:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeWeapon", "Arme");
    case EGridItemType::Shield:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeShield", "Bouclier");
    case EGridItemType::Armor:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeArmor", "Armure");
    case EGridItemType::Key:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeKey", "Clé");
    case EGridItemType::Potion:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypePotion", "Potion");
    case EGridItemType::Scroll:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeScroll", "Parchemin");
    case EGridItemType::Book:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeBook", "Livre");
    case EGridItemType::Food:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeFood", "Nourriture");
    case EGridItemType::Quest:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeQuest", "Objet de quête");
    case EGridItemType::None:
    case EGridItemType::Misc:
    default:
        return NSLOCTEXT ("GridInventoryTooltip", "ItemTypeGeneric", "Objet");
    }
}

FText UGridInventorySlotWidget::GetCompatibleEquipmentSlotsText () const
{
    const UGridItemDefinitionAsset* Definition = GetItemDefinition ();
    if (!Definition)
    {
        return FText::GetEmpty ();
    }

    const bool bSupportsMainHand =
        Definition->CompatibleEquipmentSlots.Contains (EGridEquipmentSlot::MainHand);
    const bool bSupportsOffHand =
        Definition->CompatibleEquipmentSlots.Contains (EGridEquipmentSlot::OffHand);

    if (bSupportsMainHand && bSupportsOffHand)
    {
        return NSLOCTEXT (
            "GridInventoryTooltip",
            "EquipmentSlotsBothHands",
            "Main directrice, Main secondaire");
    }

    if (bSupportsMainHand)
    {
        return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotMainHand", "Main directrice");
    }

    if (bSupportsOffHand)
    {
        return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotOffHand", "Main secondaire");
    }

    return FText::GetEmpty ();
}

FText UGridInventorySlotWidget::GetLightTooltipText () const
{
    const UGridItemDefinitionAsset* Definition = GetItemDefinition ();
    return Definition && Definition->bCanEmitLight
        ? NSLOCTEXT ("GridInventoryTooltip", "EmitsLight", "Lumière : oui")
        : FText::GetEmpty ();
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
    return CachedIconTexture.Get ();
}

void UGridInventorySlotWidget::HandleClicked ()
{
    if (OwningInventoryWidget && SlotType == EGridInventoryUiSlotType::Inventory)
    {
        OwningInventoryWidget->HandleInventorySlotClicked (InventorySlotIndex, bSplitStackRequestedByClick);
        bSplitStackRequestedByClick = false;
        return;
    }

    OnSlotClicked.Broadcast (SlotType, InventorySlotIndex);
}

void UGridInventorySlotWidget::SetOwnerInventoryWidget (UGridInventoryWidget* InOwnerInventoryWidget)
{
    OwningInventoryWidget = InOwnerInventoryWidget;
    bFixedSlotLayoutApplied = false;
    ApplyFixedSlotLayout ();
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
    bSplitStackRequestedByClick =
        SlotType == EGridInventoryUiSlotType::Inventory &&
        InMouseEvent.IsControlDown ();

    if (InMouseEvent.GetEffectingButton () == EKeys::RightMouseButton &&
        bHasItem &&
        OwningInventoryWidget)
    {
        OwningInventoryWidget->HandleItemSlotRightClicked (
            SlotType,
            InventorySlotIndex);
        return FReply::Handled ();
    }

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

    Operation->bSplitStack =
        SlotType == EGridInventoryUiSlotType::Inventory &&
        CachedItem.Quantity > 1 &&
        InMouseEvent.IsControlDown ();
    Operation->RequestedQuantity = Operation->bSplitStack ? 1 : 0;
    bSplitStackRequestedByClick = false;
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
        InventorySlotIndex,
        Operation->bSplitStack,
        Operation->RequestedQuantity);
}
