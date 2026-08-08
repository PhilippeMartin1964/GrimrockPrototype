#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridCombatHudWidget.generated.h"

class AGrimrockPartyPawn;
class UBorder;
class UButton;
class UDragDropOperation;
class UGridCombatActionPanelWidget;
class UGridCombatHotbarDragDropOperation;
class UGridPartyInventoryComponent;
class UGridTurnManagerComponent;
class UHorizontalBox;
class UImage;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UTexture2D;
class UWidget;

USTRUCT (BlueprintType)
struct FGridCombatHudPartyMemberView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bPresent = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bActive = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FText DisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 CurrentHealth = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 MaximumHealth = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 CurrentMana = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 MaximumMana = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    EGridCombatantTurnState TurnState = EGridCombatantTurnState::Waiting;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 RemainingActionPoints = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 MaximumActionPoints = 0;
};

USTRUCT (BlueprintType)
struct FGridCombatHudActionView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 HotbarSlotIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FText ShortcutText;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridCombatHotbarBinding Binding;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bHasBinding = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bResolved = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridAvailableCombatAction Action;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FText CostText;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FText DisabledReason;
};

USTRUCT (BlueprintType)
struct FGridCombatHudMobilityView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 RemainingMobilityActionPoints = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 MaximumMobilityActionPoints = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FText DisplayText;
};

USTRUCT (BlueprintType)
struct FGridCombatHudInitiativeView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridCombatantInitiativeEntry Combatant;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bActive = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 RoundNumber = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 ActivationIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bStartsNewRound = false;

    /** UI-ready health ratio, clamped to the progress-bar range. */
    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    float HealthPercent = 0.0f;
};

USTRUCT (BlueprintType)
struct FGridCombatHudView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bCombatActive = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 ActiveCharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    TArray<FGridCombatHudPartyMemberView> PartyMembers;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    TArray<FGridCombatHudActionView> Actions;

    /** Configurable class/universal actions shown outside the fixed hotbar. */
    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    TArray<FGridAvailableCombatAction> ActionPalette;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridCombatHudMobilityView Mobility;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    TArray<FGridCombatHudInitiativeView> Initiative;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bCanEndTurn = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FText EndTurnDisabledReason;
};

/** Pure, deterministic projection helpers used by runtime code and tests. */
class GRIMROCKPROTOTYPE_API FGridCombatHudViewModelBuilder
{
public:
    static constexpr int32 PartyPanelCount = 4;
    static constexpr int32 MaximumVisibleInitiativeEntries = 8;

    static void BuildPartyMembers (
        int32 ActiveCharacterCount,
        const TArray<FGridPlayerCharacterTurnState>& TurnStates,
        TArray<FGridCombatHudPartyMemberView>& OutMembers);

    static void BuildHotbarActions (
        const TArray<FGridCombatHotbarBinding>& Bindings,
        const TArray<FGridAvailableCombatAction>& AvailableActions,
        TArray<FGridCombatHudActionView>& OutActions);

    static FGridCombatHudMobilityView BuildMobility (
        const FGridPartyMobilityState& MobilityState);

    static void BuildInitiative (
        const TArray<FGridInitiativePreviewEntry>& InitiativePreview,
        TArray<FGridCombatHudInitiativeView>& OutInitiative,
        int32 MaximumVisibleEntries = MaximumVisibleInitiativeEntries);

    static float CalculateHealthPercent (
        int32 CurrentHealth,
        int32 MaximumHealth);
};

class UGridCombatHudWidget;

/** One fixed, configurable combat hotbar slot. */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridCombatHudActionWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|HUD|Visuals",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EmptySlotOpacity = 0.8f;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|HUD|Visuals",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UnavailableSlotOpacity = 0.65f;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridCombatHudActionView View;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UButton> Button_Action;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UImage> Image_ActionIcon;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_ActionName;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_ActionCost;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_DisabledReason;

    /** Optional. Without it the shortcut is prefixed to Text_ActionName. */
    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_ShortcutNumber;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    bool bActionPaletteEntry = false;

    void InitializeAction (
        UGridCombatHudWidget* InOwnerHud,
        const FGridCombatHudActionView& InView);

    /** Reuses the slot visual as a draggable, non-executable palette entry. */
    void InitializePaletteAction (
        UGridCombatHudWidget* InOwnerHud,
        const FGridAvailableCombatAction& InAction);

    /** Executes the current binding through the owning authoritative HUD. */
    UFUNCTION (BlueprintCallable, Category = "Combat|HUD|Hotbar")
    bool TryExecuteAction ();

