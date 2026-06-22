#include "UI/GridInventoryWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemTransferService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
    void SetOptionalText (UTextBlock* TextBlock, const FText& Value)
    {
        if (TextBlock)
        {
            TextBlock->SetText (Value);
        }
    }

    FText ResolveCharacterDisplayName (const FText& DisplayName, FName Id, const TCHAR* Fallback)
    {
        if (!DisplayName.IsEmpty ())
        {
            return DisplayName;
        }
        return Id.IsNone () ? FText::FromString (Fallback) : FText::FromName (Id);
    }

    const TCHAR* GetContextActionName (EGridItemActionType ActionType)
    {
        switch (ActionType)
        {
        case EGridItemActionType::Equip: return TEXT ("Equip");
        case EGridItemActionType::Unequip: return TEXT ("Unequip");
        case EGridItemActionType::Consume: return TEXT ("Consume");
        case EGridItemActionType::Read: return TEXT ("Read");
        case EGridItemActionType::Examine: return TEXT ("Examine");
        case EGridItemActionType::Use: return TEXT ("Use");
        case EGridItemActionType::UseOnTarget: return TEXT ("UseOnTarget");
        case EGridItemActionType::InsertIntoTarget: return TEXT ("InsertIntoTarget");
        case EGridItemActionType::PlaceOnTarget: return TEXT ("PlaceOnTarget");
        case EGridItemActionType::DropToGround: return TEXT ("DropToGround");
        case EGridItemActionType::Throw: return TEXT ("Throw");
        case EGridItemActionType::Combine: return TEXT ("Combine");
        case EGridItemActionType::SplitStack: return TEXT ("SplitStack");
        case EGridItemActionType::ToggleLight: return TEXT ("ToggleLight");
        case EGridItemActionType::None:
        default:
            return TEXT ("None");
        }
    }

    const TCHAR* GetContextTargetTypeName (EGridFacingTargetType TargetType)
    {
        switch (TargetType)
        {
        case EGridFacingTargetType::WallLock: return TEXT ("WallLock");
        case EGridFacingTargetType::Receptacle: return TEXT ("Receptacle");
        case EGridFacingTargetType::TorchHolder: return TEXT ("TorchHolder");
        case EGridFacingTargetType::Readable: return TEXT ("Readable");
        case EGridFacingTargetType::Door: return TEXT ("Door");
        case EGridFacingTargetType::Mechanism: return TEXT ("Mechanism");
        case EGridFacingTargetType::None:
        default:
            return TEXT ("None");
        }
    }

    const TCHAR* GetContextEquipmentSlotName (EGridEquipmentSlot EquipmentSlot)
    {
        switch (EquipmentSlot)
        {
        case EGridEquipmentSlot::MainHand: return TEXT ("MainHand");
        case EGridEquipmentSlot::OffHand: return TEXT ("OffHand");
        case EGridEquipmentSlot::Head: return TEXT ("Head");
        case EGridEquipmentSlot::Chest: return TEXT ("Chest");
        case EGridEquipmentSlot::Legs: return TEXT ("Legs");
        case EGridEquipmentSlot::Feet: return TEXT ("Feet");
        case EGridEquipmentSlot::Amulet: return TEXT ("Amulet");
        case EGridEquipmentSlot::Ring1: return TEXT ("Ring1");
        case EGridEquipmentSlot::Ring2: return TEXT ("Ring2");
        case EGridEquipmentSlot::Shoulders: return TEXT ("Shoulders");
        case EGridEquipmentSlot::Gloves: return TEXT ("Gloves");
        case EGridEquipmentSlot::Belt: return TEXT ("Belt");
        case EGridEquipmentSlot::Cloak: return TEXT ("Cloak");
        case EGridEquipmentSlot::Talisman: return TEXT ("Talisman");
        case EGridEquipmentSlot::QuickSlot1: return TEXT ("QuickSlot1");
        case EGridEquipmentSlot::QuickSlot2: return TEXT ("QuickSlot2");
        case EGridEquipmentSlot::None:
        default:
            return TEXT ("None");
        }
    }
}

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
    RefreshSelectedCharacterDetails ();
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
    const FString ClassText = Summary.ClassDisplayName.IsEmpty ()
        ? (Summary.ClassId.IsNone () ? FString (TEXT ("Classe inconnue")) : Summary.ClassId.ToString ())
        : Summary.ClassDisplayName.ToString ();
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

