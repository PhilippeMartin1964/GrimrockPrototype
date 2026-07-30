#include "UI/GridCombatActionPanelWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
    FText FormatCurrentAndMaximum (int32 CurrentValue, int32 MaximumValue)
    {
        return FText::FromString (
            FString::Printf (
                TEXT ("%d / %d"),
                FMath::Max (0, CurrentValue),
                FMath::Max (0, MaximumValue)));
    }

    FText ResolveCharacterName (
        const FGridInventoryCharacterSummary& Summary)
    {
        return Summary.DisplayName.IsEmpty ()
            ? FText::FromString (
                FString::Printf (
                    TEXT ("Hero_%02d"),
                    Summary.CharacterIndex + 1))
            : Summary.DisplayName;
    }
}

void UGridCombatActionPanelWidget::InitializeCombatActionPanel (
    AGrimrockPartyPawn* InPartyPawn,
    int32 InCharacterIndex,
    UGridTurnManagerComponent* InTurnManager)
{
    UnbindFromSources ();

    PartyPawn = InPartyPawn;
    InventoryComponent = IsValid (PartyPawn)
        ? PartyPawn->PartyInventoryComponent
        : nullptr;
    TurnManagerComponent = InTurnManager;
    if (!IsValid (TurnManagerComponent) &&
        IsValid (PartyPawn) &&
        IsValid (PartyPawn->LevelRuntimeActor))
    {
        TurnManagerComponent =
            PartyPawn->LevelRuntimeActor
                ->FindComponentByClass<UGridTurnManagerComponent> ();
    }

    ConfiguredCharacterIndex = InCharacterIndex;
    bFollowSelectedCharacter = InCharacterIndex == INDEX_NONE;
    BindToSources ();
    RefreshFromSources ();
}

void UGridCombatActionPanelWidget::SetCharacterIndex (
    int32 InCharacterIndex)
{
    ConfiguredCharacterIndex = InCharacterIndex;
    bFollowSelectedCharacter = InCharacterIndex == INDEX_NONE;
    RefreshFromSources ();
}

void UGridCombatActionPanelWidget::RefreshFromSources ()
{
    View = FGridCombatActionPanelView ();
    View.CharacterIndex = ResolveCharacterIndex ();
    View.bCombatActive =
        IsValid (TurnManagerComponent) &&
        TurnManagerComponent->bCombatActive;

    FGridInventoryCharacterSummary Summary;
    if (!IsValid (InventoryComponent) ||
        !InventoryComponent->GetCharacterSummary (
            View.CharacterIndex,
            Summary))
    {
        RefreshBoundWidgets ();
        return;
    }

    View.bHasValidCharacter = true;
    View.DisplayName = ResolveCharacterName (Summary);
    View.Portrait = Summary.Portrait;
    View.CurrentHealth = Summary.DerivedStats.CurrentHealth;
    View.MaxHealth = Summary.DerivedStats.MaxHealth;
    View.CurrentMana = Summary.DerivedStats.CurrentMana;
    View.MaxMana = Summary.DerivedStats.MaxMana;
    View.MainHand = BuildSlotView (EGridEquipmentSlot::MainHand);
    View.OffHand = BuildSlotView (EGridEquipmentSlot::OffHand);

    const bool bAlreadyActed =
        IsValid (TurnManagerComponent) &&
        TurnManagerComponent->HasCharacterCommittedAttackThisPhase (
            View.CharacterIndex);
    View.ActionState = bAlreadyActed
        ? EGridCombatActionPanelState::AlreadyActed
        : EGridCombatActionPanelState::Ready;
    View.bCanAct =
        IsValid (TurnManagerComponent) &&
        TurnManagerComponent->CanCharacterAct (View.CharacterIndex);

    RefreshBoundWidgets ();
}

FText UGridCombatActionPanelWidget::GetActionStateText () const
{
    return View.ActionState ==
        EGridCombatActionPanelState::AlreadyActed
            ? FText::FromString (TEXT ("AlreadyActed"))
            : FText::FromString (TEXT ("Ready"));
}

void UGridCombatActionPanelWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    BindToSources ();
    RefreshFromSources ();
}

