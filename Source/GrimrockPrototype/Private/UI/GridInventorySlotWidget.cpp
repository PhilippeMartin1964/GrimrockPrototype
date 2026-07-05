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
    FText GetEquipmentSlotDisplayName (EGridEquipmentSlot Slot)
    {
        switch (Slot)
        {
        case EGridEquipmentSlot::MainHand:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotMainHand", "Main directrice");
        case EGridEquipmentSlot::OffHand:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotOffHand", "Main secondaire");
        case EGridEquipmentSlot::Head:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotHead", "Tete");
        case EGridEquipmentSlot::Chest:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotChest", "Torse");
        case EGridEquipmentSlot::Legs:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotLegs", "Jambes");
        case EGridEquipmentSlot::Feet:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotFeet", "Pieds");
        case EGridEquipmentSlot::Amulet:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotAmulet", "Amulette");
        case EGridEquipmentSlot::Ring1:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotRing1", "Anneau I");
        case EGridEquipmentSlot::Ring2:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotRing2", "Anneau II");
        case EGridEquipmentSlot::Shoulders:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotShoulders", "Epaules");
        case EGridEquipmentSlot::Gloves:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotGloves", "Gants");
        case EGridEquipmentSlot::Belt:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotBelt", "Ceinture");
        case EGridEquipmentSlot::Cloak:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotCloak", "Cape");
        case EGridEquipmentSlot::Talisman:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotTalisman", "Talisman");
        case EGridEquipmentSlot::QuickSlot1:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotQuickSlot1", "Raccourci I");
        case EGridEquipmentSlot::QuickSlot2:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotQuickSlot2", "Raccourci II");
        case EGridEquipmentSlot::Face:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotFace", "Visage");
        case EGridEquipmentSlot::Shirt:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotShirt", "Chemise");
        case EGridEquipmentSlot::Bracers:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotBracers", "Brassards");
        case EGridEquipmentSlot::Earring1:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotEarring1", "Bijou d'oreille I");
        case EGridEquipmentSlot::Earring2:
            return NSLOCTEXT ("GridInventoryTooltip", "EquipmentSlotEarring2", "Bijou d'oreille II");
        case EGridEquipmentSlot::None:
        default:
            return FText::GetEmpty ();
        }
    }
}

void UGridInventorySlotWidget::InitializeInventorySlot (
    EGridInventoryUiSlotType InSlotType,
    int32 InInventorySlotIndex)
{
    SlotType = InSlotType;
    InventorySlotIndex = InInventorySlotIndex;
    EquipmentSlot = EGridEquipmentSlot::None;
    if (SlotType == EGridInventoryUiSlotType::MainHand)
    {
        EquipmentSlot = EGridEquipmentSlot::MainHand;
    }
    else if (SlotType == EGridInventoryUiSlotType::OffHand)
    {
        EquipmentSlot = EGridEquipmentSlot::OffHand;
    }
    bFixedSlotLayoutApplied = false;
    ApplyFixedSlotLayout ();
}

void UGridInventorySlotWidget::InitializeEquipmentSlot (EGridEquipmentSlot InEquipmentSlot)
{
    SlotType = EGridInventoryUiSlotType::Equipment;
    EquipmentSlot = InEquipmentSlot;
    InventorySlotIndex = static_cast<int32> (InEquipmentSlot);
    bFixedSlotLayoutApplied = false;
    ApplyFixedSlotLayout ();
}

void UGridInventorySlotWidget::SetSlotLogicalExtent (float InSlotLogicalExtent)
{
    SlotLogicalExtent = FMath::Clamp (InSlotLogicalExtent, 16.0f, 256.0f);
    bFixedSlotLayoutApplied = false;
    ApplyFixedSlotLayout ();
}

float UGridInventorySlotWidget::GetSlotLogicalExtent () const
{
    return SlotLogicalExtent;
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
    const float EffectiveSlotLogicalExtent = FMath::Clamp (SlotLogicalExtent, 16.0f, 256.0f);

    if (SizeBox_Root)
    {
        SizeBox_Root->SetWidthOverride (EffectiveSlotLogicalExtent);
        SizeBox_Root->SetHeightOverride (EffectiveSlotLogicalExtent);
    }

    SetRenderScale (FVector2D (1.0f, 1.0f));

    bool bGridLayoutApplied = SlotType != EGridInventoryUiSlotType::Inventory;
    if (Cast<UUniformGridPanel> (GetParent ()))
    {
        bGridLayoutApplied = true;
    }

    bool bGridSlotApplied = SlotType != EGridInventoryUiSlotType::Inventory;
    if (UUniformGridSlot* UniformGridSlot = Cast<UUniformGridSlot> (Slot))
    {
        UniformGridSlot->SetHorizontalAlignment (HAlign_Left);
        UniformGridSlot->SetVerticalAlignment (VAlign_Top);
        bGridSlotApplied = true;
    }

    bFixedSlotLayoutApplied = bGridLayoutApplied && bGridSlotApplied;
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

    TArray<FString> SlotLabels;
    for (const EGridEquipmentSlot CompatibleSlot : Definition->CompatibleEquipmentSlots)
    {
        const FText SlotLabel = GetEquipmentSlotDisplayName (CompatibleSlot);
        if (!SlotLabel.IsEmpty ())
        {
            SlotLabels.Add (SlotLabel.ToString ());
        }
    }

    return SlotLabels.Num () > 0
        ? FText::FromString (FString::Join (SlotLabels, TEXT (", ")))
        : FText::GetEmpty ();
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