void UGridInventoryWidget::RefreshSelectedCharacterDetails ()
{
    FGridInventoryCharacterSummary Summary;
    if (!GetCharacterSummary (GetSelectedCharacterIndex (), Summary))
    {
        SetOptionalText (Text_CharacterName, FText::GetEmpty ());
        SetOptionalText (Text_CharacterRace, FText::GetEmpty ());
        SetOptionalText (Text_CharacterClass, FText::GetEmpty ());
        SetOptionalText (Text_CharacterLevel, FText::GetEmpty ());
        SetOptionalText (Text_CharacterExperience, FText::GetEmpty ());
        SetOptionalText (Text_CharacterStrength, FText::GetEmpty ());
        SetOptionalText (Text_CharacterDexterity, FText::GetEmpty ());
        SetOptionalText (Text_CharacterConstitution, FText::GetEmpty ());
        SetOptionalText (Text_CharacterIntelligence, FText::GetEmpty ());
        SetOptionalText (Text_CharacterWisdom, FText::GetEmpty ());
        SetOptionalText (Text_CharacterCharisma, FText::GetEmpty ());
        SetOptionalText (Text_CharacterHealth, FText::GetEmpty ());
        SetOptionalText (Text_CharacterMana, FText::GetEmpty ());
        SetOptionalText (Text_CharacterCarryWeight, FText::GetEmpty ());
        if (Image_CharacterPortrait)
        {
            Image_CharacterPortrait->SetVisibility (ESlateVisibility::Collapsed);
        }
        return;
    }

    SetOptionalText (Text_CharacterName, Summary.DisplayName);
    SetOptionalText (
        Text_CharacterRace,
        ResolveCharacterDisplayName (Summary.RaceDisplayName, Summary.RaceId, TEXT ("Race inconnue")));
    SetOptionalText (
        Text_CharacterClass,
        ResolveCharacterDisplayName (Summary.ClassDisplayName, Summary.ClassId, TEXT ("Classe inconnue")));
    SetOptionalText (Text_CharacterLevel, FText::AsNumber (Summary.Level));
    SetOptionalText (Text_CharacterExperience, FText::AsNumber (Summary.Experience));
    SetOptionalText (Text_CharacterStrength, FText::AsNumber (Summary.Attributes.Strength));
    SetOptionalText (Text_CharacterDexterity, FText::AsNumber (Summary.Attributes.Dexterity));
    SetOptionalText (Text_CharacterConstitution, FText::AsNumber (Summary.Attributes.Constitution));
    SetOptionalText (Text_CharacterIntelligence, FText::AsNumber (Summary.Attributes.Intelligence));
    SetOptionalText (Text_CharacterWisdom, FText::AsNumber (Summary.Attributes.Wisdom));
    SetOptionalText (Text_CharacterCharisma, FText::AsNumber (Summary.Attributes.Charisma));
    SetOptionalText (
        Text_CharacterHealth,
        FText::FromString (FString::Printf (
            TEXT ("%d / %d"),
            Summary.DerivedStats.CurrentHealth,
            Summary.DerivedStats.MaxHealth)));
    SetOptionalText (
        Text_CharacterMana,
        FText::FromString (FString::Printf (
            TEXT ("%d / %d"),
            Summary.DerivedStats.CurrentMana,
            Summary.DerivedStats.MaxMana)));
    SetOptionalText (
        Text_CharacterCarryWeight,
        FText::FromString (FString::Printf (
            TEXT ("%.1f / %.1f"),
            Summary.CurrentWeight,
            Summary.MaxWeight)));

    if (Image_CharacterPortrait)
    {
        if (Summary.Portrait.IsNull ())
        {
            Image_CharacterPortrait->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Image_CharacterPortrait->SetBrushFromSoftTexture (Summary.Portrait, false);
            Image_CharacterPortrait->SetVisibility (ESlateVisibility::HitTestInvisible);
        }
    }
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

bool UGridInventoryWidget::ExecuteInventoryContextAction (
    EGridItemActionType ActionType,
    EGridInventoryUiSlotType SourceSlotType,
    int32 SourceSlotIndex)
{
    FGridFacingTargetContext FacingTarget;
    TArray<FGridItemContextAction> AvailableActions;
    if (!BuildContextActionsForSlot (
        SourceSlotType,
        SourceSlotIndex,
        FacingTarget,
        AvailableActions))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions Execute Failed Action=%s Reason=InvalidSource"),
            GetContextActionName (ActionType));
        return false;
    }

    const int32 MatchingActionCount = AvailableActions.FilterByPredicate (
        [ActionType] (const FGridItemContextAction& Action)
        {
            return Action.ActionType == ActionType;
        }).Num ();
    if (MatchingActionCount > 1)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions Execute Failed Action=%s Reason=AmbiguousActionType"),
            GetContextActionName (ActionType));
        return false;
    }

    const FGridItemContextAction* SelectedAction = AvailableActions.FindByPredicate (
        [ActionType] (const FGridItemContextAction& Action)
        {
            return Action.ActionType == ActionType;
        });
    if (!SelectedAction || !SelectedAction->bEnabled)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions Execute Failed Action=%s Item=%s Reason=%s"),
            GetContextActionName (ActionType),
            *LastContextItem.ItemDefinitionId.ToString (),
            SelectedAction
                ? TEXT ("ActionDisabled")
                : TEXT ("ActionUnavailable"));
        return false;
    }

    return ExecuteResolvedInventoryContextAction (
        *SelectedAction,
        FacingTarget,
        SourceSlotType,
        SourceSlotIndex);
}