void UGridCombatActionPanelWidget::NativeDestruct ()
{
    UnbindFromSources ();
    Super::NativeDestruct ();
}

int32 UGridCombatActionPanelWidget::ResolveCharacterIndex () const
{
    if (!bFollowSelectedCharacter)
    {
        return ConfiguredCharacterIndex;
    }

    return IsValid (InventoryComponent)
        ? InventoryComponent->GetSelectedCharacterIndex ()
        : INDEX_NONE;
}

void UGridCombatActionPanelWidget::BindToSources ()
{
    if (bSourcesBound)
    {
        return;
    }

    if (IsValid (InventoryComponent))
    {
        InventoryComponent->OnPartyInventoryChanged.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleInventoryChanged);
    }

    if (IsValid (TurnManagerComponent))
    {
        TurnManagerComponent->OnPhaseChanged.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandlePhaseChanged);
        TurnManagerComponent->OnRoundStarted.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleRoundStarted);
        TurnManagerComponent->OnPlayerAttackRequested.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandlePlayerAttackRequested);
        TurnManagerComponent->OnPlayerAttackResolved.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandlePlayerAttackResolved);
        TurnManagerComponent->OnAttackResolved.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleMonsterAttackResolved);
        TurnManagerComponent->OnCombatEnded.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleCombatEnded);
    }

    bSourcesBound =
        IsValid (InventoryComponent) ||
        IsValid (TurnManagerComponent);
}

void UGridCombatActionPanelWidget::UnbindFromSources ()
{
    if (IsValid (InventoryComponent))
    {
        InventoryComponent->OnPartyInventoryChanged.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleInventoryChanged);
    }

    if (IsValid (TurnManagerComponent))
    {
        TurnManagerComponent->OnPhaseChanged.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandlePhaseChanged);
        TurnManagerComponent->OnRoundStarted.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleRoundStarted);
        TurnManagerComponent->OnPlayerAttackRequested.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandlePlayerAttackRequested);
        TurnManagerComponent->OnPlayerAttackResolved.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandlePlayerAttackResolved);
        TurnManagerComponent->OnAttackResolved.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleMonsterAttackResolved);
        TurnManagerComponent->OnCombatEnded.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleCombatEnded);
    }

    bSourcesBound = false;
}

void UGridCombatActionPanelWidget::RefreshBoundWidgets ()
{
    if (bCollapseOutsideCombat)
    {
        SetVisibility (
            View.bCombatActive && View.bHasValidCharacter
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed);
    }
    else
    {
        SetVisibility (
            View.bHasValidCharacter
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed);
    }

    if (Image_Portrait)
    {
        if (View.Portrait.IsNull ())
        {
            Image_Portrait->SetBrushFromTexture (nullptr);
            Image_Portrait->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Image_Portrait->SetBrushFromSoftTexture (
                View.Portrait,
                false);
            Image_Portrait->SetVisibility (
                ESlateVisibility::HitTestInvisible);
        }
    }

    if (Text_Name)
    {
        Text_Name->SetText (View.DisplayName);
    }
    if (Text_Health)
    {
        Text_Health->SetText (
            FormatCurrentAndMaximum (
                View.CurrentHealth,
                View.MaxHealth));
    }
    if (Text_Mana)
    {
        Text_Mana->SetText (
            FormatCurrentAndMaximum (
                View.CurrentMana,
                View.MaxMana));
    }

    ApplySlotVisual (
        View.MainHand,
        Image_MainHandIcon,
        Text_MainHandQuantity);
    ApplySlotVisual (
        View.OffHand,
        Image_OffHandIcon,
        Text_OffHandQuantity);

    if (Text_ActionState)
    {
        Text_ActionState->SetText (GetActionStateText ());
    }
    if (Border_ActionState)
    {
        Border_ActionState->SetBrushColor (
            View.ActionState ==
                EGridCombatActionPanelState::AlreadyActed
                    ? AlreadyActedColor
                    : ReadyColor);
    }
    if (Panel_DisabledOverlay)
    {
        Panel_DisabledOverlay->SetVisibility (
            View.bCanAct
                ? ESlateVisibility::Collapsed
                : ESlateVisibility::HitTestInvisible);
    }

    SetIsEnabled (View.bCanAct);
    SetRenderOpacity (View.bCanAct ? 1.0f : DisabledOpacity);
}

