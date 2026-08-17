#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RPG/StatusEffects/GridStatusEffectPresentation.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridCombatActionPanelWidget.generated.h"

class AGrimrockPartyPawn;
class UBorder;
class UGridPartyInventoryComponent;
class UGridTurnManagerComponent;
class UImage;
class UTextBlock;
class UTexture2D;
class UWidget;

/**
 * Transient rendering snapshot. It is rebuilt from the live character,
 * inventory and turn manager whenever one of those sources emits an event.
 */
USTRUCT (BlueprintType)
struct FGridCombatActionPanelView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    bool bHasValidCharacter = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    FText DisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 CurrentHealth = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 MaxHealth = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 CurrentMana = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 MaxMana = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    EGridCombatantTurnState TurnState =
        EGridCombatantTurnState::Waiting;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 RemainingActionPoints = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 MaximumActionPoints = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    bool bCanAct = false;

    /** MON16.6 read-only projection of the authoritative runtime collection. */
    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI|Status Effects")
    TArray<FGridStatusEffectPresentationView> StatusEffects;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI|Status Effects")
    FText StatusSummary;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI|Status Effects")
    FText LatestStatusFeedback;
};

/**
 * MON12 combat action panel for one party member.
 * MON16.6 adds a native status summary/feedback fallback without requiring a
 * WBP change. Designer widgets named Text_StatusEffects/Text_StatusFeedback
 * are optional and take precedence when present.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridCombatActionPanelWidget
    : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 ConfiguredCharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    FGridCombatActionPanelView View;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<AGrimrockPartyPawn> PartyPawn = nullptr;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent = nullptr;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UGridTurnManagerComponent> TurnManagerComponent = nullptr;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|UI|Visuals",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DisabledOpacity = 0.45f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|UI|Visuals")
    FLinearColor ReadyColor = FLinearColor (0.20f, 0.80f, 0.25f, 1.0f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|UI|Visuals")
    FLinearColor WaitingColor =
        FLinearColor (0.75f, 0.60f, 0.15f, 1.0f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|UI|Visuals")
    FLinearColor AlreadyActedColor =
        FLinearColor (0.32f, 0.32f, 0.32f, 1.0f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|UI|Visuals")
    FLinearColor IncapacitatedColor =
        FLinearColor (0.80f, 0.35f, 0.10f, 1.0f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|UI|Visuals")
    FLinearColor DefeatedColor =
        FLinearColor (0.65f, 0.08f, 0.08f, 1.0f);

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UImage> Image_Portrait;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_Name;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_Health;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_Mana;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_ActionPoints;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_ActionState;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UBorder> Border_ActionState;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UWidget> Panel_DisabledOverlay;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI|Status Effects")
    TObjectPtr<UTextBlock> Text_StatusEffects;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI|Status Effects")
    TObjectPtr<UTextBlock> Text_StatusFeedback;

    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    void InitializeCombatActionPanel (
        AGrimrockPartyPawn* InPartyPawn,
        int32 InCharacterIndex,
        UGridTurnManagerComponent* InTurnManager = nullptr);

    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    void RefreshFromSources ();

private:
    FText GetActionStateText () const;
    void EnsureStatusWidgets ();
    void RefreshBoundWidgets ();
};
