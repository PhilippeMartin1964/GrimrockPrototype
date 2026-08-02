#include "UI/GridCombatHudWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridCombatActionPanelWidget.h"

namespace
{
    FText FormatActionCost (const FGridAvailableCombatAction& Action)
    {
        FString Result = FString::Printf (
            TEXT ("%d PA"),
            Action.CurrentActionPointCost);
        if (Action.CurrentManaCost > 0)
        {
            Result += FString::Printf (
                TEXT (" | %d mana"),
                Action.CurrentManaCost);
        }
        if (Action.CurrentSourceItemQuantityCost > 0)
        {
            Result += FString::Printf (
                TEXT (" | x%d"),
                Action.CurrentSourceItemQuantityCost);
        }
        return FText::FromString (Result);
    }

    FText FormatCurrentAndMaximum (
        const TCHAR* Prefix,
        int32 Current,
        int32 Maximum)
    {
        return FText::FromString (FString::Printf (
            TEXT ("%s %d / %d"),
            Prefix,
            FMath::Max (0, Current),
            FMath::Max (0, Maximum)));
    }
}

void FGridCombatHudViewModelBuilder::BuildPartyMembers (
    int32 ActiveCharacterCount,
    const TArray<FGridPlayerCharacterTurnState>& TurnStates,
    TArray<FGridCombatHudPartyMemberView>& OutMembers)
{
    OutMembers.Reset (PartyPanelCount);
    for (int32 CharacterIndex = 0;
        CharacterIndex < PartyPanelCount;
        ++CharacterIndex)
    {
        FGridCombatHudPartyMemberView& Member =
            OutMembers.AddDefaulted_GetRef ();
        Member.CharacterIndex = CharacterIndex;
        Member.bPresent = CharacterIndex < ActiveCharacterCount;

        const FGridPlayerCharacterTurnState* TurnState =
            TurnStates.FindByPredicate (
                [CharacterIndex] (
                    const FGridPlayerCharacterTurnState& Candidate)
                {
                    return Candidate.CharacterIndex == CharacterIndex;
                });
        if (TurnState)
        {
            Member.TurnState = TurnState->State;
            Member.RemainingActionPoints =
                TurnState->RemainingActionPoints;
            Member.MaximumActionPoints =
                TurnState->MaximumActionPoints;
            Member.bActive =
                TurnState->State == EGridCombatantTurnState::Active;
        }
    }
}

void FGridCombatHudViewModelBuilder::BuildActions (
    const TArray<FGridAvailableCombatAction>& AvailableActions,
    TArray<FGridCombatHudActionView>& OutActions)
{
    OutActions.Reset (AvailableActions.Num ());
    for (const FGridAvailableCombatAction& AvailableAction : AvailableActions)
    {
        FGridCombatHudActionView& ActionView =
            OutActions.AddDefaulted_GetRef ();
        ActionView.Action = AvailableAction;
        ActionView.CostText = FormatActionCost (AvailableAction);
        ActionView.DisabledReason = AvailableAction.bEnabled
            ? FText::GetEmpty ()
            : AvailableAction.DisabledReason;
    }
}

FGridCombatHudMobilityView FGridCombatHudViewModelBuilder::BuildMobility (
    const FGridPartyMobilityState& MobilityState)
{
    FGridCombatHudMobilityView View;
    View.RemainingMobilityActionPoints =
        MobilityState.RemainingMobilityActionPoints;
    View.MaximumMobilityActionPoints =
        MobilityState.MaximumMobilityActionPoints;
    View.DisplayText = FormatCurrentAndMaximum (
        TEXT ("PAM"),
        View.RemainingMobilityActionPoints,
        View.MaximumMobilityActionPoints);
    return View;
}