FGridCombatActionSlotView
UGridCombatActionPanelWidget::BuildSlotView (
    EGridEquipmentSlot EquipmentSlot) const
{
    FGridCombatActionSlotView Result;
    FGridItemInstance EquippedItem;
    if (!IsValid (InventoryComponent) ||
        !InventoryComponent->GetEquippedItem (
            View.CharacterIndex,
            EquipmentSlot,
            EquippedItem))
    {
        return Result;
    }

    Result.bOccupied = true;
    Result.ItemDefinitionId = EquippedItem.ItemDefinitionId;
    Result.DisplayName = EquippedItem.DisplayName;
    Result.Quantity = FMath::Max (1, EquippedItem.Quantity);

    const UGridItemDefinitionAsset* Definition =
        InventoryComponent->FindItemDefinition (
            EquippedItem.ItemDefinitionId);
    if (Definition)
    {
        Result.Icon = Definition->Icon;
        if (Result.DisplayName.IsEmpty ())
        {
            Result.DisplayName = Definition->DisplayName;
        }
        Result.bShowQuantity =
            Definition->bStackable &&
            Result.Quantity > 0;
    }
    else
    {
        Result.bShowQuantity = Result.Quantity > 1;
    }

    return Result;
}

void UGridCombatActionPanelWidget::ApplySlotVisual (
    const FGridCombatActionSlotView& SlotView,
    UImage* IconWidget,
    UTextBlock* QuantityWidget) const
{
    if (IconWidget)
    {
        if (!SlotView.bOccupied || SlotView.Icon.IsNull ())
        {
            IconWidget->SetBrushFromTexture (nullptr);
            IconWidget->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            IconWidget->SetBrushFromSoftTexture (
                SlotView.Icon,
                false);
            IconWidget->SetVisibility (
                ESlateVisibility::HitTestInvisible);
        }
    }

    if (QuantityWidget)
    {
        QuantityWidget->SetText (
            FText::AsNumber (SlotView.Quantity));
        QuantityWidget->SetVisibility (
            SlotView.bOccupied && SlotView.bShowQuantity
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
    }
}

void UGridCombatActionPanelWidget::HandleInventoryChanged (
    int32 ChangedCharacterIndex)
{
    if (bFollowSelectedCharacter ||
        ChangedCharacterIndex == INDEX_NONE ||
        ChangedCharacterIndex == ResolveCharacterIndex ())
    {
        RefreshFromSources ();
    }
}

void UGridCombatActionPanelWidget::HandlePhaseChanged (
    EGridCombatPhase NewPhase)
{
    (void)NewPhase;
    RefreshFromSources ();
}

void UGridCombatActionPanelWidget::HandleRoundStarted (
    int32 RoundNumber)
{
    (void)RoundNumber;
    RefreshFromSources ();
}

void UGridCombatActionPanelWidget::HandlePlayerAttackRequested (
    FGridPlayerAttackRequest Request)
{
    if (Request.AttackerCharacterIndex == ResolveCharacterIndex ())
    {
        RefreshFromSources ();
    }
}

void UGridCombatActionPanelWidget::HandlePlayerAttackResolved (
    FGridPlayerAttackRequest Request,
    AGridMonsterActor* TargetMonster,
    FGridAttackResult Result)
{
    (void)TargetMonster;
    (void)Result;
    if (Request.AttackerCharacterIndex == ResolveCharacterIndex ())
    {
        RefreshFromSources ();
    }
}

void UGridCombatActionPanelWidget::HandleMonsterAttackResolved (
    AGridMonsterActor* Monster,
    int32 TargetCharacterIndex,
    FGridAttackResult Result)
{
    (void)Monster;
    (void)Result;
    if (TargetCharacterIndex == ResolveCharacterIndex ())
    {
        RefreshFromSources ();
    }
}

void UGridCombatActionPanelWidget::HandleCombatEnded (
    EGridCombatPhase ResultPhase)
{
    (void)ResultPhase;
    RefreshFromSources ();
}