bool UGridInventoryWidget::ExecuteInventoryContextActionByIndex (
    EGridInventoryUiSlotType SourceSlotType,
    int32 SourceSlotIndex,
    int32 ActionIndex)
{
    FGridFacingTargetContext FacingTarget;
    TArray<FGridItemContextAction> AvailableActions;
    if (!BuildContextActionsForSlot (
        SourceSlotType,
        SourceSlotIndex,
        FacingTarget,
        AvailableActions))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions ExecuteByIndex Failed Reason=InvalidSource Slot=%s:%d ActionIndex=%d"),
            GetGridInventoryUiSlotTypeName (SourceSlotType),
            SourceSlotIndex,
            ActionIndex);
        return false;
    }

    if (!AvailableActions.IsValidIndex (ActionIndex))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions ExecuteByIndex Failed Reason=InvalidActionIndex Slot=%s:%d ActionIndex=%d ActionCount=%d"),
            GetGridInventoryUiSlotTypeName (SourceSlotType),
            SourceSlotIndex,
            ActionIndex,
            AvailableActions.Num ());
        return false;
    }

    const FGridItemContextAction& SelectedAction = AvailableActions[ActionIndex];
    UE_LOG (LogTemp, Log,
        TEXT ("GridItemActions ExecuteByIndex Slot=%s:%d ActionIndex=%d Action=%s Label=\"%s\" EquipmentSlot=%s TargetType=%s"),
        GetGridInventoryUiSlotTypeName (SourceSlotType),
        SourceSlotIndex,
        ActionIndex,
        GetContextActionName (SelectedAction.ActionType),
        *SelectedAction.Label.ToString (),
        GetContextEquipmentSlotName (SelectedAction.EquipmentSlot),
        GetContextTargetTypeName (FacingTarget.TargetType));

    if (!SelectedAction.bEnabled)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions ExecuteByIndex Failed Reason=ActionDisabled Slot=%s:%d ActionIndex=%d Action=%s"),
            GetGridInventoryUiSlotTypeName (SourceSlotType),
            SourceSlotIndex,
            ActionIndex,
            GetContextActionName (SelectedAction.ActionType));
        return false;
    }

    return ExecuteResolvedInventoryContextAction (
        SelectedAction,
        FacingTarget,
        SourceSlotType,
        SourceSlotIndex);
}

void UGridInventoryWidget::CloseItemActionMenu (FName Reason)
{
    UE_LOG (LogTemp, Log,
        TEXT ("GridItemActionMenu Closed Reason=%s"),
        Reason.IsNone () ? TEXT ("Unspecified") : *Reason.ToString ());
    OnItemActionMenuCloseRequested (Reason);
}

void UGridInventoryWidget::CloseItemReadPanel (FName Reason)
{
    UE_LOG (LogTemp, Log,
        TEXT ("GridItemReadPanel Closed Reason=%s"),
        Reason.IsNone () ? TEXT ("Unspecified") : *Reason.ToString ());
    OnItemReadPanelCloseRequested (Reason);
}