void FGridCombatHudViewModelBuilder::BuildInitiative (
    const TArray<FGridCombatantInitiativeEntry>& UpcomingOrder,
    TArray<FGridCombatHudInitiativeView>& OutInitiative,
    int32& OutOverflowCount,
    int32 MaximumVisibleEntries)
{
    const int32 VisibleCount = FMath::Min (
        UpcomingOrder.Num (),
        FMath::Max (0, MaximumVisibleEntries));
    OutInitiative.Reset (VisibleCount);
    for (int32 Index = 0; Index < VisibleCount; ++Index)
    {
        FGridCombatHudInitiativeView& Entry =
            OutInitiative.AddDefaulted_GetRef ();
        Entry.Combatant = UpcomingOrder[Index];
        Entry.bActive = Index == 0 &&
            UpcomingOrder[Index].State == EGridCombatantTurnState::Active;
    }
    OutOverflowCount = FMath::Max (
        0,
        UpcomingOrder.Num () - VisibleCount);
}

void UGridCombatHudActionWidget::InitializeAction (
    UGridCombatHudWidget* InOwnerHud,
    const FGridCombatHudActionView& InView)
{
    OwnerHud = InOwnerHud;
    View = InView;
    RefreshWidgets ();
}

void UGridCombatHudActionWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    if (Button_Action)
    {
        Button_Action->OnClicked.AddUniqueDynamic (
            this,
            &UGridCombatHudActionWidget::HandleClicked);
    }
    RefreshWidgets ();
}

void UGridCombatHudActionWidget::NativeDestruct ()
{
    if (Button_Action)
    {
        Button_Action->OnClicked.RemoveDynamic (
            this,
            &UGridCombatHudActionWidget::HandleClicked);
    }
    OwnerHud = nullptr;
    Super::NativeDestruct ();
}

void UGridCombatHudActionWidget::RefreshWidgets ()
{
    if (Button_Action)
    {
        Button_Action->SetIsEnabled (View.Action.bEnabled);
        Button_Action->SetToolTipText (
            View.Action.bEnabled
                ? View.Action.Definition.Description
                : View.DisabledReason);
    }
    if (Image_ActionIcon)
    {
        if (View.Action.Definition.Icon.IsNull ())
        {
            Image_ActionIcon->SetBrushFromTexture (nullptr);
        }
        else
        {
            Image_ActionIcon->SetBrushFromSoftTexture (
                View.Action.Definition.Icon,
                false);
        }
    }
    if (Text_ActionName)
    {
        Text_ActionName->SetText (
            View.Action.Definition.DisplayName.IsEmpty ()
                ? FText::FromName (View.Action.Definition.ActionId)
                : View.Action.Definition.DisplayName);
    }
    if (Text_ActionCost)
    {
        Text_ActionCost->SetText (View.CostText);
    }
    if (Text_DisabledReason)
    {
        Text_DisabledReason->SetText (View.DisabledReason);
        Text_DisabledReason->SetVisibility (
            View.Action.bEnabled
                ? ESlateVisibility::Collapsed
                : ESlateVisibility::HitTestInvisible);
    }
    SetRenderOpacity (View.Action.bEnabled ? 1.0f : 0.45f);
}

void UGridCombatHudActionWidget::HandleClicked ()
{
    if (!IsValid (OwnerHud))
    {
        return;
    }
    FGridCombatActionRequestResult Result;
    OwnerHud->RequestCombatAction (View, Result);
}

void UGridCombatHudInitiativeSlotWidget::InitializeInitiativeSlot (
    const FGridCombatHudInitiativeView& InView)
{
    View = InView;
    RefreshWidgets ();
}

void UGridCombatHudInitiativeSlotWidget::RefreshWidgets ()
{
    if (Image_Portrait)
    {
        if (View.Combatant.Portrait.IsNull ())
        {
            Image_Portrait->SetBrushFromTexture (nullptr);
        }
        else
        {
            Image_Portrait->SetBrushFromSoftTexture (
                View.Combatant.Portrait,
                false);
        }
    }
    if (Text_Name)
    {
        Text_Name->SetText (View.Combatant.DisplayName);
    }
    if (Text_Health)
    {
        Text_Health->SetText (FormatCurrentAndMaximum (
            TEXT ("PV"),
            View.Combatant.CurrentHealth,
            View.Combatant.MaximumHealth));
    }
    if (Text_State)
    {
        Text_State->SetText (
            UEnum::GetDisplayValueAsText (View.Combatant.State));
    }
    if (Text_Side)
    {
        Text_Side->SetText (
            UEnum::GetDisplayValueAsText (View.Combatant.Side));
    }
    if (Border_Active)
    {
        Border_Active->SetVisibility (
            View.bActive
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
    }
    SetRenderScale (FVector2D (
        View.bActive ? FMath::Max (1.0f, ActiveScale) : 1.0f));
    SetRenderTransformPivot (FVector2D (0.5f, 0.5f));
}

void UGridCombatHudWidget::InitializeCombatHud (
    AGrimrockPartyPawn* InPartyPawn,
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
        TurnManagerComponent = PartyPawn->LevelRuntimeActor
            ->FindComponentByClass<UGridTurnManagerComponent> ();
    }
    BindToSources ();
    EnsurePartyMemberPanels ();
    RefreshFromSources ();
}

