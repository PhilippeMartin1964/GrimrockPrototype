#include "UI/GridCombatHudWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Components/WrapBoxSlot.h"
#include "InputCoreTypes.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridCombatActionPanelWidget.h"
#include "UI/GridCombatHotbarDragDropOperation.h"
#include "UI/GridInventoryDragDropOperation.h"

namespace
{
    constexpr float InitiativeRoundSeparatorWidth = 24.0f;
    constexpr float InitiativeRoundSeparatorHeight = 64.0f;
    constexpr float InitiativeRoundSeparatorTextAngle = -90.0f;

    FText GetHotbarShortcutText (int32 SlotIndex)
    {
        return FText::AsNumber (
            SlotIndex == FGridCombatHotbarBinding::SlotCount - 1
                ? 0
                : SlotIndex + 1);
    }

    bool DoesHotbarBindingMatchAction (
        const FGridCombatHotbarBinding& Binding,
        const FGridAvailableCombatAction& Action)
    {
        if (Binding.IsEmpty () ||
            Binding.ActionId != Action.Definition.ActionId ||
            Binding.SourcePolicy != Action.Definition.SourcePolicy ||
            Binding.SourceDefinitionId != Action.SourceDefinitionId)
        {
            return false;
        }

        if (Binding.SourcePolicy ==
            EGridCombatActionSourcePolicy::Equipment)
        {
            return Binding.PreferredSourceRuntimeId.IsValid () &&
                Binding.PreferredSourceRuntimeId == Action.SourceRuntimeId;
        }
        if (Binding.PreferredSourceRuntimeId.IsValid ())
        {
            return Binding.PreferredSourceRuntimeId ==
                Action.SourceRuntimeId;
        }
        return true;
    }

    bool HaveSameHotbarIdentity (
        const FGridCombatHotbarBinding& Left,
        const FGridCombatHotbarBinding& Right)
    {
        return Left.ActionId == Right.ActionId &&
            Left.SourcePolicy == Right.SourcePolicy &&
            Left.SourceDefinitionId == Right.SourceDefinitionId &&
            Left.PreferredSourceRuntimeId ==
                Right.PreferredSourceRuntimeId &&
            Left.PreferredEquipmentSlot ==
                Right.PreferredEquipmentSlot;
    }

    bool IsActionPaletteSource (
        EGridCombatActionSourcePolicy SourcePolicy)
    {
        return SourcePolicy ==
                EGridCombatActionSourcePolicy::Universal ||
            SourcePolicy == EGridCombatActionSourcePolicy::Ability ||
            SourcePolicy == EGridCombatActionSourcePolicy::Spell;
    }