bool UGridInventoryWidget::ExecuteResolvedInventoryContextAction (
    const FGridItemContextAction& Action,
    const FGridFacingTargetContext& FacingTarget,
    EGridInventoryUiSlotType SourceSlotType,
    int32 SourceSlotIndex)
{
    const int32 CharacterIndex = InventoryComponent
        ? InventoryComponent->GetSelectedCharacterIndex ()
        : INDEX_NONE;
    bool bExecuted = false;
    switch (Action.ActionType)
    {
    case EGridItemActionType::Examine:
        {
            UGridInventorySlotWidget* SourceWidget =
                FindRegisteredSlotWidget (SourceSlotType, SourceSlotIndex);
            const FText TooltipText = SourceWidget
                ? SourceWidget->GetTooltipText ()
                : LastContextItem.DisplayName;
            UE_LOG (LogTemp, Log,
                TEXT ("GridItemActions Execute Examine Item=%s"),
                *LastContextItem.ItemDefinitionId.ToString ());
            PresentItemExamination (LastContextItem, TooltipText);
            bExecuted = true;
            break;
        }

    case EGridItemActionType::Read:
        {
            const UGridItemDefinitionAsset* ItemDefinition = InventoryComponent
                ? InventoryComponent->FindItemDefinition (LastContextItem.ItemDefinitionId)
                : nullptr;
            FText Title = LastContextItem.ReadTitleOverride;
            if (Title.IsEmpty () && LastContextItem.ReadableContentAsset)
            {
                Title = LastContextItem.ReadableContentAsset->Title;
            }
            if (Title.IsEmpty ())
            {
                Title = LastContextItem.DisplayName;
            }
            if (Title.IsEmpty () && ItemDefinition)
            {
                Title = ItemDefinition->DisplayName;
            }
            if (Title.IsEmpty ())
            {
                Title = FText::FromName (LastContextItem.ItemDefinitionId);
            }

            FText ReadText = LastContextItem.ReadTextOverride;
            const TCHAR* ReadSource = TEXT ("InstanceOverride");
            FName ResolvedContentId = LastContextItem.ReadableContentId;
            if (ReadText.IsEmpty () && LastContextItem.ReadableContentAsset)
            {
                ReadText = LastContextItem.ReadableContentAsset->BodyText;
                ReadSource = TEXT ("ReadableContentAsset");
                if (ResolvedContentId.IsNone ())
                {
                    ResolvedContentId = LastContextItem.ReadableContentAsset->ReadableContentId;
                }
            }
            if (ReadText.IsEmpty () && ItemDefinition && !ItemDefinition->ReadText.IsEmpty ())
            {
                ReadText = ItemDefinition->ReadText;
                ReadSource = TEXT ("DefinitionFallback");
            }
            if (ReadText.IsEmpty ())
            {
                ReadText = NSLOCTEXT ("GridItemActions", "EmptyReadText", "Rien n'est écrit.");
                ReadSource = TEXT ("EmptyFallback");
            }

            UE_LOG (LogTemp, Log,
                TEXT ("GridItemReading Resolve Item=%s Source=%s Content=%s"),
                *LastContextItem.ItemDefinitionId.ToString (),
                ReadSource,
                ResolvedContentId.IsNone () ? TEXT ("None") : *ResolvedContentId.ToString ());
            UE_LOG (LogTemp, Log,
                TEXT ("GridItemActions Execute Read Item=%s"),
                *LastContextItem.ItemDefinitionId.ToString ());
            PresentItemReading (LastContextItem, Title, ReadText);
            bExecuted = true;
            break;
        }

    case EGridItemActionType::Equip:
        {
            UE_LOG (LogTemp, Log,
                TEXT ("GridItemActions Execute Equip Item=%s EquipmentSlot=%s"),
                *LastContextItem.ItemDefinitionId.ToString (),
                GetContextEquipmentSlotName (Action.EquipmentSlot));
            if (SourceSlotType == EGridInventoryUiSlotType::Inventory &&
                InventoryComponent &&
                OwningPartyPawn &&
                Action.EquipmentSlot != EGridEquipmentSlot::None)
            {
                const UGridItemDefinitionAsset* ItemDefinition =
                    InventoryComponent->FindItemDefinition (LastContextItem.ItemDefinitionId);
                if (!ItemDefinition ||
                    !ItemDefinition->CanEquipToSlot (Action.EquipmentSlot))
                {
                    UE_LOG (LogTemp, Warning,
                        TEXT ("GridItemActions Execute Equip Failed Item=%s EquipmentSlot=%s Reason=IncompatibleSlot"),
                        *LastContextItem.ItemDefinitionId.ToString (),
                        GetContextEquipmentSlotName (Action.EquipmentSlot));
                    break;
                }

                if (!InventoryComponent->CanEquipItemToSlot (
                    CharacterIndex,
                    LastContextItem,
                    Action.EquipmentSlot))
                {
                    UE_LOG (LogTemp, Warning,
                        TEXT ("GridItemActions Execute Equip Failed Item=%s EquipmentSlot=%s Reason=IncompatibleSlot"),
                        *LastContextItem.ItemDefinitionId.ToString (),
                        GetContextEquipmentSlotName (Action.EquipmentSlot));
                    break;
                }

                bExecuted = InventoryComponent->EquipItemFromInventorySlot (
                    CharacterIndex,
                    SourceSlotIndex,
                    Action.EquipmentSlot);
                if (bExecuted)
                {
                    OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment ();
                }
            }
            break;
        }

    case EGridItemActionType::Unequip:
        {
            const EGridEquipmentSlot SourceEquipmentSlot =
                ResolveSourceEquipmentSlot (Action, SourceSlotType);
            UE_LOG (LogTemp, Log,
                TEXT ("GridItemActions Execute Unequip Item=%s EquipmentSlot=%s"),
                *LastContextItem.ItemDefinitionId.ToString (),
                GetContextEquipmentSlotName (SourceEquipmentSlot));
            if (InventoryComponent &&
                OwningPartyPawn &&
                SourceEquipmentSlot != EGridEquipmentSlot::None)
            {
                if (!InventoryComponent->CanAddItemToCharacterInventory (
                    CharacterIndex,
                    LastContextItem))
                {
                    UE_LOG (LogTemp, Warning,
                        TEXT ("GridItemActions Execute Unequip Failed Item=%s Reason=NoFreeInventorySlot"),
                        *LastContextItem.ItemDefinitionId.ToString ());
                    break;
                }

                bExecuted = InventoryComponent->UnequipItemToInventory (
                    CharacterIndex,
                    SourceEquipmentSlot);
                if (bExecuted)
                {
                    OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment ();
                }
                else
                {
                    UE_LOG (LogTemp, Warning,
                        TEXT ("GridItemActions Execute Unequip Failed Item=%s Reason=NoFreeInventorySlot"),
                        *LastContextItem.ItemDefinitionId.ToString ());
                }
            }
            break;
        }

    case EGridItemActionType::InsertIntoTarget:
        {
            AGridWallLockActor* WallLock =
                Cast<AGridWallLockActor> (FacingTarget.TargetActor);
            UE_LOG (LogTemp, Log,
                TEXT ("GridItemActions Execute InsertIntoTarget Item=%s Target=%s"),
                *LastContextItem.ItemDefinitionId.ToString (),
                WallLock ? TEXT ("WallLock") : TEXT ("None"));
            if (SourceSlotType == EGridInventoryUiSlotType::Inventory &&
                WallLock &&
                InventoryComponent &&
                OwningPartyPawn)
            {
                bExecuted =
                    UGridItemTransferService::TransferInventorySlotToWallLock (
                        InventoryComponent,
                        CharacterIndex,
                        SourceSlotIndex,
                        WallLock,
                        OwningPartyPawn).bSuccess;
            }
            break;
        }

    case EGridItemActionType::PlaceOnTarget:
        {
            AGridReceptacleActor* Receptacle =
                Cast<AGridReceptacleActor> (FacingTarget.TargetActor);
            const bool bIsTorchHolder =
                FacingTarget.TargetType == EGridFacingTargetType::TorchHolder;
            const bool bIsReceptacleTarget =
                FacingTarget.TargetType == EGridFacingTargetType::Receptacle ||
                FacingTarget.TargetType == EGridFacingTargetType::TorchHolder;
            UE_LOG (LogTemp, Log,
                TEXT ("GridItemActions Execute PlaceOnTarget Item=%s Source=%s Target=%s"),
                *LastContextItem.ItemDefinitionId.ToString (),
                GetGridInventoryUiSlotTypeName (SourceSlotType),
                bIsTorchHolder ? TEXT ("TorchHolder") : (bIsReceptacleTarget ? TEXT ("Receptacle") : TEXT ("None")));
            if (!Receptacle || !InventoryComponent || !bIsReceptacleTarget)
            {
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridItemActions Execute PlaceOnTarget Failed Item=%s Source=%s Reason=TargetRejected"),
                    *LastContextItem.ItemDefinitionId.ToString (),
                    GetGridInventoryUiSlotTypeName (SourceSlotType));
                break;
            }

            FGridItemTransferResult TransferResult;
            if (SourceSlotType == EGridInventoryUiSlotType::Inventory)
            {
                TransferResult =
                    UGridItemTransferService::TransferInventorySlotToReceptacle (
                        InventoryComponent,
                        CharacterIndex,
                        SourceSlotIndex,
                        Receptacle);
            }
            else if (SourceSlotType == EGridInventoryUiSlotType::MainHand ||
                     SourceSlotType == EGridInventoryUiSlotType::OffHand)
            {
                const EGridEquipmentSlot SourceEquipmentSlot =
                    ResolveSourceEquipmentSlot (Action, SourceSlotType);
                TransferResult =
                    UGridItemTransferService::TransferEquipmentSlotToReceptacle (
                        InventoryComponent,
                        CharacterIndex,
                        SourceEquipmentSlot,
                        Receptacle);
            }
            else
            {
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridItemActions Execute PlaceOnTarget Failed Item=%s Source=%s Reason=InvalidSource"),
                    *LastContextItem.ItemDefinitionId.ToString (),
                    GetGridInventoryUiSlotTypeName (SourceSlotType));
                break;
            }

            bExecuted = TransferResult.bSuccess;
            if (!bExecuted)
            {
                const TCHAR* Reason =
                    TransferResult.Result == EGridItemTransferResult::InvalidSource
                        ? TEXT ("InvalidSource")
                        : TEXT ("TargetRejected");
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridItemActions Execute PlaceOnTarget Failed Item=%s Source=%s Reason=%s"),
                    *LastContextItem.ItemDefinitionId.ToString (),
                    GetGridInventoryUiSlotTypeName (SourceSlotType),
                    Reason);
                break;
            }

            if (bIsTorchHolder)
            {
                const int32 InsertedItemIndex =
                    Receptacle->GetContainedItemCount () - 1;
                const bool bTorchLightEnabled =
                    Receptacle->SetContainedItemLightsEnabled (
                        InsertedItemIndex,
                        true);
                UE_LOG (LogTemp, Log,
                    TEXT ("GridItemActions Execute PlaceOnTarget TorchLightEnabled=%s"),
                    bTorchLightEnabled ? TEXT ("true") : TEXT ("false"));
            }
            break;
        }

    case EGridItemActionType::DropToGround:
        bExecuted = DropContextItemToGround (
            Action,
            SourceSlotType,
            SourceSlotIndex);
        break;

    default:
        UE_LOG (LogTemp, Log,
            TEXT ("GridItemActions Execute NotImplemented Action=%s Item=%s"),
            GetContextActionName (Action.ActionType),
            *LastContextItem.ItemDefinitionId.ToString ());
        break;
    }

    if (bExecuted)
    {
        RefreshInventory ();
    }
    return bExecuted;
}