void UGridCombatHudWidget::RefreshFromSources ()
{
    View = FGridCombatHudView ();
    View.bCombatActive =
        IsValid (TurnManagerComponent) &&
        TurnManagerComponent->bCombatActive;

    TArray<FGridPlayerCharacterTurnState> TurnStates;
    const int32 ActiveCharacterCount = IsValid (InventoryComponent)
        ? InventoryComponent->GetActiveCharacterCount ()
        : 0;
    for (int32 CharacterIndex = 0;
        CharacterIndex < FGridCombatHudViewModelBuilder::PartyPanelCount;
        ++CharacterIndex)
    {
        FGridPlayerCharacterTurnState TurnState;
        if (IsValid (TurnManagerComponent) &&
            TurnManagerComponent->GetPlayerCharacterTurnState (
                CharacterIndex,
                TurnState))
        {
            TurnStates.Add (TurnState);
        }
    }
    FGridCombatHudViewModelBuilder::BuildPartyMembers (
        ActiveCharacterCount,
        TurnStates,
        View.PartyMembers);

    for (FGridCombatHudPartyMemberView& Member : View.PartyMembers)
    {
        FGridInventoryCharacterSummary Summary;
        if (!Member.bPresent ||
            !IsValid (InventoryComponent) ||
            !InventoryComponent->GetCharacterSummary (
                Member.CharacterIndex,
                Summary))
        {
            continue;
        }
        Member.DisplayName = Summary.DisplayName;
        Member.Portrait = Summary.Portrait;
        Member.CurrentHealth = Summary.DerivedStats.CurrentHealth;
        Member.MaximumHealth = Summary.DerivedStats.MaxHealth;
        Member.CurrentMana = Summary.DerivedStats.CurrentMana;
        Member.MaximumMana = Summary.DerivedStats.MaxMana;
    }

    FGridCombatantInitiativeEntry ActiveCombatant;
    if (IsValid (TurnManagerComponent) &&
        TurnManagerComponent->GetActiveCombatant (ActiveCombatant) &&
        ActiveCombatant.Side == EGridCombatantSide::Party)
    {
        View.ActiveCharacterIndex = ActiveCombatant.CharacterIndex;
        if (View.PartyMembers.IsValidIndex (
            View.ActiveCharacterIndex))
        {
            View.PartyMembers[View.ActiveCharacterIndex].bActive = true;
        }

        TArray<FGridAvailableCombatAction> AvailableActions;
        TurnManagerComponent->GetAvailableCombatActions (
            View.ActiveCharacterIndex,
            AvailableActions);
        FGridCombatHudViewModelBuilder::BuildActions (
            AvailableActions,
            View.Actions);
    }

    View.Mobility = FGridCombatHudViewModelBuilder::BuildMobility (
        IsValid (TurnManagerComponent)
            ? TurnManagerComponent->GetPartyMobilityState ()
            : FGridPartyMobilityState ());

    TArray<FGridCombatantInitiativeEntry> UpcomingOrder;
    if (IsValid (TurnManagerComponent))
    {
        TurnManagerComponent->GetUpcomingInitiativeOrder (UpcomingOrder);
    }
    FGridCombatHudViewModelBuilder::BuildInitiative (
        UpcomingOrder,
        View.Initiative,
        View.InitiativeOverflowCount);

    View.bCanEndTurn =
        IsValid (TurnManagerComponent) &&
        TurnManagerComponent->CanEndActivePlayerTurn ();
    if (!View.bCanEndTurn)
    {
        if (!View.bCombatActive)
        {
            View.EndTurnDisabledReason =
                FText::FromString (TEXT ("Combat inactif"));
        }
        else if (!IsValid (TurnManagerComponent) ||
            !TurnManagerComponent->GetActiveCombatant (ActiveCombatant) ||
            ActiveCombatant.Side != EGridCombatantSide::Party)
        {
            View.EndTurnDisabledReason =
                FText::FromString (TEXT ("Tour ennemi"));
        }
        else if (TurnManagerComponent->IsPartyMotionInProgress ())
        {
            View.EndTurnDisabledReason =
                FText::FromString (TEXT ("Déplacement en cours"));
        }
        else
        {
            View.EndTurnDisabledReason =
                FText::FromString (TEXT ("Résolution en cours"));
        }
    }

    for (UGridCombatActionPanelWidget* Panel : PartyMemberPanels)
    {
        if (IsValid (Panel))
        {
            Panel->RefreshFromSources ();
        }
    }
    RebuildActionWidgets ();
    RebuildInitiativeWidgets ();
    RefreshBoundWidgets ();
}

