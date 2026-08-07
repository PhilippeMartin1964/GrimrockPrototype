#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridCombatHudWidget.generated.h"

class AGrimrockPartyPawn;
class UBorder;
class UButton;
class UGridCombatActionPanelWidget;
class UGridPartyInventoryComponent;
class UGridTurnManagerComponent;
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

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridCombatHudMobilityView Mobility;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    TArray<FGridCombatHudInitiativeView> Initiative;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    int32 InitiativeOverflowCount = 0;

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

    static void BuildActions (
        const TArray<FGridAvailableCombatAction>& AvailableActions,
        TArray<FGridCombatHudActionView>& OutActions);

    static FGridCombatHudMobilityView BuildMobility (
        const FGridPartyMobilityState& MobilityState);

    static void BuildInitiative (
        const TArray<FGridInitiativePreviewEntry>& InitiativePreview,
        TArray<FGridCombatHudInitiativeView>& OutInitiative,
        int32& OutOverflowCount,
        int32 MaximumVisibleEntries = MaximumVisibleInitiativeEntries);

    static float CalculateHealthPercent (
        int32 CurrentHealth,
        int32 MaximumHealth);
};

class UGridCombatHudWidget;

/** One generated action button. Its identity comes entirely from MON12.6. */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridCombatHudActionWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
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

    void InitializeAction (
        UGridCombatHudWidget* InOwnerHud,
        const FGridCombatHudActionView& InView);

protected:
    virtual void NativeConstruct () override;
    virtual void NativeDestruct () override;

private:
    UPROPERTY (Transient)
    TObjectPtr<UGridCombatHudWidget> OwnerHud;

    void RefreshWidgets ();

    UFUNCTION ()
    void HandleClicked ();
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
    float ActiveScale = 1.28f;

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
    TObjectPtr<UTextBlock> Text_Side;

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
 * Event-driven MON12.7 combat HUD. It projects authoritative runtime state and
 * never calculates initiative, action costs, availability or end-turn rules.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridCombatHudWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Combat|HUD")
    FGridCombatHudView View;

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

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|HUD")
    TArray<TObjectPtr<UGridCombatActionPanelWidget>> PartyMemberPanels;

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

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UPanelWidget> Panel_Initiative;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_MobilityActionPoints;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_InitiativeOverflow;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UButton> Button_EndTurn;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UTextBlock> Text_EndTurnDisabledReason;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|HUD")
    TObjectPtr<UWidget> Panel_CombatHud;

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

    UFUNCTION (BlueprintCallable, Category = "Combat|HUD")
    bool RequestEndTurn ();

protected:
    virtual void NativeConstruct () override;
    virtual void NativeDestruct () override;

private:
    bool bSourcesBound = false;

    void BindToSources ();
    void UnbindFromSources ();
    void EnsurePartyMemberPanels ();
    void RebuildActionWidgets ();
    void EnsureInitiativeWidgets ();
    void RefreshInitiativeWidgets ();
    void RefreshBoundWidgets ();

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
    void HandlePlayerAttackResolved (
        FGridPlayerAttackRequest Request,
        class AGridMonsterActor* TargetMonster,
        FGridAttackResult Result);

    UFUNCTION ()
    void HandlePlayerAttackRejected (
        int32 CharacterIndex,
        EGridPlayerAttackRejectReason RejectReason);

    UFUNCTION ()
    void HandleActionStarted (FGridCombatAction Action);

    UFUNCTION ()
    void HandleActionCompleted (FGridCombatAction Action, bool bSucceeded);

    UFUNCTION ()
    void HandleCombatEnded (EGridCombatPhase ResultPhase);
};
