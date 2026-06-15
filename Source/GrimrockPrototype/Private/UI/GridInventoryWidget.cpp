#include "UI/GridInventoryWidget.h"

#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

void UGridInventoryWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
}

void UGridInventoryWidget::InitializeInventoryWidget (AGrimrockPartyPawn* InPartyPawn)
{
    OwningPartyPawn = InPartyPawn;
    InventoryComponent = InPartyPawn ? InPartyPawn->PartyInventoryComponent : nullptr;
    RefreshInventory ();
}

void UGridInventoryWidget::RefreshInventory_Implementation ()
{
    UE_LOG (LogTemp, Verbose, TEXT ("GridInventory UI Refresh Pawn=%s InventoryComponent=%s"),
        *GetNameSafe (OwningPartyPawn),
        *GetNameSafe (InventoryComponent));
    RefreshRegisteredPartyMemberWidgets ();
    RefreshRegisteredSlotWidgets ();
}

int32 UGridInventoryWidget::GetSelectedCharacterIndex () const
{
    return InventoryComponent ? InventoryComponent->GetSelectedCharacterIndex () : INDEX_NONE;
}

int32 UGridInventoryWidget::GetInventorySlotCount () const
{
    if (!InventoryComponent)
    {
        return 0;
    }

    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
    return State.ActiveCharacters.IsValidIndex (CharacterIndex)
        ? State.ActiveCharacters[CharacterIndex].InventorySlots.Num ()
        : 0;
}

bool UGridInventoryWidget::GetInventoryItemAtSlot (int32 SlotIndex, FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!InventoryComponent)
    {
        return false;
    }

    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
    if (!State.ActiveCharacters.IsValidIndex (CharacterIndex))
    {
        return false;
    }

    const FGridCharacterInventoryState& CharacterState = State.ActiveCharacters[CharacterIndex];
    if (!CharacterState.InventorySlots.IsValidIndex (SlotIndex) ||
        CharacterState.InventorySlots[SlotIndex].IsEmpty ())
    {
        return false;
    }

    OutItem = CharacterState.InventorySlots[SlotIndex].Item;
    return true;
}

bool UGridInventoryWidget::GetMainHandItem (FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!InventoryComponent)
    {
        return false;
    }
    return InventoryComponent->GetEquippedItem (
        InventoryComponent->GetSelectedCharacterIndex (),
        EGridEquipmentSlot::MainHand,
        OutItem);
}

bool UGridInventoryWidget::GetOffHandItem (FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!InventoryComponent)
    {
        return false;
    }
    return InventoryComponent->GetEquippedItem (
        InventoryComponent->GetSelectedCharacterIndex (),
        EGridEquipmentSlot::OffHand,
        OutItem);
}

bool UGridInventoryWidget::GetCursorItem (FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!InventoryComponent || !InventoryComponent->HasCursorItem ())
    {
        return false;
    }

    OutItem = InventoryComponent->GetCursorItem ();
    return true;
}

bool UGridInventoryWidget::HasCursorItem () const
{
    return InventoryComponent && InventoryComponent->HasCursorItem ();
}

FString UGridInventoryWidget::GetItemDisplayString (const FGridItemInstance& Item) const
{
    return Item.ItemDefinitionId.IsNone () ? FString (TEXT ("Empty")) : Item.ItemDefinitionId.ToString ();
}

FString UGridInventoryWidget::GetCursorItemDisplayText () const
{
    FGridItemInstance Item;
    GetCursorItem (Item);
    return FString::Printf (TEXT ("Cursor: %s"), *GetItemDisplayString (Item));
}

FString UGridInventoryWidget::GetMainHandDisplayText () const
{
    FGridItemInstance Item;
    GetMainHandItem (Item);
    return FString::Printf (TEXT ("MainHand: %s"), *GetItemDisplayString (Item));
}

FString UGridInventoryWidget::GetOffHandDisplayText () const
{
    FGridItemInstance Item;
    GetOffHandItem (Item);
    return FString::Printf (TEXT ("OffHand: %s"), *GetItemDisplayString (Item));
}

FString UGridInventoryWidget::GetInventorySlotDisplayText (int32 SlotIndex) const
{
    FGridItemInstance Item;
    GetInventoryItemAtSlot (SlotIndex, Item);
    return FString::Printf (TEXT ("Slot %d: %s"), SlotIndex, *GetItemDisplayString (Item));
}