EGridEquipmentSlot UGridInventoryWidget::ResolveSourceEquipmentSlot (
    const FGridItemContextAction& Action,
    EGridInventoryUiSlotType SourceSlotType) const
{
    if (Action.EquipmentSlot != EGridEquipmentSlot::None)
    {
        return Action.EquipmentSlot;
    }

    switch (SourceSlotType)
    {
    case EGridInventoryUiSlotType::MainHand:
        return EGridEquipmentSlot::MainHand;
    case EGridInventoryUiSlotType::OffHand:
        return EGridEquipmentSlot::OffHand;
    default:
        return EGridEquipmentSlot::None;
    }
}

bool UGridInventoryWidget::DropContextItemToGround (
    const FGridItemContextAction& Action,
    EGridInventoryUiSlotType SourceSlotType,
    int32 SourceSlotIndex)
{
    if (!InventoryComponent || !OwningPartyPawn || !OwningPartyPawn->LevelRuntimeActor)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=MissingRuntime"),
            *LastContextItem.ItemDefinitionId.ToString ());
        return false;
    }

    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    if (SourceSlotType == EGridInventoryUiSlotType::Inventory)
    {
        const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
        if (!State.ActiveCharacters.IsValidIndex (CharacterIndex) ||
            !State.ActiveCharacters[CharacterIndex].InventorySlots.IsValidIndex (SourceSlotIndex) ||
            State.ActiveCharacters[CharacterIndex].InventorySlots[SourceSlotIndex].IsEmpty ())
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidInventorySlot"),
                *LastContextItem.ItemDefinitionId.ToString ());
            return false;
        }
    }
    else if (SourceSlotType == EGridInventoryUiSlotType::MainHand ||
             SourceSlotType == EGridInventoryUiSlotType::OffHand)
    {
        const EGridEquipmentSlot SourceEquipmentSlot =
            ResolveSourceEquipmentSlot (Action, SourceSlotType);
        if (!InventoryComponent->PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidEquipmentState"),
                *LastContextItem.ItemDefinitionId.ToString ());
            return false;
        }

        const FGridCharacterEquipmentState& EquipmentState =
            InventoryComponent->PartyInventoryState.ActiveEquipment[CharacterIndex];
        const FGridItemInstance* EquippedItem = EquipmentState.GetSlot (SourceEquipmentSlot);
        if (!EquippedItem || !EquippedItem->IsValid ())
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidEquipmentSlot"),
                *LastContextItem.ItemDefinitionId.ToString ());
            return false;
        }
    }
    else
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=UnsupportedSource"),
            *LastContextItem.ItemDefinitionId.ToString ());
        return false;
    }

    FGridItemInstance ItemToDrop = LastContextItem;
    ItemToDrop.OwnerType = EGridItemOwnerType::World;
    ItemToDrop.OwnerGuid = FGuid ();
    ItemToDrop.OwnerCharacterIndex = INDEX_NONE;
    ItemToDrop.EquipmentSlot = EGridEquipmentSlot::None;

    if (!OwningPartyPawn->LevelRuntimeActor->TryDropItemInstanceAtCell (
        ItemToDrop,
        OwningPartyPawn->CurrentCellX,
        OwningPartyPawn->CurrentCellY,
        EGridEdge::None,
        FVector::ZeroVector))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=WorldDropFailed"),
            *LastContextItem.ItemDefinitionId.ToString ());
        return false;
    }

    if (SourceSlotType == EGridInventoryUiSlotType::Inventory)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridItemActions Execute DropToGround Item=%s Source=Inventory Slot=%d"),
            *LastContextItem.ItemDefinitionId.ToString (),
            SourceSlotIndex);
        FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
        if (!State.ActiveCharacters.IsValidIndex (CharacterIndex) ||
            !State.ActiveCharacters[CharacterIndex].InventorySlots.IsValidIndex (SourceSlotIndex))
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidInventorySlotAfterDrop"),
                *LastContextItem.ItemDefinitionId.ToString ());
            return false;
        }

        State.ActiveCharacters[CharacterIndex].InventorySlots[SourceSlotIndex] = FGridInventorySlot ();
        InventoryComponent->RecalculateCharacterWeight (CharacterIndex);
        return true;
    }

    const EGridEquipmentSlot SourceEquipmentSlot =
        ResolveSourceEquipmentSlot (Action, SourceSlotType);
    UE_LOG (LogTemp, Log,
        TEXT ("GridItemActions Execute DropToGround Item=%s Source=%s"),
        *LastContextItem.ItemDefinitionId.ToString (),
        GetContextEquipmentSlotName (SourceEquipmentSlot));
    if (!InventoryComponent->PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidEquipmentStateAfterDrop"),
            *LastContextItem.ItemDefinitionId.ToString ());
        return false;
    }

    FGridCharacterEquipmentState& EquipmentState =
        InventoryComponent->PartyInventoryState.ActiveEquipment[CharacterIndex];
    if (FGridItemInstance* EquippedItem = EquipmentState.GetMutableSlot (SourceEquipmentSlot))
    {
        *EquippedItem = FGridItemInstance ();
        InventoryComponent->RecalculateCharacterWeight (CharacterIndex);
        OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment ();
        return true;
    }

    UE_LOG (LogTemp, Warning,
        TEXT ("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidEquipmentSlot"),
        *LastContextItem.ItemDefinitionId.ToString ());
    return false;
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

    auto ResolveEquipmentSlot = [] (EGridInventoryUiSlotType SlotType) -> EGridEquipmentSlot
    {
        switch (SlotType)
        {
        case EGridInventoryUiSlotType::MainHand:
            return EGridEquipmentSlot::MainHand;
        case EGridInventoryUiSlotType::OffHand:
            return EGridEquipmentSlot::OffHand;
        default:
            return EGridEquipmentSlot::None;
        }
    };

    auto GetMutableSlotItem = [&] (
        EGridInventoryUiSlotType SlotType,
        int32 SlotIndex) -> FGridItemInstance*
    {
        if (!InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex (CharacterIndex) ||
            !InventoryComponent->PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
        {
            return nullptr;
        }

        if (SlotType == EGridInventoryUiSlotType::Inventory)
        {
            FGridCharacterInventoryState& CharacterState =
                InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
            if (!CharacterState.InventorySlots.IsValidIndex (SlotIndex) ||
                CharacterState.InventorySlots[SlotIndex].IsEmpty ())
            {
                return nullptr;
            }
            return &CharacterState.InventorySlots[SlotIndex].Item;
        }

        if (const EGridEquipmentSlot EquipmentSlot = ResolveEquipmentSlot (SlotType);
            EquipmentSlot != EGridEquipmentSlot::None)
        {
            FGridCharacterEquipmentState& EquipmentState =
                InventoryComponent->PartyInventoryState.ActiveEquipment[CharacterIndex];
            FGridItemInstance* Item = EquipmentState.GetMutableSlot (EquipmentSlot);
            return Item && Item->IsValid () ? Item : nullptr;
        }

        return nullptr;
    };

    auto CanPlaceItemInSlot = [&] (
        const FGridItemInstance& Item,
        EGridInventoryUiSlotType SlotType) -> bool
    {
        if (SlotType == EGridInventoryUiSlotType::Inventory)
        {
            return Item.IsValid ();
        }

        const EGridEquipmentSlot EquipmentSlot = ResolveEquipmentSlot (SlotType);
        return EquipmentSlot != EGridEquipmentSlot::None &&
            InventoryComponent->CanEquipItemToSlot (CharacterIndex, Item, EquipmentSlot);
    };

    auto PrepareItemForSlot = [&] (
        FGridItemInstance& Item,
        EGridInventoryUiSlotType SlotType)
    {
        if (SlotType == EGridInventoryUiSlotType::Inventory)
        {
            Item.OwnerType = EGridItemOwnerType::CharacterInventory;
            Item.OwnerGuid = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId;
            Item.OwnerCharacterIndex = CharacterIndex;
            Item.EquipmentSlot = EGridEquipmentSlot::None;
            return;
        }

        const EGridEquipmentSlot EquipmentSlot = ResolveEquipmentSlot (SlotType);
        Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
        Item.OwnerGuid = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId;
        Item.OwnerCharacterIndex = CharacterIndex;
        Item.EquipmentSlot = EquipmentSlot;
    };

    bool bSwapOccupiedSlotsAttempted = false;
    auto TrySwapOccupiedSlots = [&] () -> bool
    {
        if (bSplitStack ||
            SourceType == EGridInventoryUiSlotType::Cursor ||
            TargetType == EGridInventoryUiSlotType::Cursor)
        {
            return false;
        }

        FGridItemInstance* SourceItemPtr = GetMutableSlotItem (SourceType, SourceIndex);
        FGridItemInstance* TargetItemPtr = GetMutableSlotItem (TargetType, TargetIndex);
        if (!SourceItemPtr || !TargetItemPtr)
        {
            return false;
        }
        bSwapOccupiedSlotsAttempted = true;

        UE_LOG (LogTemp, Log,
            TEXT ("GridInventory SwapSlots Source=%s SourceIndex=%d Target=%s TargetIndex=%d"),
            GetGridInventoryUiSlotTypeName (SourceType),
            SourceIndex,
            GetGridInventoryUiSlotTypeName (TargetType),
            TargetIndex);

        FGridItemInstance SourceItem = *SourceItemPtr;
        FGridItemInstance TargetItem = *TargetItemPtr;
        if (!CanPlaceItemInSlot (SourceItem, TargetType))
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridInventory SwapSlots Failed Reason=IncompatibleSourceToTarget Item=%s"),
                *SourceItem.ItemDefinitionId.ToString ());
            return false;
        }

        if (!CanPlaceItemInSlot (TargetItem, SourceType))
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridInventory SwapSlots Failed Reason=IncompatibleTargetToSource Item=%s"),
                *TargetItem.ItemDefinitionId.ToString ());
            return false;
        }

        PrepareItemForSlot (SourceItem, TargetType);
        PrepareItemForSlot (TargetItem, SourceType);
        *SourceItemPtr = TargetItem;
        *TargetItemPtr = SourceItem;
        InventoryComponent->RecalculateCharacterWeight (CharacterIndex);
        if (SourceType == EGridInventoryUiSlotType::MainHand ||
            SourceType == EGridInventoryUiSlotType::OffHand ||
            TargetType == EGridInventoryUiSlotType::MainHand ||
            TargetType == EGridInventoryUiSlotType::OffHand)
        {
            OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment ();
        }

        UE_LOG (LogTemp, Log,
            TEXT ("GridInventory SwapSlots Success ItemA=%s ItemB=%s"),
            *SourceItem.ItemDefinitionId.ToString (),
            *TargetItem.ItemDefinitionId.ToString ());
        return true;
    };

    if (TrySwapOccupiedSlots ())
    {
        ValidateOwnership ();
        RefreshInventory ();
        return true;
    }
    if (bSwapOccupiedSlotsAttempted)
    {
        ValidateOwnership ();
        RefreshInventory ();
        return false;
    }

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

UGridInventorySlotWidget* UGridInventoryWidget::FindRegisteredSlotWidget (
    EGridInventoryUiSlotType SlotType,
    int32 SlotIndex) const
{
    switch (SlotType)
    {
    case EGridInventoryUiSlotType::Inventory:
        for (UGridInventorySlotWidget* SlotWidget : RegisteredInventorySlots)
        {
            if (SlotWidget && SlotWidget->InventorySlotIndex == SlotIndex)
            {
                return SlotWidget;
            }
        }
        return nullptr;
    case EGridInventoryUiSlotType::MainHand:
        return MainHandSlotWidget;
    case EGridInventoryUiSlotType::OffHand:
        return OffHandSlotWidget;
    case EGridInventoryUiSlotType::Cursor:
        return CursorSlotWidget;
    default:
        return nullptr;
    }
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