bool UGridCombatHudWidget::RequestCombatAction (
    const FGridCombatHudActionView& ActionView,
    FGridCombatActionRequestResult& OutResult)
{
    OutResult = FGridCombatActionRequestResult ();
    if (!IsValid (TurnManagerComponent))
    {
        return false;
    }

    const FGridAvailableCombatAction& Action = ActionView.Action;
    const bool bAccepted =
        TurnManagerComponent->RequestCharacterCombatAction (
            Action.CharacterIndex,
            Action.Definition.ActionId,
            Action.Definition.SourcePolicy,
            Action.SourceDefinitionId,
            Action.SourceEquipmentSlot,
            OutResult);
    RefreshFromSources ();
    return bAccepted;
}

bool UGridCombatHudWidget::RequestEndTurn ()
{
    if (!IsValid (TurnManagerComponent))
    {
        return false;
    }
    const bool bEnded = TurnManagerComponent->EndActivePlayerTurn ();
    RefreshFromSources ();
    return bEnded;
}

void UGridCombatHudWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    if (Button_EndTurn)
    {
        Button_EndTurn->OnClicked.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleEndTurnClicked);
    }
    BindToSources ();
    EnsurePartyMemberPanels ();
    RefreshFromSources ();
}

void UGridCombatHudWidget::NativeDestruct ()
{
    if (Button_EndTurn)
    {
        Button_EndTurn->OnClicked.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleEndTurnClicked);
    }
    UnbindFromSources ();
    PartyMemberPanels.Reset ();
    Super::NativeDestruct ();
}

void UGridCombatHudWidget::BindToSources ()
{
    if (bSourcesBound)
    {
        return;
    }
    if (IsValid (InventoryComponent))
    {
        InventoryComponent->OnPartyInventoryChanged.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleInventoryChanged);
    }
    if (IsValid (TurnManagerComponent))
    {
        TurnManagerComponent->OnPhaseChanged.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandlePhaseChanged);
        TurnManagerComponent->OnRoundStarted.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleRoundStarted);
        TurnManagerComponent->OnPlayerCharacterTurnStateChanged
            .AddUniqueDynamic (
                this,
                &UGridCombatHudWidget::HandlePlayerTurnStateChanged);
        TurnManagerComponent->OnPartyMobilityStateChanged.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandlePartyMobilityChanged);
        TurnManagerComponent->OnTurnOrderChanged.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleTurnOrderChanged);
        TurnManagerComponent->OnActiveCombatantChanged.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleActiveCombatantChanged);
        TurnManagerComponent->OnCombatantStateChanged.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleCombatantStateChanged);
        TurnManagerComponent->OnPlayerAttackResolved.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandlePlayerAttackResolved);
        TurnManagerComponent->OnPlayerAttackRejected.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandlePlayerAttackRejected);
        TurnManagerComponent->OnActionStarted.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleActionStarted);
        TurnManagerComponent->OnActionCompleted.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleActionCompleted);
        TurnManagerComponent->OnCombatEnded.AddUniqueDynamic (
            this,
            &UGridCombatHudWidget::HandleCombatEnded);
    }
    bSourcesBound =
        IsValid (InventoryComponent) ||
        IsValid (TurnManagerComponent);
}