int32 UGridInventoryWidget::GetActiveCharacterCount () const
{
    return InventoryComponent ? InventoryComponent->GetActiveCharacterCount () : 0;
}

int32 UGridInventoryWidget::GetMaxActiveCharacterCount () const
{
    return InventoryComponent ? InventoryComponent->GetMaxActiveCharacterCount () : 0;
}

bool UGridInventoryWidget::GetCharacterSummary (
    int32 CharacterIndex,
    FGridInventoryCharacterSummary& OutSummary) const
{
    OutSummary = FGridInventoryCharacterSummary ();
    return InventoryComponent && InventoryComponent->GetCharacterSummary (CharacterIndex, OutSummary);
}

bool UGridInventoryWidget::SelectCharacter (int32 CharacterIndex)
{
    const bool bResult = InventoryComponent && InventoryComponent->SetSelectedCharacterIndex (CharacterIndex);
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI SelectCharacter Index=%d Result=%s"),
        CharacterIndex,
        bResult ? TEXT ("true") : TEXT ("false"));
    RefreshInventory ();
    return bResult;
}

FString UGridInventoryWidget::GetCharacterDisplayText (int32 CharacterIndex) const
{
    FGridInventoryCharacterSummary Summary;
    if (!GetCharacterSummary (CharacterIndex, Summary))
    {
        return FString::Printf (TEXT ("%d Empty"), CharacterIndex);
    }

    const FString NameText = Summary.DisplayName.IsEmpty ()
        ? FString::Printf (TEXT ("Hero_%02d"), CharacterIndex + 1)
        : Summary.DisplayName.ToString ();
    return FString::Printf (TEXT ("%d %s"), CharacterIndex, *NameText);
}

FString UGridInventoryWidget::GetSelectedCharacterDisplayText () const
{
    const int32 CharacterIndex = GetSelectedCharacterIndex ();
    FGridInventoryCharacterSummary Summary;
    if (!GetCharacterSummary (CharacterIndex, Summary))
    {
        return TEXT ("SelectedCharacter: None");
    }

    const FString NameText = Summary.DisplayName.IsEmpty ()
        ? FString::Printf (TEXT ("Hero_%02d"), CharacterIndex + 1)
        : Summary.DisplayName.ToString ();
    const FString ClassText = Summary.ClassId.IsNone () ? FString (TEXT ("Unknown")) : Summary.ClassId.ToString ();
    return FString::Printf (
        TEXT ("SelectedCharacter: %d %s %s Lv%d"),
        CharacterIndex,
        *NameText,
        *ClassText,
        Summary.Level);
}

void UGridInventoryWidget::RegisterPartyMemberWidget (
    UGridPartyMemberWidget* MemberWidget,
    int32 CharacterIndex)
{
    if (!MemberWidget)
    {
        return;
    }

    MemberWidget->InitializePartyMember (CharacterIndex);
    MemberWidget->OnPartyMemberClicked.RemoveDynamic (this, &UGridInventoryWidget::HandleRegisteredPartyMemberClicked);
    MemberWidget->OnPartyMemberClicked.AddDynamic (this, &UGridInventoryWidget::HandleRegisteredPartyMemberClicked);
    RegisteredPartyMemberWidgets.AddUnique (MemberWidget);
    RefreshRegisteredPartyMemberWidgets ();
}

void UGridInventoryWidget::RefreshRegisteredPartyMemberWidgets ()
{
    for (UGridPartyMemberWidget* MemberWidget : RegisteredPartyMemberWidgets)
    {
        if (!MemberWidget)
        {
            continue;
        }

        FGridInventoryCharacterSummary Summary;
        if (GetCharacterSummary (MemberWidget->CharacterIndex, Summary))
        {
            MemberWidget->SetVisibility (ESlateVisibility::Visible);
            MemberWidget->SetCharacterSummary (Summary);
        }
        else
        {
            MemberWidget->SetVisibility (ESlateVisibility::Collapsed);
        }
    }
}

void UGridInventoryWidget::HandleRegisteredPartyMemberClicked (int32 CharacterIndex)
{
    SelectCharacter (CharacterIndex);
}