    EGridEquipmentSlot ResolveDraggedEquipmentSlot (
        EGridInventoryUiSlotType SlotType,
        int32 SlotIndex)
    {
        switch (SlotType)
        {
        case EGridInventoryUiSlotType::Equipment:
            return static_cast<EGridEquipmentSlot> (SlotIndex);
        case EGridInventoryUiSlotType::MainHand:
            return EGridEquipmentSlot::MainHand;
        case EGridInventoryUiSlotType::OffHand:
            return EGridEquipmentSlot::OffHand;
        default:
            return EGridEquipmentSlot::None;
        }
    }

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
            Result += Action.Definition.SourcePolicy ==
                    EGridCombatActionSourcePolicy::QuickItem
                ? FString::Printf (
                    TEXT (" | x%d/%d"),
                    Action.CurrentSourceItemQuantityCost,
                    Action.CurrentSourceItemQuantity)
                : FString::Printf (
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

void FGridCombatHudViewModelBuilder::BuildHotbarActions (
    const TArray<FGridCombatHotbarBinding>& Bindings,
    const TArray<FGridAvailableCombatAction>& AvailableActions,
    TArray<FGridCombatHudActionView>& OutActions)
{
    OutActions.Reset (FGridCombatHotbarBinding::SlotCount);
    for (int32 SlotIndex = 0;
        SlotIndex < FGridCombatHotbarBinding::SlotCount;
        ++SlotIndex)
    {
        FGridCombatHudActionView& ActionView =
            OutActions.AddDefaulted_GetRef ();
        ActionView.HotbarSlotIndex = SlotIndex;
        ActionView.ShortcutText = GetHotbarShortcutText (SlotIndex);
        ActionView.Binding.Reset (SlotIndex);
        if (!Bindings.IsValidIndex (SlotIndex) ||
            !Bindings[SlotIndex].IsValid () ||
            Bindings[SlotIndex].IsEmpty ())
        {
            continue;
        }

        ActionView.Binding = Bindings[SlotIndex];
        ActionView.Binding.SlotIndex = SlotIndex;
        ActionView.bHasBinding = true;
        const FGridAvailableCombatAction* ResolvedAction =
            AvailableActions.FindByPredicate (
                [&ActionView] (
                    const FGridAvailableCombatAction& Candidate)
                {
                    return DoesHotbarBindingMatchAction (
                        ActionView.Binding,
                        Candidate);
                });
        if (ResolvedAction)
        {
            ActionView.Action = *ResolvedAction;
            ActionView.bResolved = true;
            ActionView.CostText = FormatActionCost (*ResolvedAction);
            ActionView.DisabledReason = ResolvedAction->bEnabled
                ? FText::GetEmpty ()
                : ResolvedAction->DisabledReason;
            continue;
        }

        ActionView.Action.Definition.ActionId =
            ActionView.Binding.ActionId;
        ActionView.Action.Definition.SourcePolicy =
            ActionView.Binding.SourcePolicy;
        ActionView.Action.SourceDefinitionId =
            ActionView.Binding.SourceDefinitionId;
        ActionView.Action.SourceRuntimeId =
            ActionView.Binding.PreferredSourceRuntimeId;
        ActionView.Action.SourceEquipmentSlot =
            ActionView.Binding.PreferredEquipmentSlot;
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
    const TArray<FGridInitiativePreviewEntry>& InitiativePreview,
    TArray<FGridCombatHudInitiativeView>& OutInitiative,
    int32 MaximumVisibleEntries)
{
    const int32 VisibleCount = FMath::Min (
        InitiativePreview.Num (),
        FMath::Max (0, MaximumVisibleEntries));
    OutInitiative.Reset (VisibleCount);
    for (int32 Index = 0; Index < VisibleCount; ++Index)
    {
        FGridCombatHudInitiativeView& Entry =
            OutInitiative.AddDefaulted_GetRef ();
        const FGridInitiativePreviewEntry& Preview =
            InitiativePreview[Index];
        Entry.Combatant = Preview.Combatant;
        Entry.bActive = Preview.bIsActive;
        Entry.RoundNumber = Preview.RoundNumber;
        Entry.ActivationIndex = Preview.ActivationIndex;
        Entry.bStartsNewRound = Preview.bStartsNewRound;
        Entry.HealthPercent = CalculateHealthPercent (
            Entry.Combatant.CurrentHealth,
            Entry.Combatant.MaximumHealth);
    }
}

float FGridCombatHudViewModelBuilder::CalculateHealthPercent (
    int32 CurrentHealth,
    int32 MaximumHealth)
{
    if (MaximumHealth <= 0)
    {
        return 0.0f;
    }

    return FMath::Clamp (
        static_cast<float> (CurrentHealth) /
            static_cast<float> (MaximumHealth),
        0.0f,
        1.0f);
}

void UGridCombatHudActionWidget::InitializeAction (
    UGridCombatHudWidget* InOwnerHud,
    const FGridCombatHudActionView& InView)
{
    OwnerHud = InOwnerHud;
    bActionPaletteEntry = false;
    View = InView;
    RefreshWidgets ();
}

void UGridCombatHudActionWidget::InitializePaletteAction (
    UGridCombatHudWidget* InOwnerHud,
    const FGridAvailableCombatAction& InAction)
{
    OwnerHud = InOwnerHud;
    bActionPaletteEntry = true;
    View = FGridCombatHudActionView ();
    View.bHasBinding = true;
    View.bResolved = true;
    View.Action = InAction;
    View.Binding.ActionId = InAction.Definition.ActionId;
    View.Binding.SourcePolicy = InAction.Definition.SourcePolicy;
    View.Binding.SourceDefinitionId = InAction.SourceDefinitionId;
    View.CostText = FormatActionCost (InAction);
    View.DisabledReason = InAction.bEnabled
        ? FText::GetEmpty ()
        : InAction.DisabledReason;
    RefreshWidgets ();
}

void UGridCombatHudActionWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    RefreshWidgets ();
}

void UGridCombatHudActionWidget::NativeDestruct ()
{
    bLeftMousePressed = false;
    bDragDetected = false;
    OwnerHud = nullptr;
    Super::NativeDestruct ();
}

bool UGridCombatHudActionWidget::TryExecuteAction ()
{
    if (!IsValid (OwnerHud) ||
        bActionPaletteEntry ||
        !View.bHasBinding)
    {
        return false;
    }

    FGridCombatActionRequestResult Result;
    return OwnerHud->RequestHotbarSlot (
        View.HotbarSlotIndex,
        Result);
}

void UGridCombatHudActionWidget::RefreshWidgets ()
{
    if (Button_Action)
    {
        Button_Action->SetIsEnabled (true);
        if (bActionPaletteEntry)
        {
            Button_Action->SetToolTipText (
                View.Action.Definition.Description.IsEmpty ()
                    ? FText::FromString (TEXT (
                        "Glissez cette action vers un raccourci."))
                    : View.Action.Definition.Description);
        }
        else if (!View.bHasBinding)
        {
            Button_Action->SetToolTipText (FText::FromString (
                TEXT ("Déposez ici une arme équipée, une potion, un parchemin ou une action de la palette.")));
        }
        else
        {
            Button_Action->SetToolTipText (
                View.bResolved && View.Action.bEnabled
                    ? View.Action.Definition.Description
                    : View.DisabledReason);
        }
    }
    if (Image_ActionIcon)
    {
        if (!View.bHasBinding || View.Action.Definition.Icon.IsNull ())
        {
            Image_ActionIcon->SetBrushFromTexture (nullptr);
            Image_ActionIcon->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Image_ActionIcon->SetBrushFromSoftTexture (
                View.Action.Definition.Icon,
                false);
            Image_ActionIcon->SetVisibility (
                ESlateVisibility::HitTestInvisible);
        }
    }
    if (Text_ShortcutNumber)
    {
        Text_ShortcutNumber->SetText (View.ShortcutText);
        Text_ShortcutNumber->SetVisibility (
            bActionPaletteEntry
                ? ESlateVisibility::Collapsed
                : ESlateVisibility::HitTestInvisible);
    }
    if (Text_ActionName)
    {
        FText ActionName = FText::GetEmpty ();
        if (View.bHasBinding)
        {
            const bool bPreferSourceName =
                View.Binding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::Equipment ||
                View.Binding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::QuickItem;
            const FName FallbackName = bPreferSourceName &&
                    !View.Binding.SourceDefinitionId.IsNone ()
                ? View.Binding.SourceDefinitionId
                : View.Binding.ActionId;
            ActionName = View.Action.Definition.DisplayName.IsEmpty ()
                ? FText::FromName (FallbackName)
                : View.Action.Definition.DisplayName;
        }
        Text_ActionName->SetText (bActionPaletteEntry
            ? ActionName
            : (Text_ShortcutNumber
                ? ActionName
                : FText::FromString (View.bHasBinding
                    ? FString::Printf (
                        TEXT ("[%s] %s"),
                        *View.ShortcutText.ToString (),
                        *ActionName.ToString ())
                    : FString::Printf (
                        TEXT ("[%s]"),
                        *View.ShortcutText.ToString ()))));
    }
    if (Text_ActionCost)
    {
        Text_ActionCost->SetText (View.CostText);
        Text_ActionCost->SetVisibility (
            View.bHasBinding && !View.CostText.IsEmpty ()
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
    }
    if (Text_DisabledReason)
    {
        Text_DisabledReason->SetText (View.DisabledReason);
        Text_DisabledReason->SetVisibility (
            View.bHasBinding &&
                (!View.bResolved || !View.Action.bEnabled) &&
                !View.DisabledReason.IsEmpty ()
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed);
    }
    // Keep empty slots easy to identify and target. Dimming the whole widget
    // too aggressively also fades its frame and shortcut number.
    SetRenderOpacity (!View.bHasBinding
        ? FMath::Clamp (EmptySlotOpacity, 0.0f, 1.0f)
        : View.bResolved && View.Action.bEnabled
            ? 1.0f
            : FMath::Clamp (UnavailableSlotOpacity, 0.0f, 1.0f));
}

FReply UGridCombatHudActionWidget::NativeOnPreviewMouseButtonDown (
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (!IsValid (OwnerHud))
    {
        return Super::NativeOnPreviewMouseButtonDown (
            InGeometry,
            InMouseEvent);
    }

    if (bActionPaletteEntry &&
        InMouseEvent.GetEffectingButton () == EKeys::LeftMouseButton)
    {
        bLeftMousePressed = true;
        bDragDetected = false;
        return UWidgetBlueprintLibrary::DetectDragIfPressed (
            InMouseEvent,
            this,
            EKeys::LeftMouseButton).NativeReply;
    }

    if (!bActionPaletteEntry &&
        InMouseEvent.GetEffectingButton () == EKeys::RightMouseButton &&
        View.bHasBinding)
    {
        bLeftMousePressed = false;
        bDragDetected = false;
        return OwnerHud->ClearHotbarSlot (View.HotbarSlotIndex)
            ? FReply::Handled ()
            : FReply::Unhandled ();
    }
    if (InMouseEvent.GetEffectingButton () == EKeys::LeftMouseButton &&
        View.bHasBinding)
    {
        bLeftMousePressed = true;
        bDragDetected = false;
        return UWidgetBlueprintLibrary::DetectDragIfPressed (
            InMouseEvent,
            this,
            EKeys::LeftMouseButton).NativeReply;
    }

    return Super::NativeOnPreviewMouseButtonDown (
        InGeometry,
        InMouseEvent);
}

FReply UGridCombatHudActionWidget::NativeOnMouseButtonUp (
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton () == EKeys::LeftMouseButton &&
        bLeftMousePressed)
    {
        const bool bShouldExecute =
            !bActionPaletteEntry && !bDragDetected;
        bLeftMousePressed = false;
        bDragDetected = false;
        if (bShouldExecute)
        {
            TryExecuteAction ();
        }
        return FReply::Handled ();
    }

    return Super::NativeOnMouseButtonUp (
        InGeometry,
        InMouseEvent);
}

void UGridCombatHudActionWidget::NativeOnDragDetected (
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent,
    UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected (
        InGeometry,
        InMouseEvent,
        OutOperation);
    bDragDetected = true;
    bLeftMousePressed = false;
    if (!IsValid (OwnerHud) || !View.bHasBinding)
    {
        return;
    }

    UGridCombatHotbarDragDropOperation* Operation =
        NewObject<UGridCombatHotbarDragDropOperation> (this);
    if (!Operation)
    {
        return;
    }
    if (bActionPaletteEntry)
    {
        Operation->InitializeFromActionPalette (
            View.Action.CharacterIndex,
            View.Action);
    }
    else
    {
        Operation->InitializeFromHotbarSlot (
            View.Action.CharacterIndex,
            View.HotbarSlotIndex,
            View.Binding);
    }
    Operation->DefaultDragVisual = nullptr;
    Operation->Pivot = EDragPivot::MouseDown;
    OutOperation = Operation;
}

bool UGridCombatHudActionWidget::NativeOnDrop (
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    if (IsValid (OwnerHud) &&
        OwnerHud->HandleHotbarDrop (
            View.HotbarSlotIndex,
            InOperation))
    {
        return true;
    }
    return Super::NativeOnDrop (
        InGeometry,
        InDragDropEvent,
        InOperation);
}

void UGridCombatHudInitiativeSlotWidget::InitializeInitiativeSlot (
    const FGridCombatHudInitiativeView& InView)
{
    View = InView;
    EnsureHealthProgressBar ();
    RefreshWidgets ();
}

void UGridCombatHudInitiativeSlotWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    EnsureHealthProgressBar ();
    RefreshWidgets ();
}

void UGridCombatHudInitiativeSlotWidget::EnsureHealthProgressBar ()
{
    if (ProgressBar_Health || !Text_Health || !WidgetTree)
    {
        return;
    }

    UVerticalBox* TextContainer = Cast<UVerticalBox> (
        Text_Health->GetParent ());
    if (!TextContainer)
    {
        return;
    }

    USizeBox* HealthBarSizeBox =
        WidgetTree->ConstructWidget<USizeBox> (
            USizeBox::StaticClass (),
            TEXT ("SizeBox_HealthBar_Runtime"));
    UProgressBar* HealthBar =
        WidgetTree->ConstructWidget<UProgressBar> (
            UProgressBar::StaticClass (),
            TEXT ("ProgressBar_Health_Runtime"));
    if (!HealthBarSizeBox || !HealthBar)
    {
        return;
    }

    HealthBarSizeBox->SetHeightOverride (6.0f);
    HealthBarSizeBox->AddChild (HealthBar);
    UVerticalBoxSlot* HealthBarSlot =
        TextContainer->AddChildToVerticalBox (HealthBarSizeBox);
    if (HealthBarSlot)
    {
        HealthBarSlot->SetPadding (FMargin (2.0f, 1.0f, 2.0f, 0.0f));
        HealthBarSlot->SetHorizontalAlignment (HAlign_Fill);
    }
    ProgressBar_Health = HealthBar;
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
    if (ProgressBar_Health)
    {
        ProgressBar_Health->SetPercent (View.HealthPercent);
        ProgressBar_Health->SetFillColorAndOpacity (
            HealthBarFillColor);
        ProgressBar_Health->SetVisibility (
            ESlateVisibility::HitTestInvisible);
        ProgressBar_Health->SetToolTipText (FormatCurrentAndMaximum (
            TEXT ("PV"),
            View.Combatant.CurrentHealth,
            View.Combatant.MaximumHealth));
    }
    if (Text_State)
    {
        Text_State->SetText (
            View.bActive
                ? FText::FromString (TEXT ("ACTIF"))
                : FText::GetEmpty ());
        Text_State->SetVisibility (
            View.bActive
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
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
    bCombatActionTargetingActive = false;
    PendingTargetingActionView = FGridCombatHudActionView ();
    TargetingPreview = FGridCombatActionTargetingPreview ();
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
    TArray<FGridAvailableCombatAction> AvailableActions;
    TArray<FGridCombatHotbarBinding> HotbarBindings;
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
        TurnManagerComponent->GetAvailableCombatActions (
            View.ActiveCharacterIndex,
            AvailableActions);
        for (const FGridAvailableCombatAction& Action : AvailableActions)
        {
            if (IsActionPaletteSource (
                Action.Definition.SourcePolicy))
            {
                View.ActionPalette.Add (Action);
            }
        }
        if (IsValid (InventoryComponent))
        {
            HotbarBindings.SetNum (
                FGridCombatHotbarBinding::SlotCount);
            for (int32 SlotIndex = 0;
                SlotIndex < FGridCombatHotbarBinding::SlotCount;
                ++SlotIndex)
            {
                HotbarBindings[SlotIndex].Reset (SlotIndex);
                InventoryComponent->GetCharacterCombatHotbarBinding (
                    View.ActiveCharacterIndex,
                    SlotIndex,
                    HotbarBindings[SlotIndex]);
            }
        }
    }
    FGridCombatHudViewModelBuilder::BuildHotbarActions (
        HotbarBindings,
        AvailableActions,
        View.Actions);
    for (FGridCombatHudActionView& ActionView : View.Actions)
    {
        ActionView.Action.CharacterIndex = View.ActiveCharacterIndex;
    }
    ApplyHotbarPresentationFallbacks ();
    ValidateCombatActionTargetingState ();

    View.Mobility = FGridCombatHudViewModelBuilder::BuildMobility (
        IsValid (TurnManagerComponent)
            ? TurnManagerComponent->GetPartyMobilityState ()
            : FGridPartyMobilityState ());

    TArray<FGridInitiativePreviewEntry> InitiativePreview;
    if (IsValid (TurnManagerComponent))
    {
        TurnManagerComponent->GetInitiativePreview (
            InitiativePreview,
            FMath::Clamp (VisibleInitiativeSlotCount, 7, 10));
    }
    FGridCombatHudViewModelBuilder::BuildInitiative (
        InitiativePreview,
        View.Initiative,
        FMath::Clamp (VisibleInitiativeSlotCount, 7, 10));

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
    EnsureActionWidgets ();
    RefreshActionWidgets ();
    EnsureActionPaletteWidgets ();
    RefreshActionPaletteWidgets ();
    EnsureInitiativeWidgets ();
    RefreshInitiativeWidgets ();
    RefreshBoundWidgets ();
    RefreshTargetingWidgets ();
}

bool UGridCombatHudWidget::RequestCombatAction (
    const FGridCombatHudActionView& ActionView,
    FGridCombatActionRequestResult& OutResult)
{
    OutResult = FGridCombatActionRequestResult ();
    if (!IsValid (TurnManagerComponent) ||
        !ActionView.bHasBinding ||
        !ActionView.bResolved)
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

bool UGridCombatHudWidget::RequestHotbarSlot (
    int32 SlotIndex,
    FGridCombatActionRequestResult& OutResult)
{
    OutResult = FGridCombatActionRequestResult ();
    if (SlotIndex < 0 ||
        SlotIndex >= FGridCombatHotbarBinding::SlotCount)
    {
        return false;
    }

    // A binding is only an identity. Resolve it again immediately before the
    // request so keyboard input cannot execute a stale cached action.
    RefreshFromSources ();
    if (!View.Actions.IsValidIndex (SlotIndex))
    {
        return false;
    }

    const FGridCombatHudActionView ActionView = View.Actions[SlotIndex];
    if (ActionView.bHasBinding &&
        ActionView.bResolved &&
        ActionView.Action.bEnabled &&
        (ActionView.Action.Definition.TargetingPolicy ==
                EGridCombatTargetingPolicy::Cell ||
            ActionView.Action.Definition.TargetingPolicy ==
                EGridCombatTargetingPolicy::Area))
    {
        OutResult.Action = ActionView.Action;
        return BeginCombatActionTargeting (ActionView);
    }
    if (ActionView.bHasBinding &&
        ActionView.bResolved &&
        ActionView.Action.bEnabled)
    {
        CancelCombatActionTargeting ();
    }
    return RequestCombatAction (ActionView, OutResult);
}

bool UGridCombatHudWidget::BeginCombatActionTargeting (
    const FGridCombatHudActionView& ActionView)
{
    if (!IsValid (TurnManagerComponent) ||
        !ActionView.bHasBinding ||
        !ActionView.bResolved ||
        !ActionView.Action.bEnabled ||
        (ActionView.Action.Definition.TargetingPolicy !=
                EGridCombatTargetingPolicy::Cell &&
            ActionView.Action.Definition.TargetingPolicy !=
                EGridCombatTargetingPolicy::Area))
    {
        return false;
    }

    bCombatActionTargetingActive = true;
    PendingTargetingActionView = ActionView;
    TargetingPreview = FGridCombatActionTargetingPreview ();
    TargetingPreview.Action = ActionView.Action;
    RefreshTargetingWidgets ();
    return true;
}

bool UGridCombatHudWidget::UpdateCombatActionTargetingPreview (
    FIntPoint TargetCell)
{
    if (!bCombatActionTargetingActive ||
        !IsValid (TurnManagerComponent))
    {
        return false;
    }

    const FGridAvailableCombatAction& Action =
        PendingTargetingActionView.Action;
    const bool bValid =
        TurnManagerComponent->BuildCombatActionTargetingPreview (
            Action.CharacterIndex,
            Action.Definition.ActionId,
            Action.Definition.SourcePolicy,
            Action.SourceDefinitionId,
            Action.SourceEquipmentSlot,
            TargetCell,
            TargetingPreview);
    RefreshTargetingWidgets ();
    return bValid;
}

bool UGridCombatHudWidget::ConfirmCombatActionTarget (
    FIntPoint TargetCell,
    FGridCombatActionRequestResult& OutResult)
{
    OutResult = FGridCombatActionRequestResult ();
    if (!UpdateCombatActionTargetingPreview (TargetCell) ||
        !TargetingPreview.bValid ||
        !IsValid (TurnManagerComponent))
    {
        OutResult.Action = TargetingPreview.Action;
        OutResult.RejectReason =
            EGridCombatActionRequestRejectReason::InvalidTarget;
        return false;
    }

    const FGridAvailableCombatAction Action = TargetingPreview.Action;
    const bool bAccepted =
        TurnManagerComponent->RequestCharacterCombatActionAtCell (
            Action.CharacterIndex,
            Action.Definition.ActionId,
            Action.Definition.SourcePolicy,
            Action.SourceDefinitionId,
            Action.SourceEquipmentSlot,
            TargetCell,
            OutResult);
    if (bAccepted)
    {
        bCombatActionTargetingActive = false;
        PendingTargetingActionView = FGridCombatHudActionView ();
        TargetingPreview = FGridCombatActionTargetingPreview ();
    }
    RefreshFromSources ();
    return bAccepted;
}

void UGridCombatHudWidget::ClearCombatActionTargetingPreview ()
{
    if (!bCombatActionTargetingActive)
    {
        return;
    }
    TargetingPreview = FGridCombatActionTargetingPreview ();
    TargetingPreview.Action = PendingTargetingActionView.Action;
    RefreshTargetingWidgets ();
}

void UGridCombatHudWidget::CancelCombatActionTargeting ()
{
    bCombatActionTargetingActive = false;
    PendingTargetingActionView = FGridCombatHudActionView ();
    TargetingPreview = FGridCombatActionTargetingPreview ();
    RefreshTargetingWidgets ();
}

bool UGridCombatHudWidget::RequestEndTurn ()
{
    if (!IsValid (TurnManagerComponent))
    {
        return false;
    }
    CancelCombatActionTargeting ();
    const bool bEnded = TurnManagerComponent->EndActivePlayerTurn ();
    RefreshFromSources ();
    return bEnded;
}

bool UGridCombatHudWidget::HandleHotbarDrop (
    int32 TargetSlotIndex,
    UDragDropOperation* DragOperation)
{
    if (!IsValid (InventoryComponent) ||
        View.ActiveCharacterIndex == INDEX_NONE ||
        TargetSlotIndex < 0 ||
        TargetSlotIndex >= FGridCombatHotbarBinding::SlotCount ||
        !IsValid (DragOperation))
    {
        return false;
    }

    if (const UGridCombatHotbarDragDropOperation* HotbarOperation =
        Cast<UGridCombatHotbarDragDropOperation> (DragOperation))
    {
        if (HotbarOperation->bFromActionPalette)
        {
            TArray<FGridAvailableCombatAction> CurrentActions;
            if (IsValid (TurnManagerComponent))
            {
                TurnManagerComponent->GetAvailableCombatActions (
                    View.ActiveCharacterIndex,
                    CurrentActions);
            }
            const FGridAvailableCombatAction* CurrentAction =
                CurrentActions.FindByPredicate (
                    [HotbarOperation] (
                        const FGridAvailableCombatAction& Candidate)
                    {
                        return IsActionPaletteSource (
                                Candidate.Definition.SourcePolicy) &&
                            DoesHotbarBindingMatchAction (
                                HotbarOperation->Binding,
                                Candidate);
                    });
            return HotbarOperation->CharacterIndex ==
                    View.ActiveCharacterIndex &&
                CurrentAction &&
                AssignCombatActionToHotbarSlot (
                    TargetSlotIndex,
                    *CurrentAction);
        }

        FGridCombatHotbarBinding CurrentBinding;
        return HotbarOperation->CharacterIndex ==
                View.ActiveCharacterIndex &&
            InventoryComponent->GetCharacterCombatHotbarBinding (
                View.ActiveCharacterIndex,
                HotbarOperation->SourceSlotIndex,
                CurrentBinding) &&
            HaveSameHotbarIdentity (
                CurrentBinding,
                HotbarOperation->Binding) &&
            InventoryComponent->MoveOrSwapCharacterCombatHotbarBinding (
                View.ActiveCharacterIndex,
                HotbarOperation->SourceSlotIndex,
                TargetSlotIndex);
    }

    const UGridInventoryDragDropOperation* InventoryOperation =
        Cast<UGridInventoryDragDropOperation> (DragOperation);
    if (!InventoryOperation || !InventoryOperation->bHasItem ||
        InventoryOperation->SourceSlotType ==
            EGridInventoryUiSlotType::Cursor)
    {
        return false;
    }

    const EGridEquipmentSlot EquipmentSlot =
        ResolveDraggedEquipmentSlot (
            InventoryOperation->SourceSlotType,
            InventoryOperation->SourceSlotIndex);
    return InventoryComponent->SetCharacterCombatHotbarBindingFromItem (
        View.ActiveCharacterIndex,
        TargetSlotIndex,
        InventoryOperation->SourceItem,
        EquipmentSlot);
}

bool UGridCombatHudWidget::AssignCombatActionToHotbarSlot (
    int32 TargetSlotIndex,
    const FGridAvailableCombatAction& Action)
{
    if (!IsValid (InventoryComponent) ||
        !IsValid (TurnManagerComponent) ||
        View.ActiveCharacterIndex == INDEX_NONE ||
        Action.CharacterIndex != View.ActiveCharacterIndex ||
        TargetSlotIndex < 0 ||
        TargetSlotIndex >= FGridCombatHotbarBinding::SlotCount ||
        !IsActionPaletteSource (Action.Definition.SourcePolicy))
    {
        return false;
    }

    TArray<FGridAvailableCombatAction> CurrentActions;
    TurnManagerComponent->GetAvailableCombatActions (
        View.ActiveCharacterIndex,
        CurrentActions);
    const FGridAvailableCombatAction* CurrentAction =
        CurrentActions.FindByPredicate (
            [&Action] (const FGridAvailableCombatAction& Candidate)
            {
                return Candidate.MatchesSource (
                    Action.Definition.ActionId,
                    Action.Definition.SourcePolicy,
                    Action.SourceDefinitionId,
                    Action.SourceEquipmentSlot);
            });
    if (!CurrentAction ||
        !IsActionPaletteSource (
            CurrentAction->Definition.SourcePolicy))
    {
        return false;
    }

    FGridCombatHotbarBinding Binding;
    Binding.SlotIndex = TargetSlotIndex;
    Binding.ActionId = CurrentAction->Definition.ActionId;
    Binding.SourcePolicy = CurrentAction->Definition.SourcePolicy;
    Binding.SourceDefinitionId = CurrentAction->SourceDefinitionId;
    return Binding.IsValid () &&
        InventoryComponent->SetCharacterCombatHotbarBinding (
            View.ActiveCharacterIndex,
            TargetSlotIndex,
            Binding);
}

bool UGridCombatHudWidget::ClearHotbarSlot (int32 SlotIndex)
{
    return IsValid (InventoryComponent) &&
        View.ActiveCharacterIndex != INDEX_NONE &&
        InventoryComponent->ClearCharacterCombatHotbarBinding (
            View.ActiveCharacterIndex,
            SlotIndex);
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
    CancelCombatActionTargeting ();
    if (Button_EndTurn)
    {
        Button_EndTurn->OnClicked.RemoveDynamic (
            this,
            &UGridCombatHudWidget::HandleEndTurnClicked);
    }
    UnbindFromSources ();
    PartyMemberPanels.Reset ();
    HotbarActionWidgets.Reset ();
    ActionPaletteWidgets.Reset ();
    HotbarRow = nullptr;
    InitiativeSlotWidgets.Reset ();
    InitiativeRoundSeparatorWidgets.Reset ();
    InitiativeRoundSeparatorTexts.Reset ();
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
        Panel->InitializeCombatActionPanel (
            PartyPawn,
            CharacterIndex,
            TurnManagerComponent);
        Panel_PartyMembers->AddChild (Panel);
        PartyMemberPanels.Add (Panel);
    }
}

void UGridCombatHudWidget::ApplyHotbarPresentationFallbacks ()
{
    for (FGridCombatHudActionView& ActionView : View.Actions)
    {
        if (!ActionView.bHasBinding)
        {
            continue;
        }

        const UGridItemDefinitionAsset* ItemDefinition =
            IsValid (InventoryComponent) &&
                !ActionView.Binding.SourceDefinitionId.IsNone ()
            ? InventoryComponent->FindItemDefinition (
                ActionView.Binding.SourceDefinitionId)
            : nullptr;
        if (IsValid (ItemDefinition))
        {
            if (ActionView.Action.Definition.DisplayName.IsEmpty ())
            {
                ActionView.Action.Definition.DisplayName =
                    ItemDefinition->DisplayName;
            }
            if (ActionView.Action.Definition.Description.IsEmpty ())
            {
                ActionView.Action.Definition.Description =
                    ItemDefinition->Description;
            }
            if (ActionView.Action.Definition.Icon.IsNull ())
            {
                ActionView.Action.Definition.Icon = ItemDefinition->Icon;
            }
        }

        if (ActionView.bResolved)
        {
            continue;
        }

        switch (ActionView.Binding.SourcePolicy)
        {
        case EGridCombatActionSourcePolicy::QuickItem:
            ActionView.DisabledReason = FText::FromString (TEXT (
                "Aucune action de combat n’est configurée dans la "
                "définition de cette potion ou de ce parchemin."));
            break;
        case EGridCombatActionSourcePolicy::Equipment:
            ActionView.DisabledReason = FText::FromString (TEXT (
                "L’arme liée n’est plus équipée par ce personnage."));
            break;
        default:
            ActionView.DisabledReason = FText::FromString (TEXT (
                "Cette action n’est pas disponible actuellement."));
            break;
        }
    }
}

void UGridCombatHudWidget::EnsureActionWidgets ()
{
    if (!Panel_Actions || !ActionWidgetClass)
    {
        return;
    }

    if (UHorizontalBox* DesignerRow =
        Cast<UHorizontalBox> (Panel_Actions))
    {
        HotbarRow = DesignerRow;
    }
    else if (!IsValid (HotbarRow) ||
        HotbarRow->GetParent () != Panel_Actions)
    {
        Panel_Actions->ClearChildren ();
        HotbarRow = WidgetTree
            ? WidgetTree->ConstructWidget<UHorizontalBox> (
                UHorizontalBox::StaticClass (),
                MakeUniqueObjectName (
                    WidgetTree,
                    UHorizontalBox::StaticClass (),
                    TEXT ("HorizontalBox_Hotbar_Runtime")))
            : NewObject<UHorizontalBox> (
                this,
                MakeUniqueObjectName (
                    this,
                    UHorizontalBox::StaticClass (),
                    TEXT ("HorizontalBox_Hotbar_Runtime")));
        if (!HotbarRow)
        {
            return;
        }
        UPanelSlot* ContainerSlot = Panel_Actions->AddChild (HotbarRow);
        if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot> (ContainerSlot))
        {
            WrapSlot->SetFillEmptySpace (true);
        }
    }

    bool bPoolValid =
        IsValid (HotbarRow) &&
        HotbarActionWidgets.Num () ==
            FGridCombatHotbarBinding::SlotCount &&
        HotbarRow->GetChildrenCount () ==
            FGridCombatHotbarBinding::SlotCount;
    for (const UGridCombatHudActionWidget* ActionWidget :
        HotbarActionWidgets)
    {
        bPoolValid = bPoolValid && IsValid (ActionWidget) &&
            ActionWidget->GetParent () == HotbarRow;
    }
    if (bPoolValid)
    {
        return;
    }

    HotbarRow->ClearChildren ();
    HotbarActionWidgets.Reset (FGridCombatHotbarBinding::SlotCount);
    for (int32 SlotIndex = 0;
        SlotIndex < FGridCombatHotbarBinding::SlotCount;
        ++SlotIndex)
    {
        UGridCombatHudActionWidget* ActionWidget =
            CreateWidget<UGridCombatHudActionWidget> (
                this,
                ActionWidgetClass);
        if (ActionWidget)
        {
            UHorizontalBoxSlot* HotbarSlot =
                HotbarRow->AddChildToHorizontalBox (ActionWidget);
            if (HotbarSlot)
            {
                HotbarSlot->SetSize (
                    FSlateChildSize (ESlateSizeRule::Fill));
                const float HalfSpacing =
                    FMath::Max (0.0f, HotbarSlotSpacing) * 0.5f;
                HotbarSlot->SetPadding (FMargin (
                    HalfSpacing,
                    0.0f,
                    HalfSpacing,
                    0.0f));
                HotbarSlot->SetHorizontalAlignment (HAlign_Fill);
                HotbarSlot->SetVerticalAlignment (VAlign_Fill);
            }
            HotbarActionWidgets.Add (ActionWidget);
        }
    }
}

void UGridCombatHudWidget::RefreshActionWidgets ()
{
    for (int32 SlotIndex = 0;
        SlotIndex < HotbarActionWidgets.Num ();
        ++SlotIndex)
    {
        UGridCombatHudActionWidget* ActionWidget =
            HotbarActionWidgets[SlotIndex];
        if (!IsValid (ActionWidget) ||
            !View.Actions.IsValidIndex (SlotIndex))
        {
            continue;
        }
        ActionWidget->InitializeAction (
            this,
            View.Actions[SlotIndex]);
        ActionWidget->SetVisibility (ESlateVisibility::Visible);
    }
}

void UGridCombatHudWidget::EnsureActionPaletteWidgets ()
{
    if (!Panel_ActionPalette ||
        Panel_ActionPalette == Panel_Actions ||
        !ActionWidgetClass)
    {
        return;
    }

    const int32 DesiredCount = View.ActionPalette.Num ();
    bool bPoolValid =
        ActionPaletteWidgets.Num () == DesiredCount &&
        Panel_ActionPalette->GetChildrenCount () == DesiredCount;
    for (const UGridCombatHudActionWidget* ActionWidget :
        ActionPaletteWidgets)
    {
        bPoolValid = bPoolValid && IsValid (ActionWidget) &&
            ActionWidget->GetParent () == Panel_ActionPalette;
    }
    if (bPoolValid)
    {
        return;
    }

    Panel_ActionPalette->ClearChildren ();
    ActionPaletteWidgets.Reset (DesiredCount);
    for (int32 ActionIndex = 0;
        ActionIndex < DesiredCount;
        ++ActionIndex)
    {
        UGridCombatHudActionWidget* ActionWidget =
            CreateWidget<UGridCombatHudActionWidget> (
                this,
                ActionWidgetClass);
        if (!ActionWidget)
        {
            continue;
        }

        UPanelSlot* PaletteSlot =
            Panel_ActionPalette->AddChild (ActionWidget);
        const float HalfSpacing =
            FMath::Max (0.0f, HotbarSlotSpacing) * 0.5f;
        if (UHorizontalBoxSlot* HorizontalSlot =
            Cast<UHorizontalBoxSlot> (PaletteSlot))
        {
            HorizontalSlot->SetSize (
                FSlateChildSize (ESlateSizeRule::Automatic));
            HorizontalSlot->SetPadding (FMargin (
                HalfSpacing,
                0.0f,
                HalfSpacing,
                0.0f));
            HorizontalSlot->SetVerticalAlignment (VAlign_Fill);
        }
        else if (UWrapBoxSlot* WrapSlot =
            Cast<UWrapBoxSlot> (PaletteSlot))
        {
            WrapSlot->SetPadding (FMargin (
                HalfSpacing,
                0.0f,
                HalfSpacing,
                0.0f));
        }
        ActionPaletteWidgets.Add (ActionWidget);
    }
}

void UGridCombatHudWidget::RefreshActionPaletteWidgets ()
{
    if (!Panel_ActionPalette ||
        Panel_ActionPalette == Panel_Actions)
    {
        return;
    }

    Panel_ActionPalette->SetVisibility (
        View.ActionPalette.IsEmpty ()
            ? ESlateVisibility::Collapsed
            : ESlateVisibility::Visible);
    for (int32 ActionIndex = 0;
        ActionIndex < ActionPaletteWidgets.Num ();
        ++ActionIndex)
    {
        UGridCombatHudActionWidget* ActionWidget =
            ActionPaletteWidgets[ActionIndex];
        if (!IsValid (ActionWidget) ||
            !View.ActionPalette.IsValidIndex (ActionIndex))
        {
            continue;
        }
        ActionWidget->InitializePaletteAction (
            this,
            View.ActionPalette[ActionIndex]);
        ActionWidget->SetVisibility (ESlateVisibility::Visible);
    }
}

void UGridCombatHudWidget::EnsureInitiativeWidgets ()
{
    if (!Panel_Initiative || !InitiativeSlotWidgetClass)
    {
        return;
    }

    const int32 SlotCount = FMath::Clamp (
        VisibleInitiativeSlotCount,
        7,
        10);
    bool bPoolValid =
        InitiativeSlotWidgets.Num () == SlotCount &&
        InitiativeRoundSeparatorWidgets.Num () == SlotCount - 1 &&
        InitiativeRoundSeparatorTexts.Num () == SlotCount - 1;
    for (const UGridCombatHudInitiativeSlotWidget* SlotWidget :
        InitiativeSlotWidgets)
    {
        bPoolValid = bPoolValid && IsValid (SlotWidget);
    }
    for (int32 Index = 0;
        Index < InitiativeRoundSeparatorWidgets.Num ();
        ++Index)
    {
        bPoolValid = bPoolValid &&
            IsValid (InitiativeRoundSeparatorWidgets[Index]) &&
            InitiativeRoundSeparatorTexts.IsValidIndex (Index) &&
            IsValid (InitiativeRoundSeparatorTexts[Index]);
    }
    if (bPoolValid)
    {
        return;
    }

    Panel_Initiative->ClearChildren ();
    InitiativeSlotWidgets.Reset (SlotCount);
    InitiativeRoundSeparatorWidgets.Reset (SlotCount - 1);
    InitiativeRoundSeparatorTexts.Reset (SlotCount - 1);
    for (int32 Index = 0; Index < SlotCount; ++Index)
    {
        UGridCombatHudInitiativeSlotWidget* InitiativeWidget =
            CreateWidget<UGridCombatHudInitiativeSlotWidget> (
                this,
                InitiativeSlotWidgetClass);
        if (InitiativeWidget)
        {
            InitiativeWidget->SetVisibility (ESlateVisibility::Collapsed);
            Panel_Initiative->AddChild (InitiativeWidget);
            InitiativeSlotWidgets.Add (InitiativeWidget);
        }
    }

    for (int32 Index = 0; Index < SlotCount - 1; ++Index)
    {
        UBorder* Separator = NewObject<UBorder> (
            this,
            UBorder::StaticClass (),
            MakeUniqueObjectName (
                this,
                UBorder::StaticClass (),
                TEXT ("Border_InitiativeRoundSeparator")));
        UTextBlock* SeparatorText = NewObject<UTextBlock> (
            this,
            UTextBlock::StaticClass (),
            MakeUniqueObjectName (
                this,
                UTextBlock::StaticClass (),
                TEXT ("Text_InitiativeRoundNumber")));
        USizeBox* SeparatorContent = NewObject<USizeBox> (
            this,
            USizeBox::StaticClass (),
            MakeUniqueObjectName (
                this,
                USizeBox::StaticClass (),
                TEXT ("SizeBox_InitiativeRoundSeparator")));
        UCanvasPanel* SeparatorCanvas = NewObject<UCanvasPanel> (
            this,
            UCanvasPanel::StaticClass (),
            MakeUniqueObjectName (
                this,
                UCanvasPanel::StaticClass (),
                TEXT ("Canvas_InitiativeRoundSeparator")));
        if (!Separator || !SeparatorText || !SeparatorContent ||
            !SeparatorCanvas)
        {
            continue;
        }

        Separator->SetBrushColor (FLinearColor (
            0.28f,
            0.18f,
            0.06f,
            0.92f));
        Separator->SetPadding (FMargin (2.0f, 6.0f));
        Separator->SetHorizontalAlignment (HAlign_Center);
        Separator->SetVerticalAlignment (VAlign_Center);
        Separator->SetVisibility (ESlateVisibility::Collapsed);
        SeparatorContent->SetWidthOverride (
            InitiativeRoundSeparatorWidth);
        SeparatorContent->SetHeightOverride (
            InitiativeRoundSeparatorHeight);
        SeparatorText->SetColorAndOpacity (FSlateColor (FLinearColor (
            0.92f,
            0.78f,
            0.45f,
            1.0f)));
        SeparatorText->SetJustification (ETextJustify::Center);
        SeparatorText->SetRenderTransformPivot (FVector2D (0.5f, 0.5f));
        SeparatorText->SetRenderTransformAngle (
            InitiativeRoundSeparatorTextAngle);
        FSlateFontInfo Font = SeparatorText->GetFont ();
        Font.Size = 10;
        SeparatorText->SetFont (Font);
        UCanvasPanelSlot* SeparatorTextSlot =
            SeparatorCanvas->AddChildToCanvas (SeparatorText);
        if (!SeparatorTextSlot)
        {
            continue;
        }
        SeparatorTextSlot->SetAnchors (FAnchors (0.5f, 0.5f));
        SeparatorTextSlot->SetAlignment (FVector2D (0.5f, 0.5f));
        SeparatorTextSlot->SetPosition (FVector2D::ZeroVector);
        SeparatorTextSlot->SetAutoSize (true);
        SeparatorContent->AddChild (SeparatorCanvas);
        Separator->AddChild (SeparatorContent);
        InitiativeRoundSeparatorWidgets.Add (Separator);
        InitiativeRoundSeparatorTexts.Add (SeparatorText);
    }
}

void UGridCombatHudWidget::RefreshInitiativeWidgets ()
{
    if (!Panel_Initiative)
    {
        return;
    }

    Panel_Initiative->ClearChildren ();
    int32 UsedSeparatorCount = 0;
    for (int32 Index = 0;
        Index < InitiativeSlotWidgets.Num ();
        ++Index)
    {
        UGridCombatHudInitiativeSlotWidget* InitiativeWidget =
            InitiativeSlotWidgets[Index];
        if (!IsValid (InitiativeWidget))
        {
            continue;
        }
        if (!View.Initiative.IsValidIndex (Index))
        {
            InitiativeWidget->SetVisibility (ESlateVisibility::Collapsed);
            Panel_Initiative->AddChild (InitiativeWidget);
            continue;
        }

        const FGridCombatHudInitiativeView& InitiativeView =
            View.Initiative[Index];
        if (InitiativeView.bStartsNewRound &&
            InitiativeRoundSeparatorWidgets.IsValidIndex (
                UsedSeparatorCount) &&
            InitiativeRoundSeparatorTexts.IsValidIndex (
                UsedSeparatorCount))
        {
            UBorder* Separator =
                InitiativeRoundSeparatorWidgets[UsedSeparatorCount];
            UTextBlock* SeparatorText =
                InitiativeRoundSeparatorTexts[UsedSeparatorCount];
            SeparatorText->SetText (FText::FromString (FString::Printf (
                TEXT ("ROUND %d"),
                InitiativeView.RoundNumber)));
            Separator->SetVisibility (
                ESlateVisibility::SelfHitTestInvisible);
            Panel_Initiative->AddChild (Separator);
            ++UsedSeparatorCount;
        }

        InitiativeWidget->InitializeInitiativeSlot (
            InitiativeView);
        InitiativeWidget->SetVisibility (
            ESlateVisibility::SelfHitTestInvisible);
        Panel_Initiative->AddChild (InitiativeWidget);
    }

    for (int32 Index = UsedSeparatorCount;
        Index < InitiativeRoundSeparatorWidgets.Num ();
        ++Index)
    {
        if (IsValid (InitiativeRoundSeparatorWidgets[Index]))
        {
            InitiativeRoundSeparatorWidgets[Index]->SetVisibility (
                ESlateVisibility::Collapsed);
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

void UGridCombatHudWidget::ValidateCombatActionTargetingState ()
{
    if (!bCombatActionTargetingActive)
    {
        return;
    }

    const FGridCombatHudActionView* CurrentActionView =
        View.Actions.FindByPredicate (
            [this] (const FGridCombatHudActionView& Candidate)
            {
                return Candidate.bHasBinding &&
                    Candidate.bResolved &&
                    Candidate.Action.bEnabled &&
                    HaveSameHotbarIdentity (
                        Candidate.Binding,
                        PendingTargetingActionView.Binding) &&
                    (Candidate.Action.Definition.TargetingPolicy ==
                            EGridCombatTargetingPolicy::Cell ||
                        Candidate.Action.Definition.TargetingPolicy ==
                            EGridCombatTargetingPolicy::Area);
            });
    if (!CurrentActionView ||
        CurrentActionView->Action.CharacterIndex !=
            View.ActiveCharacterIndex)
    {
        bCombatActionTargetingActive = false;
        PendingTargetingActionView = FGridCombatHudActionView ();
        TargetingPreview = FGridCombatActionTargetingPreview ();
        return;
    }

    PendingTargetingActionView = *CurrentActionView;
    TargetingPreview.Action = CurrentActionView->Action;
}

void UGridCombatHudWidget::RefreshTargetingWidgets ()
{
    if (Panel_Targeting)
    {
        Panel_Targeting->SetVisibility (
            bCombatActionTargetingActive
                ? ESlateVisibility::SelfHitTestInvisible
                : ESlateVisibility::Collapsed);
    }
    if (Text_TargetingInstructions)
    {
        const FText ActionName =
            PendingTargetingActionView.Action.Definition.DisplayName.IsEmpty ()
                ? FText::FromName (
                    PendingTargetingActionView.Action.Definition.ActionId)
                : PendingTargetingActionView.Action.Definition.DisplayName;
        Text_TargetingInstructions->SetText (
            bCombatActionTargetingActive
                ? FText::FromString (FString::Printf (
                    TEXT ("%s : cliquez une cellule pour confirmer — Échap pour annuler"),
                    *ActionName.ToString ()))
                : FText::GetEmpty ());
    }
    if (Text_TargetingCell)
    {
        FText Status = FText::GetEmpty ();
        if (bCombatActionTargetingActive)
        {
            if (TargetingPreview.TargetCell.X == INDEX_NONE ||
                TargetingPreview.TargetCell.Y == INDEX_NONE)
            {
                Status = FText::FromString (
                    TEXT ("Survolez une cellule du donjon."));
            }
            else if (TargetingPreview.bValid)
            {
                Status = FText::FromString (FString::Printf (
                    TEXT ("Cellule (%d,%d) — %d cible(s)"),
                    TargetingPreview.TargetCell.X,
                    TargetingPreview.TargetCell.Y,
                    TargetingPreview.TargetMonsterIds.Num ()));
            }
            else
            {
                Status = TargetingPreview.InvalidReason;
            }
        }
        Text_TargetingCell->SetText (Status);
        Text_TargetingCell->SetVisibility (
            bCombatActionTargetingActive
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
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