void UGridCombatHudWidget::UnbindFromSources ()
{
    if (IsValid (InventoryComponent))
    {
        InventoryComponent->OnPartyInventoryChanged.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleInventoryChanged);
    }
    if (IsValid (TurnManagerComponent))
    {
        TurnManagerComponent->OnPhaseChanged.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandlePhaseChanged);
        TurnManagerComponent->OnRoundStarted.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleRoundStarted);
        TurnManagerComponent->OnPlayerCharacterTurnStateChanged
            .RemoveDynamic (
                this,
                &UGridCombatHudWidget::HandlePlayerTurnStateChanged);
        TurnManagerComponent->OnPartyMobilityStateChanged.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandlePartyMobilityChanged);
        TurnManagerComponent->OnTurnOrderChanged.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleTurnOrderChanged);
        TurnManagerComponent->OnActiveCombatantChanged.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleActiveCombatantChanged);
        TurnManagerComponent->OnCombatantStateChanged.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleCombatantStateChanged);
        TurnManagerComponent->OnPlayerAttackResolved.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandlePlayerAttackResolved);
        TurnManagerComponent->OnPlayerAttackRejected.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandlePlayerAttackRejected);
        TurnManagerComponent->OnActionStarted.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleActionStarted);
        TurnManagerComponent->OnActionCompleted.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleActionCompleted);
        TurnManagerComponent->OnCombatEnded.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleCombatEnded);
    }
    bSourcesBound = false;
}

void UGridCombatHudWidget::EnsurePartyMemberPanels ()
{
    if (!Panel_PartyMembers || !PartyMemberPanelWidgetClass)
    {
        return;
    }
    if (PartyMemberPanels.Num () ==
            FGridCombatHudViewModelBuilder::PartyPanelCount &&
        Panel_PartyMembers->GetChildrenCount () ==
            FGridCombatHudViewModelBuilder::PartyPanelCount)
    {
        return;
    }

    Panel_PartyMembers->ClearChildren ();
    PartyMemberPanels.Reset (
        FGridCombatHudViewModelBuilder::PartyPanelCount);
    for (int32 CharacterIndex = 0;
        CharacterIndex < FGridCombatHudViewModelBuilder::PartyPanelCount;
        ++CharacterIndex)
    {
        UGridCombatActionPanelWidget* Panel =
            CreateWidget<UGridCombatActionPanelWidget> (
                this,
                PartyMemberPanelWidgetClass);
        if (!Panel)
        {
            continue;
        }
        Panel->bShowHandActionButtons = false;
        Panel->InitializeCombatActionPanel (
            PartyPawn,
            CharacterIndex,
            TurnManagerComponent);
        Panel_PartyMembers->AddChild (Panel);
        PartyMemberPanels.Add (Panel);
    }
}

void UGridCombatHudWidget::RebuildActionWidgets ()
{
    if (!Panel_Actions)
    {
        return;
    }
    Panel_Actions->ClearChildren ();
    if (!ActionWidgetClass)
    {
        return;
    }
    for (const FGridCombatHudActionView& ActionView : View.Actions)
    {
        UGridCombatHudActionWidget* ActionWidget =
            CreateWidget<UGridCombatHudActionWidget> (
                this,
                ActionWidgetClass);
        if (ActionWidget)
        {
            ActionWidget->InitializeAction (this, ActionView);
            Panel_Actions->AddChild (ActionWidget);
        }
    }
}

void UGridCombatHudWidget::RebuildInitiativeWidgets ()
{
    if (!Panel_Initiative)
    {
        return;
    }
    Panel_Initiative->ClearChildren ();
    if (!InitiativeSlotWidgetClass)
    {
        return;
    }
    for (const FGridCombatHudInitiativeView& InitiativeView : View.Initiative)
    {
        UGridCombatHudInitiativeSlotWidget* InitiativeWidget =
            CreateWidget<UGridCombatHudInitiativeSlotWidget> (
                this,
                InitiativeSlotWidgetClass);
        if (InitiativeWidget)
        {
            InitiativeWidget->InitializeInitiativeSlot (InitiativeView);
            Panel_Initiative->AddChild (InitiativeWidget);
        }
    }
}