void UGridInventoryWidget::RegisterInventorySlotWidget (
    UGridInventorySlotWidget* SlotWidget,
    EGridInventoryUiSlotType SlotType,
    int32 SlotIndex)
{
    if (!SlotWidget)
    {
        return;
    }

    SlotWidget->InitializeInventorySlot (SlotType, SlotIndex);
    SlotWidget->SetOwnerInventoryWidget (this);
    SlotWidget->OnSlotClicked.RemoveDynamic (this, &UGridInventoryWidget::HandleRegisteredSlotClicked);
    SlotWidget->OnSlotClicked.AddDynamic (this, &UGridInventoryWidget::HandleRegisteredSlotClicked);

    switch (SlotType)
    {
    case EGridInventoryUiSlotType::Inventory:
        RegisteredInventorySlots.AddUnique (SlotWidget);
        break;
    case EGridInventoryUiSlotType::MainHand:
        MainHandSlotWidget = SlotWidget;
        break;
    case EGridInventoryUiSlotType::OffHand:
        OffHandSlotWidget = SlotWidget;
        break;
    case EGridInventoryUiSlotType::Cursor:
        CursorSlotWidget = SlotWidget;
        break;
    default:
        break;
    }

    RefreshRegisteredSlotWidgets ();
}

void UGridInventoryWidget::RebuildInventorySlotWidgets ()
{
    if (!InventorySlotsGridPanel)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI RebuildSlots Failed Reason=NoGridPanel"));
        return;
    }

    if (!InventorySlotWidgetClass)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI RebuildSlots Failed Reason=NoSlotWidgetClass"));
        return;
    }

    const int32 SlotCount = FMath::Max (1, ResolveInventorySlotWidgetCount ());
    const int32 ColumnCount = FMath::Max (1, InventorySlotColumnCount);
    if (bInventorySlotsBuilt &&
        LastBuiltSlotCount == SlotCount &&
        LastBuiltColumnCount == ColumnCount &&
        LastBuiltSlotWidgetClass == InventorySlotWidgetClass &&
        LastBuiltGridPanel == InventorySlotsGridPanel &&
        GeneratedInventorySlotWidgets.Num () == SlotCount)
    {
        UE_LOG (LogTemp, Verbose, TEXT ("GridInventory UI RebuildSlots Skipped Reason=AlreadyBuilt Count=%d Columns=%d"),
            SlotCount,
            ColumnCount);
        return;
    }

    ClearGeneratedInventorySlotWidgets ();

    for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
    {
        UGridInventorySlotWidget* NewSlot = CreateWidget<UGridInventorySlotWidget> (this, InventorySlotWidgetClass);
        if (!NewSlot)
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI RebuildSlots Failed Reason=CreateWidgetFailed Index=%d"), SlotIndex);
            continue;
        }

        NewSlot->InitializeInventorySlot (EGridInventoryUiSlotType::Inventory, SlotIndex);
        RegisterInventorySlotWidget (NewSlot, EGridInventoryUiSlotType::Inventory, SlotIndex);

        const int32 Row = SlotIndex / ColumnCount;
        const int32 Column = SlotIndex % ColumnCount;
        if (UUniformGridSlot* GridSlot = InventorySlotsGridPanel->AddChildToUniformGrid (NewSlot, Row, Column))
        {
            GridSlot->SetHorizontalAlignment (HAlign_Left);
            GridSlot->SetVerticalAlignment (VAlign_Top);
        }

        GeneratedInventorySlotWidgets.Add (NewSlot);
    }

    RefreshRegisteredSlotWidgets ();
    bInventorySlotsBuilt = true;
    LastBuiltSlotCount = SlotCount;
    LastBuiltColumnCount = ColumnCount;
    LastBuiltSlotWidgetClass = InventorySlotWidgetClass;
    LastBuiltGridPanel = InventorySlotsGridPanel;
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI RebuildSlots Count=%d Columns=%d"), SlotCount, ColumnCount);
}

void UGridInventoryWidget::ClearGeneratedInventorySlotWidgets ()
{
    RemoveGeneratedInventorySlotsFromRegistry ();

    for (UGridInventorySlotWidget* SlotWidget : GeneratedInventorySlotWidgets)
    {
        if (SlotWidget)
        {
            SlotWidget->RemoveFromParent ();
        }
    }

    if (InventorySlotsGridPanel)
    {
        InventorySlotsGridPanel->ClearChildren ();
    }

    GeneratedInventorySlotWidgets.Empty ();
    bInventorySlotsBuilt = false;
    LastBuiltSlotCount = 0;
    LastBuiltColumnCount = 0;
    LastBuiltSlotWidgetClass = nullptr;
    LastBuiltGridPanel = nullptr;
}