protected:
    virtual void NativeConstruct () override;
    virtual void NativeDestruct () override;

    virtual FReply NativeOnPreviewMouseButtonDown (
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseButtonUp (
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual void NativeOnDragDetected (
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent,
        UDragDropOperation*& OutOperation) override;

    virtual bool NativeOnDrop (
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;

private:
    UPROPERTY (Transient)
    TObjectPtr<UGridCombatHudWidget> OwnerHud;

    bool bLeftMousePressed = false;
    bool bDragDetected = false;

    void RefreshWidgets ();
};

/** One read-only entry copied from the TurnManager upcoming order. */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridCombatHudInitiativeSlotWidget
    : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridCombatHudInitiativeView View;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|HUD|Visuals")
    float ActiveScale = 1.12f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|HUD|Visuals")
    FLinearColor HealthBarFillColor = FLinearColor (
        0.82f,
        0.02f,
        0.02f,
        1.0f);

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UImage> Image_Portrait;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_Name;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_Health;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UProgressBar> ProgressBar_Health;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_State;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UBorder> Border_Active;

    void InitializeInitiativeSlot (
        const FGridCombatHudInitiativeView& InView);

protected:
    virtual void NativeConstruct () override;

private:
    void EnsureHealthProgressBar ();
    void RefreshWidgets ();
};

/**
 * Event-driven MON12.8 combat HUD. It projects authoritative runtime state and
 * never calculates initiative, action costs, availability or end-turn rules.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridCombatHudWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridCombatHudView View;

    /** Live MON12.8.6 cell/area preview; no resource has been paid yet. */
    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD|Targeting")
    FGridCombatActionTargetingPreview TargetingPreview;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD|Targeting")
    bool bCombatActionTargetingActive = false;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<AGrimrockPartyPawn> PartyPawn;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UGridTurnManagerComponent> TurnManagerComponent;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|HUD|Classes")
    TSubclassOf<UGridCombatActionPanelWidget> PartyMemberPanelWidgetClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|HUD|Classes")
    TSubclassOf<UGridCombatHudActionWidget> ActionWidgetClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|HUD|Classes")
    TSubclassOf<UGridCombatHudInitiativeSlotWidget> InitiativeSlotWidgetClass;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|HUD|Initiative",
        meta = (ClampMin = "7", ClampMax = "10", UIMin = "7", UIMax = "10"))
    int32 VisibleInitiativeSlotCount = 8;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|HUD|Hotbar",
        meta = (ClampMin = "0.0", ClampMax = "16.0", UIMin = "0.0", UIMax = "16.0"))
    float HotbarSlotSpacing = 4.0f;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TArray<TObjectPtr<UGridCombatActionPanelWidget>> PartyMemberPanels;

    /** Fixed ten-slot pool; entries are refreshed instead of recreated. */
    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TArray<TObjectPtr<UGridCombatHudActionWidget>> HotbarActionWidgets;

    /** Variable-sized pool for Universal, Ability and Spell sources. */
    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TArray<TObjectPtr<UGridCombatHudActionWidget>> ActionPaletteWidgets;

    /** Native fallback used when the legacy WBP has no action palette panel. */
    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UPanelWidget> RuntimeActionPalettePanel;

    /**
     * Single runtime row that owns all ten hotbar widgets. When the designer
     * panel is already a HorizontalBox, this points directly to it.
     */
    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UHorizontalBox> HotbarRow;

    /** Fixed widget pool; entries are updated instead of recreated. */
    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TArray<TObjectPtr<UGridCombatHudInitiativeSlotWidget>>
        InitiativeSlotWidgets;

    /** Native separator pool inserted between slots without consuming one. */
    UPROPERTY (Transient)
    TArray<TObjectPtr<UBorder>> InitiativeRoundSeparatorWidgets;

    UPROPERTY (Transient)
    TArray<TObjectPtr<UTextBlock>> InitiativeRoundSeparatorTexts;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UPanelWidget> Panel_PartyMembers;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UPanelWidget> Panel_Actions;

    /** Optional empty WrapBox/HorizontalBox populated from class actions. */
    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UPanelWidget> Panel_ActionPalette;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UPanelWidget> Panel_Initiative;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_MobilityActionPoints;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UButton> Button_EndTurn;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_EndTurnDisabledReason;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UWidget> Panel_CombatHud;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD|Targeting")
    TObjectPtr<UWidget> Panel_Targeting;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD|Targeting")
    TObjectPtr<UTextBlock> Text_TargetingInstructions;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD|Targeting")
    TObjectPtr<UTextBlock> Text_TargetingCell;

    UFUNCTION (BlueprintCallable, Category = "Combat|HUD")
    void InitializeCombatHud (
        AGrimrockPartyPawn* InPartyPawn,
        UGridTurnManagerComponent* InTurnManager = nullptr);

    UFUNCTION (BlueprintCallable, Category = "Combat|HUD")
    void RefreshFromSources ();

    UFUNCTION (BlueprintCallable, Category = "Combat|HUD")
    bool RequestCombatAction (
        const FGridCombatHudActionView& ActionView,
        FGridCombatActionRequestResult& OutResult);

    /** Rebuilds and executes one of the ten fixed slots by its internal index. */
    UFUNCTION (BlueprintCallable, Category = "Combat|HUD|Hotbar")
    bool RequestHotbarSlot (
        int32 SlotIndex,
        FGridCombatActionRequestResult& OutResult);

    UFUNCTION (BlueprintPure, Category = "Combat|HUD|Targeting")
    bool IsCombatActionTargetingActive () const
    {
        return bCombatActionTargetingActive;
    }

    /** Begins targeting without executing or paying the selected action. */
    UFUNCTION (BlueprintCallable, Category = "Combat|HUD|Targeting")
    bool BeginCombatActionTargeting (
        const FGridCombatHudActionView& ActionView);

    /** Rebuilds the authoritative preview for the currently hovered cell. */
    UFUNCTION (BlueprintCallable, Category = "Combat|HUD|Targeting")
    bool UpdateCombatActionTargetingPreview (FIntPoint TargetCell);

    /** Executes at a valid target; invalid confirmation keeps targeting active. */
    UFUNCTION (BlueprintCallable, Category = "Combat|HUD|Targeting")
    bool ConfirmCombatActionTarget (
        FIntPoint TargetCell,
        FGridCombatActionRequestResult& OutResult);

    UFUNCTION (BlueprintCallable, Category = "Combat|HUD|Targeting")
    void ClearCombatActionTargetingPreview ();

    UFUNCTION (BlueprintCallable, Category = "Combat|HUD|Targeting")
    void CancelCombatActionTargeting ();

    UFUNCTION (BlueprintCallable, Category = "Combat|HUD")
    bool RequestEndTurn ();

    /** Routes inventory/equipment and hotbar drag payloads to persistence. */
    bool HandleHotbarDrop (
        int32 TargetSlotIndex,
        UDragDropOperation* DragOperation);

    /** Assigns a currently catalogued class/universal action to one slot. */
    UFUNCTION (BlueprintCallable, Category = "Combat|HUD|Hotbar")
    bool AssignCombatActionToHotbarSlot (
        int32 TargetSlotIndex,
        const FGridAvailableCombatAction& Action);

    /** Right-click contract: unassigns the slot without moving its source. */
    bool ClearHotbarSlot (int32 SlotIndex);

