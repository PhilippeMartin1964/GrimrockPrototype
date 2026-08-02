#include "UI/GridCombatActionPanelWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
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

    FText FormatActionPoints (int32 CurrentValue, int32 MaximumValue)
    {
        return FText::FromString (
            FString::Printf (
                TEXT ("PA %d / %d"),
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

    FGridPlayerCharacterTurnState TurnState;
    if (IsValid (TurnManagerComponent) &&
        TurnManagerComponent->GetPlayerCharacterTurnState (
            View.CharacterIndex,
            TurnState))
    {
        View.TurnState = TurnState.State;
        View.RemainingActionPoints = TurnState.RemainingActionPoints;
        View.MaximumActionPoints = TurnState.MaximumActionPoints;
        View.AttackActionPointCost = FMath::Clamp (
            TurnManagerComponent->PlayerAttackActionPointCost,
            1,
            6);
        View.bCanAct = TurnManagerComponent->CanCharacterAct (
            View.CharacterIndex);
        View.bCanPayAttackCost =
            TurnManagerComponent->CanCharacterSpendActionPoints (
                View.CharacterIndex,
                View.AttackActionPointCost);
    }

    RefreshBoundWidgets ();
}

bool UGridCombatActionPanelWidget::RequestAttackFromSlot (
    EGridEquipmentSlot EquipmentSlot)
{
    if (!IsValid (TurnManagerComponent) ||
        (EquipmentSlot != EGridEquipmentSlot::MainHand &&
            EquipmentSlot != EGridEquipmentSlot::OffHand))
    {
        return false;
    }

    FGridPlayerAttackRequest Request;
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    const bool bAccepted =
        TurnManagerComponent->RequestCharacterAttackFromSlot (
            ResolveCharacterIndex (),
            EquipmentSlot,
            Request,
            Result,
            RejectReason);
    RefreshFromSources ();
    return bAccepted;
}

FText UGridCombatActionPanelWidget::GetActionStateText () const
{
    switch (View.TurnState)
    {
    case EGridCombatantTurnState::Active:
        return FText::FromString (TEXT ("Active"));
    case EGridCombatantTurnState::Completed:
        return FText::FromString (TEXT ("Completed"));
    case EGridCombatantTurnState::Incapacitated:
        return FText::FromString (TEXT ("Incapacitated"));
    case EGridCombatantTurnState::Defeated:
        return FText::FromString (TEXT ("Defeated"));
    case EGridCombatantTurnState::Waiting:
    default:
        return FText::FromString (TEXT ("Waiting"));
    }
}

void UGridCombatActionPanelWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    if (Button_MainHand)
    {
        Button_MainHand->OnClicked.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleMainHandClicked);
    }
    if (Button_OffHand)
    {
        Button_OffHand->OnClicked.AddUniqueDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleOffHandClicked);
    }
    BindToSources ();
    RefreshFromSources ();
}

void UGridCombatActionPanelWidget::NativeDestruct ()
{
    if (Button_MainHand)
    {
        Button_MainHand->OnClicked.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleMainHandClicked);
    }
    if (Button_OffHand)
    {
        Button_OffHand->OnClicked.RemoveDynamic (
            this,
            &UGridCombatActionPanelWidget::HandleOffHandClicked);
    }
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
        TurnManagerComponent->OnPlayerCharacterTurnStateChanged
            .AddUniqueDynamic (
                this,
                &UGridCombatActionPanelWidget::HandlePlayerCharacterTurnStateChanged);
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
        TurnManagerComponent->OnPlayerCharacterTurnStateChanged
            .RemoveDynamic (
                this,
                &UGridCombatActionPanelWidget::HandlePlayerCharacterTurnStateChanged);
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
    if (Text_ActionPoints)
    {
        Text_ActionPoints->SetText (
            FormatActionPoints (
                View.RemainingActionPoints,
                View.MaximumActionPoints));
    }

    ApplySlotVisual (
        View.MainHand,
        Button_MainHand,
        Image_MainHandIcon,
        Text_MainHandQuantity);
    ApplySlotVisual (
        View.OffHand,
        Button_OffHand,
        Image_OffHandIcon,
        Text_OffHandQuantity);

    if (Text_ActionState)
    {
        Text_ActionState->SetText (GetActionStateText ());
    }
    if (Border_ActionState)
    {
        FLinearColor StateColor = WaitingColor;
        switch (View.TurnState)
        {
        case EGridCombatantTurnState::Active:
            StateColor = ReadyColor;
            break;
        case EGridCombatantTurnState::Completed:
            StateColor = AlreadyActedColor;
            break;
        case EGridCombatantTurnState::Incapacitated:
            StateColor = IncapacitatedColor;
            break;
        case EGridCombatantTurnState::Defeated:
            StateColor = DefeatedColor;
            break;
        case EGridCombatantTurnState::Waiting:
        default:
            break;
        }
        Border_ActionState->SetBrushColor (StateColor);
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
        Result.bCanAttack = true;
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
        Result.bCanAttack =
            Definition->CanProvideAttackFromSlot (EquipmentSlot);
    }
    else
    {
        Result.bShowQuantity = Result.Quantity > 1;
    }

    return Result;
}

void UGridCombatActionPanelWidget::ApplySlotVisual (
    const FGridCombatActionSlotView& SlotView,
    UButton* ButtonWidget,
    UImage* IconWidget,
    UTextBlock* QuantityWidget) const
{
    if (ButtonWidget)
    {
        ButtonWidget->SetIsEnabled (
            View.bCanPayAttackCost && SlotView.bCanAttack);
    }

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

void UGridCombatActionPanelWidget::HandleMainHandClicked ()
{
    RequestAttackFromSlot (EGridEquipmentSlot::MainHand);
}

void UGridCombatActionPanelWidget::HandleOffHandClicked ()
{
    RequestAttackFromSlot (EGridEquipmentSlot::OffHand);
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

void UGridCombatActionPanelWidget::HandlePlayerCharacterTurnStateChanged (
    FGridPlayerCharacterTurnState TurnState)
{
    if (TurnState.CharacterIndex == ResolveCharacterIndex ())
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