int32 UGridInventoryWidget::ResolveInventorySlotWidgetCount () const
{
    if (InventorySlotCountOverride > 0)
    {
        return InventorySlotCountOverride;
    }

    if (InventoryComponent)
    {
        FGridInventoryCharacterSummary Summary;
        if (InventoryComponent->GetCharacterSummary (InventoryComponent->GetSelectedCharacterIndex (), Summary) &&
            Summary.MaxInventorySlots > 0)
        {
            return Summary.MaxInventorySlots;
        }
    }

    return 24;
}

void UGridInventoryWidget::SetInventorySlotWidgetClass (TSubclassOf<UGridInventorySlotWidget> InClass)
{
    InventorySlotWidgetClass = InClass;
    bInventorySlotsBuilt = false;
}

void UGridInventoryWidget::SetInventorySlotsGridPanel (UUniformGridPanel* InGridPanel)
{
    InventorySlotsGridPanel = InGridPanel;
    bInventorySlotsBuilt = false;
}

void UGridInventoryWidget::RemoveGeneratedInventorySlotsFromRegistry ()
{
    RegisteredInventorySlots.RemoveAll ([this] (const TObjectPtr<UGridInventorySlotWidget>& SlotWidget)
    {
        if (!SlotWidget || SlotWidget->SlotType != EGridInventoryUiSlotType::Inventory)
        {
            return false;
        }

        return GeneratedInventorySlotWidgets.Contains (SlotWidget) ||
            (InventorySlotsGridPanel && SlotWidget->GetParent () == InventorySlotsGridPanel);
    });
}

void UGridInventoryWidget::RefreshRegisteredSlotWidgets ()
{
    for (UGridInventorySlotWidget* SlotWidget : RegisteredInventorySlots)
    {
        if (!SlotWidget)
        {
            continue;
        }

        FGridItemInstance Item;
        if (GetInventoryItemAtSlot (SlotWidget->InventorySlotIndex, Item))
        {
            SlotWidget->SetItem (Item);
        }
        else
        {
            SlotWidget->ClearItem ();
        }
    }

    if (MainHandSlotWidget)
    {
        FGridItemInstance Item;
        if (GetMainHandItem (Item))
        {
            MainHandSlotWidget->SetItem (Item);
        }
        else
        {
            MainHandSlotWidget->ClearItem ();
        }
    }

    if (OffHandSlotWidget)
    {
        FGridItemInstance Item;
        if (GetOffHandItem (Item))
        {
            OffHandSlotWidget->SetItem (Item);
        }
        else
        {
            OffHandSlotWidget->ClearItem ();
        }
    }

    if (CursorSlotWidget)
    {
        FGridItemInstance Item;
        if (GetCursorItem (Item))
        {
            CursorSlotWidget->SetItem (Item);
        }
        else
        {
            CursorSlotWidget->ClearItem ();
        }
    }
}

void UGridInventoryWidget::HandleRegisteredSlotClicked (EGridInventoryUiSlotType SlotType, int32 SlotIndex)
{
    switch (SlotType)
    {
    case EGridInventoryUiSlotType::Inventory:
        HandleInventorySlotClicked (SlotIndex);
        break;
    case EGridInventoryUiSlotType::MainHand:
        HandleMainHandClicked ();
        break;
    case EGridInventoryUiSlotType::OffHand:
        HandleOffHandClicked ();
        break;
    case EGridInventoryUiSlotType::Cursor:
        HandleCursorReturnToInventoryClicked ();
        break;
    default:
        break;
    }
}

