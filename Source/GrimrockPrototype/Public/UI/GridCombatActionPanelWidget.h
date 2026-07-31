#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridCombatActionPanelWidget.generated.h"

class AGridMonsterActor;
class AGrimrockPartyPawn;
class UBorder;
class UButton;
class UGridPartyInventoryComponent;
class UGridTurnManagerComponent;
class UImage;
class UTextBlock;
class UTexture2D;
class UWidget;

UENUM (BlueprintType)
enum class EGridCombatActionPanelState : uint8
{
    Ready,
    AlreadyActed
};

/**
 * Read-only presentation of one equipped hand.
 * The inventory component remains the only owner of the real item instance.
 */
USTRUCT (BlueprintType)
struct FGridCombatActionSlotView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    bool bOccupied = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    FName ItemDefinitionId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    FText DisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    int32 Quantity = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    bool bShowQuantity = false;

    /** True for an offensive equipped item or an empty, unarmed hand. */
    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    bool bCanAttack = false;
};

/**
 * Transient rendering snapshot. It is rebuilt from the live character,
 * equipment and turn manager whenever one of those sources emits an event.
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
    FGridCombatActionSlotView MainHand;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    FGridCombatActionSlotView OffHand;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    EGridCombatActionPanelState ActionState =
        EGridCombatActionPanelState::Ready;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    bool bCombatActive = false;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    bool bCanAct = false;
};

/**
 * MON12 combat action panel for one party member.
 *
 * Passing INDEX_NONE as the member index makes the panel follow the selected
 * character. Passing an explicit index prepares the same class for MON12.3.
 * MON12.2 routes hand clicks through the authoritative turn manager.
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
    bool bFollowSelectedCharacter = true;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|UI")
    FGridCombatActionPanelView View;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<AGrimrockPartyPawn> PartyPawn = nullptr;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent = nullptr;

    UPROPERTY (Transient, BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UGridTurnManagerComponent> TurnManagerComponent = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|UI|Visuals")
    bool bCollapseOutsideCombat = true;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|UI|Visuals",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DisabledOpacity = 0.45f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|UI|Visuals")
    FLinearColor ReadyColor = FLinearColor (0.20f, 0.80f, 0.25f, 1.0f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|UI|Visuals")
    FLinearColor AlreadyActedColor =
        FLinearColor (0.32f, 0.32f, 0.32f, 1.0f);

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UImage> Image_Portrait;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_Name;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_Health;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_Mana;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UButton> Button_MainHand;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UImage> Image_MainHandIcon;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_MainHandQuantity;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UButton> Button_OffHand;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UImage> Image_OffHandIcon;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_OffHandQuantity;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UTextBlock> Text_ActionState;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UBorder> Border_ActionState;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
    TObjectPtr<UWidget> Panel_DisabledOverlay;

    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    void InitializeCombatActionPanel (
        AGrimrockPartyPawn* InPartyPawn,
        int32 InCharacterIndex = -1,
        UGridTurnManagerComponent* InTurnManager = nullptr);

    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    void SetCharacterIndex (int32 InCharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    void RefreshFromSources ();

    /** Requests one attack from the clicked hand without resolving rules here. */
    UFUNCTION (BlueprintCallable, Category = "Combat|UI")
    bool RequestAttackFromSlot (EGridEquipmentSlot EquipmentSlot);

    UFUNCTION (BlueprintPure, Category = "Combat|UI")
    FText GetActionStateText () const;

protected:
    virtual void NativeConstruct () override;
    virtual void NativeDestruct () override;

private:
    bool bSourcesBound = false;

    int32 ResolveCharacterIndex () const;
    void BindToSources ();
    void UnbindFromSources ();
    void RefreshBoundWidgets ();
    FGridCombatActionSlotView BuildSlotView (
        EGridEquipmentSlot EquipmentSlot) const;
    void ApplySlotVisual (
        const FGridCombatActionSlotView& SlotView,
        UButton* ButtonWidget,
        UImage* IconWidget,
        UTextBlock* QuantityWidget) const;

    UFUNCTION ()
    void HandleMainHandClicked ();

    UFUNCTION ()
    void HandleOffHandClicked ();

    UFUNCTION ()
    void HandleInventoryChanged (int32 ChangedCharacterIndex);

    UFUNCTION ()
    void HandlePhaseChanged (EGridCombatPhase NewPhase);

    UFUNCTION ()
    void HandleRoundStarted (int32 RoundNumber);

    UFUNCTION ()
    void HandlePlayerAttackRequested (FGridPlayerAttackRequest Request);

    UFUNCTION ()
    void HandlePlayerAttackResolved (
        FGridPlayerAttackRequest Request,
        AGridMonsterActor* TargetMonster,
        FGridAttackResult Result);

    UFUNCTION ()
    void HandleMonsterAttackResolved (
        AGridMonsterActor* Monster,
        int32 TargetCharacterIndex,
        FGridAttackResult Result);

    UFUNCTION ()
    void HandleCombatEnded (EGridCombatPhase ResultPhase);
};
