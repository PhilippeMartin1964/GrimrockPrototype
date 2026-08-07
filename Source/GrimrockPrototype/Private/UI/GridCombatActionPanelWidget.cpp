#include "UI/GridCombatActionPanelWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
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
    RefreshFromSources ();
}

void UGridCombatActionPanelWidget::RefreshFromSources ()
{
    View = FGridCombatActionPanelView ();
    View.CharacterIndex = ConfiguredCharacterIndex;
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

    FGridPlayerCharacterTurnState TurnState;
    if (IsValid (TurnManagerComponent) &&
        TurnManagerComponent->GetPlayerCharacterTurnState (
            View.CharacterIndex,
            TurnState))
    {
        View.TurnState = TurnState.State;
        View.RemainingActionPoints = TurnState.RemainingActionPoints;
        View.MaximumActionPoints = TurnState.MaximumActionPoints;
        View.bCanAct = TurnManagerComponent->CanCharacterAct (
            View.CharacterIndex);
    }

    RefreshBoundWidgets ();
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