bool UGridInventoryWidget::HandleItemSlotRightClicked (
    EGridInventoryUiSlotType SlotType,
    int32 SlotIndex)
{
    LastContextItem = FGridItemInstance ();
    LastFacingTargetContext = FGridFacingTargetContext ();
    LastContextActions.Reset ();

    const bool bBuilt = BuildContextActionsForSlot (
        SlotType,
        SlotIndex,
        LastFacingTargetContext,
        LastContextActions);

    UE_LOG (LogTemp, Log,
        TEXT ("GridInventory RightClick Slot=%d Item=%s Actions=%d Result=%s"),
        SlotIndex,
        LastContextItem.ItemDefinitionId.IsNone ()
            ? TEXT ("None")
            : *LastContextItem.ItemDefinitionId.ToString (),
        LastContextActions.Num (),
        bBuilt ? TEXT ("true") : TEXT ("false"));

    if (bBuilt)
    {
        OnContextActionsRequested.Broadcast (SlotType, SlotIndex);
    }
    return bBuilt;
}

bool UGridInventoryWidget::BuildContextActionsForSlot (
    EGridInventoryUiSlotType SlotType,
    int32 SlotIndex,
    FGridFacingTargetContext& OutFacingTarget,
    TArray<FGridItemContextAction>& OutActions)
{
    OutFacingTarget = FGridFacingTargetContext ();
    OutActions.Reset ();
    LastContextItem = FGridItemInstance ();
    if (!OwningPartyPawn || !InventoryComponent)
    {
        return false;
    }

    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    if (SlotType == EGridInventoryUiSlotType::Inventory)
    {
        if (!GetInventoryItemAtSlot (SlotIndex, LastContextItem))
        {
            return false;
        }

        return UGridItemContextActionLibrary::BuildInventorySlotContextActions (
            OwningPartyPawn,
            CharacterIndex,
            SlotIndex,
            OutFacingTarget,
            OutActions);
    }

    FGridItemActionContext ItemContext;
    ItemContext.PartyPawn = OwningPartyPawn;
    ItemContext.CharacterIndex = CharacterIndex;
    ItemContext.InventorySlotIndex = INDEX_NONE;

    switch (SlotType)
    {
    case EGridInventoryUiSlotType::MainHand:
        if (!GetMainHandItem (LastContextItem))
        {
            return false;
        }
        ItemContext.EquipmentSlot = EGridEquipmentSlot::MainHand;
        break;
    case EGridInventoryUiSlotType::OffHand:
        if (!GetOffHandItem (LastContextItem))
        {
            return false;
        }
        ItemContext.EquipmentSlot = EGridEquipmentSlot::OffHand;
        break;
    case EGridInventoryUiSlotType::Cursor:
        if (!GetCursorItem (LastContextItem))
        {
            return false;
        }
        break;
    default:
        return false;
    }

    ItemContext.Item = LastContextItem;
    ItemContext.ItemDefinition =
        InventoryComponent->FindItemDefinition (LastContextItem.ItemDefinitionId);
    return UGridItemContextActionLibrary::BuildItemContextActions (
        ItemContext,
        OutFacingTarget,
        OutActions);
}