protected:
    virtual void NativeConstruct () override;
    virtual void NativeDestruct () override;

private:
    bool bSourcesBound = false;

    FGridCombatHudActionView PendingTargetingActionView;

    void BindToSources ();
    void UnbindFromSources ();
    void EnsurePartyMemberPanels ();
    void EnsureActionWidgets ();
    void RefreshActionWidgets ();
    void EnsureActionPalettePanel ();
    void EnsureActionPaletteWidgets ();
    void RefreshActionPaletteWidgets ();
    void ApplyHotbarPresentationFallbacks ();
    void EnsureInitiativeWidgets ();
    void RefreshInitiativeWidgets ();
    void RefreshBoundWidgets ();
    void ValidateCombatActionTargetingState ();
    void RefreshTargetingWidgets ();

    UFUNCTION ()
    void HandleEndTurnClicked ();

    UFUNCTION ()
    void HandleInventoryChanged (int32 CharacterIndex);

    UFUNCTION ()
    void HandlePhaseChanged (EGridCombatPhase NewPhase);

    UFUNCTION ()
    void HandleRoundStarted (int32 RoundNumber);

    UFUNCTION ()
    void HandlePlayerTurnStateChanged (FGridPlayerCharacterTurnState TurnState);

    UFUNCTION ()
    void HandlePartyMobilityChanged (FGridPartyMobilityState MobilityState);

    UFUNCTION ()
    void HandleTurnOrderChanged ();

    UFUNCTION ()
    void HandleActiveCombatantChanged (FGridCombatantInitiativeEntry Combatant);

    UFUNCTION ()
    void HandleCombatantStateChanged (FGridCombatantInitiativeEntry Combatant);

    UFUNCTION ()
    void HandleCombatEnded (EGridCombatPhase ResultPhase);
};