void UGridCombatHudWidget::RefreshBoundWidgets ()
{
    if (Panel_CombatHud)
    {
        Panel_CombatHud->SetVisibility (
            View.bCombatActive
                ? ESlateVisibility::SelfHitTestInvisible
                : ESlateVisibility::Collapsed);
    }
    if (Text_MobilityActionPoints)
    {
        Text_MobilityActionPoints->SetText (View.Mobility.DisplayText);
    }
    if (Text_InitiativeOverflow)
    {
        Text_InitiativeOverflow->SetText (
            View.InitiativeOverflowCount > 0
                ? FText::FromString (FString::Printf (
                    TEXT ("+ %d"),
                    View.InitiativeOverflowCount))
                : FText::GetEmpty ());
        Text_InitiativeOverflow->SetVisibility (
            View.InitiativeOverflowCount > 0
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
    }
    if (Button_EndTurn)
    {
        Button_EndTurn->SetIsEnabled (View.bCanEndTurn);
        Button_EndTurn->SetToolTipText (
            View.bCanEndTurn
                ? FText::FromString (TEXT ("Terminer le tour actif"))
                : View.EndTurnDisabledReason);
    }
    if (Text_EndTurnDisabledReason)
    {
        Text_EndTurnDisabledReason->SetText (
            View.EndTurnDisabledReason);
        Text_EndTurnDisabledReason->SetVisibility (
            View.bCanEndTurn
                ? ESlateVisibility::Collapsed
                : ESlateVisibility::HitTestInvisible);
    }

    for (int32 Index = 0; Index < PartyMemberPanels.Num (); ++Index)
    {
        if (IsValid (PartyMemberPanels[Index]))
        {
            const bool bPresent = View.PartyMembers.IsValidIndex (Index) &&
                View.PartyMembers[Index].bPresent;
            PartyMemberPanels[Index]->SetVisibility (
                bPresent
                    ? ESlateVisibility::SelfHitTestInvisible
                    : ESlateVisibility::Collapsed);
        }
    }
}

void UGridCombatHudWidget::HandleEndTurnClicked ()
{
    RequestEndTurn ();
}

void UGridCombatHudWidget::HandleInventoryChanged (int32 CharacterIndex)
{
    (void)CharacterIndex;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandlePhaseChanged (EGridCombatPhase NewPhase)
{
    (void)NewPhase;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandleRoundStarted (int32 RoundNumber)
{
    (void)RoundNumber;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandlePlayerTurnStateChanged (
    FGridPlayerCharacterTurnState TurnState)
{
    (void)TurnState;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandlePartyMobilityChanged (
    FGridPartyMobilityState MobilityState)
{
    (void)MobilityState;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandleTurnOrderChanged ()
{
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandleActiveCombatantChanged (
    FGridCombatantInitiativeEntry Combatant)
{
    (void)Combatant;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandleCombatantStateChanged (
    FGridCombatantInitiativeEntry Combatant)
{
    (void)Combatant;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandlePlayerAttackResolved (
    FGridPlayerAttackRequest Request,
    AGridMonsterActor* TargetMonster,
    FGridAttackResult Result)
{
    (void)Request;
    (void)TargetMonster;
    (void)Result;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandlePlayerAttackRejected (
    int32 CharacterIndex,
    EGridPlayerAttackRejectReason RejectReason)
{
    (void)CharacterIndex;
    (void)RejectReason;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandleActionStarted (FGridCombatAction Action)
{
    (void)Action;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandleActionCompleted (
    FGridCombatAction Action,
    bool bSucceeded)
{
    (void)Action;
    (void)bSucceeded;
    RefreshFromSources ();
}

void UGridCombatHudWidget::HandleCombatEnded (
    EGridCombatPhase ResultPhase)
{
    (void)ResultPhase;
    RefreshFromSources ();
}