bool UGridInventoryWidget::HandleSlotDrop (
    EGridInventoryUiSlotType SourceType,
    int32 SourceIndex,
    EGridInventoryUiSlotType TargetType,
    int32 TargetIndex,
    bool bSplitStack,
    int32 RequestedQuantity)
{
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop Source=%s SourceIndex=%d Target=%s TargetIndex=%d"),
        GetGridInventoryUiSlotTypeName (SourceType),
        SourceIndex,
        GetGridInventoryUiSlotTypeName (TargetType),
        TargetIndex);

    if (!InventoryComponent || !OwningPartyPawn)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI Drop Failed Reason=MissingPawnOrInventoryComponent"));
        RefreshInventory ();
        return false;
    }

    if (SourceType == TargetType && SourceIndex == TargetIndex)
    {
        UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop Result=true Reason=SameSlot"));
        RefreshInventory ();
        return true;
    }

    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    auto ValidateOwnership = [&] ()
    {
        FString OwnershipError;
        if (!InventoryComponent->ValidateInventoryOwnership (OwnershipError))
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI Drop Ownership Failed Error=%s"), *OwnershipError);
        }
        else
        {
            UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop Ownership OK"));
        }
    };

    if (TargetType == EGridInventoryUiSlotType::Inventory)
    {
        if (TargetIndex < 0 || TargetIndex >= GetInventorySlotCount ())
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI Drop Failed Reason=InvalidTargetIndex Target=%d"),
                TargetIndex);
            RefreshInventory ();
            return false;
        }

        bool bInventoryTargetResult = false;
        switch (SourceType)
        {
        case EGridInventoryUiSlotType::Inventory:
            if (bSplitStack)
            {
                const bool bTookSplitStack = InventoryComponent->TryTakeInventorySlotQuantityToCursor (
                    CharacterIndex,
                    SourceIndex,
                    FMath::Max (1, RequestedQuantity));
                bInventoryTargetResult =
                    bTookSplitStack &&
                    InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot (CharacterIndex, TargetIndex);
                if (!bInventoryTargetResult && bTookSplitStack && InventoryComponent->HasCursorItem ())
                {
                    InventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory ();
                }
            }
            else
            {
                bInventoryTargetResult = InventoryComponent->TryMoveCharacterInventorySlot (
                    CharacterIndex,
                    SourceIndex,
                    TargetIndex);
            }
            UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop InventoryToInventory Source=%d Target=%d Result=%s"),
                SourceIndex,
                TargetIndex,
                bInventoryTargetResult ? TEXT ("true") : TEXT ("false"));
            break;

        case EGridInventoryUiSlotType::Cursor:
            bInventoryTargetResult = InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot (
                CharacterIndex,
                TargetIndex);
            UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop CursorToInventory Target=%d Result=%s"),
                TargetIndex,
                bInventoryTargetResult ? TEXT ("true") : TEXT ("false"));
            break;

        case EGridInventoryUiSlotType::MainHand:
            bInventoryTargetResult =
                OwningPartyPawn->TryTakeSelectedCharacterMainHandToCursor () &&
                InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot (CharacterIndex, TargetIndex);
            UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop MainHandToInventory Target=%d Result=%s"),
                TargetIndex,
                bInventoryTargetResult ? TEXT ("true") : TEXT ("false"));
            break;

        case EGridInventoryUiSlotType::OffHand:
            bInventoryTargetResult =
                OwningPartyPawn->TryTakeSelectedCharacterOffHandToCursor () &&
                InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot (CharacterIndex, TargetIndex);
            UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop OffHandToInventory Target=%d Result=%s"),
                TargetIndex,
                bInventoryTargetResult ? TEXT ("true") : TEXT ("false"));
            break;

        default:
            break;
        }

        ValidateOwnership ();
        RefreshInventory ();
        return bInventoryTargetResult;
    }

    auto HasCurrentSourceItem = [&] () -> bool
    {
        FGridItemInstance Item;
        switch (SourceType)
        {
        case EGridInventoryUiSlotType::Inventory:
            return GetInventoryItemAtSlot (SourceIndex, Item);
        case EGridInventoryUiSlotType::MainHand:
            return GetMainHandItem (Item);
        case EGridInventoryUiSlotType::OffHand:
            return GetOffHandItem (Item);
        case EGridInventoryUiSlotType::Cursor:
            return GetCursorItem (Item);
        default:
            return false;
        }
    };

    auto TakeSourceToCursor = [&] () -> bool
    {
        switch (SourceType)
        {
        case EGridInventoryUiSlotType::Inventory:
            return bSplitStack
                ? InventoryComponent->TryTakeInventorySlotQuantityToCursor (
                    CharacterIndex,
                    SourceIndex,
                    FMath::Max (1, RequestedQuantity))
                : InventoryComponent->TryTakeInventorySlotToCursor (CharacterIndex, SourceIndex);
        case EGridInventoryUiSlotType::MainHand:
            return OwningPartyPawn->TryTakeSelectedCharacterMainHandToCursor ();
        case EGridInventoryUiSlotType::OffHand:
            return OwningPartyPawn->TryTakeSelectedCharacterOffHandToCursor ();
        case EGridInventoryUiSlotType::Cursor:
            return InventoryComponent->HasCursorItem ();
        default:
            return false;
        }
    };

    auto PlaceCursorToTarget = [&] () -> bool
    {
        switch (TargetType)
        {
        case EGridInventoryUiSlotType::Inventory:
            UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop InventoryTargetIndex Informative TargetIndex=%d"), TargetIndex);
            return InventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory ();
        case EGridInventoryUiSlotType::MainHand:
            return OwningPartyPawn->TryEquipCursorItemToSelectedCharacterMainHand ();
        case EGridInventoryUiSlotType::OffHand:
            return OwningPartyPawn->TryEquipCursorItemToSelectedCharacterOffHand ();
        case EGridInventoryUiSlotType::Cursor:
            return InventoryComponent->HasCursorItem ();
        default:
            return false;
        }
    };

    if (!HasCurrentSourceItem ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI Drop Failed Reason=SourceEmpty"));
        RefreshInventory ();
        return false;
    }

    bool bResult = false;
    bool bTookSourceToCursor = false;

    if (SourceType == EGridInventoryUiSlotType::Cursor)
    {
        bResult = PlaceCursorToTarget ();
    }
    else
    {
        if (InventoryComponent->HasCursorItem ())
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI Drop Failed Reason=CursorOccupied"));
            RefreshInventory ();
            return false;
        }

        bTookSourceToCursor = TakeSourceToCursor ();
        if (!bTookSourceToCursor)
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI Drop Failed Reason=TakeSourceFailed"));
            RefreshInventory ();
            return false;
        }

        bResult = TargetType == EGridInventoryUiSlotType::Cursor ? true : PlaceCursorToTarget ();
    }

    if (!bResult && bTookSourceToCursor && InventoryComponent->HasCursorItem ())
    {
        const bool bRecoveryResult = InventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory ();
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI Drop Recovery Result=%s"),
            bRecoveryResult ? TEXT ("true") : TEXT ("false"));
    }

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Drop Result=%s"),
        bResult ? TEXT ("true") : TEXT ("false"));
    ValidateOwnership ();
    RefreshInventory ();
    return bResult;
}

bool UGridInventoryWidget::HandleInventorySlotClicked (int32 SlotIndex, bool bSplitStack)
{
    if (!InventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI SlotClicked Slot=%d CursorBefore=false Result=false Reason=NoInventoryComponent"),
            SlotIndex);
        RefreshInventory ();
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    const bool bResult = bCursorBefore
        ? InventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory ()
        : (bSplitStack
            ? InventoryComponent->TryTakeInventorySlotQuantityToCursor (CharacterIndex, SlotIndex, 1)
            : InventoryComponent->TryTakeInventorySlotToCursor (CharacterIndex, SlotIndex));

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI SlotClicked Slot=%d CursorBefore=%s Result=%s"),
        SlotIndex,
        bCursorBefore ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    RefreshInventory ();
    return bResult;
}

bool UGridInventoryWidget::HandleMainHandClicked ()
{
    if (!OwningPartyPawn || !InventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI MainHandClicked CursorBefore=false Result=false Reason=MissingPawnOrInventoryComponent"));
        RefreshInventory ();
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    const bool bResult = bCursorBefore
        ? OwningPartyPawn->TryEquipCursorItemToSelectedCharacterMainHand ()
        : OwningPartyPawn->TryTakeSelectedCharacterMainHandToCursor ();

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI MainHandClicked CursorBefore=%s Result=%s"),
        bCursorBefore ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    RefreshInventory ();
    return bResult;
}

bool UGridInventoryWidget::HandleOffHandClicked ()
{
    if (!OwningPartyPawn || !InventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI OffHandClicked CursorBefore=false Result=false Reason=MissingPawnOrInventoryComponent"));
        RefreshInventory ();
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    const bool bResult = bCursorBefore
        ? OwningPartyPawn->TryEquipCursorItemToSelectedCharacterOffHand ()
        : OwningPartyPawn->TryTakeSelectedCharacterOffHandToCursor ();

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI OffHandClicked CursorBefore=%s Result=%s"),
        bCursorBefore ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    RefreshInventory ();
    return bResult;
}

bool UGridInventoryWidget::HandleCursorReturnToInventoryClicked ()
{
    if (!OwningPartyPawn || !InventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI CursorReturnToInventory CursorBefore=false Result=false Reason=MissingPawnOrInventoryComponent"));
        RefreshInventory ();
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    if (!bCursorBefore)
    {
        UE_LOG (LogTemp, Log, TEXT ("GridInventory UI CursorReturnToInventory CursorBefore=false Result=false"));
        RefreshInventory ();
        return false;
    }

    const bool bResult = OwningPartyPawn->DebugPlaceCursorItemInSelectedInventory ();
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI CursorReturnToInventory CursorBefore=true Result=%s"),
        bResult ? TEXT ("true") : TEXT ("false"));

    RefreshInventory ();
    return bResult;
}
