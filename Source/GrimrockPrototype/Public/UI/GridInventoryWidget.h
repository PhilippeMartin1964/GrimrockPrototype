#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemActionTypes.h"
#include "UI/GridPartyMemberWidget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridInventoryWidget.generated.h"

class AGrimrockPartyPawn;
class UGridPartyInventoryComponent;
class UGridInventorySlotWidget;
class UGridPartyMemberWidget;
class UUniformGridPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams (
    FOnGridInventoryContextActionsRequested,
    EGridInventoryUiSlotType, SlotType,
    int32, SlotIndex);

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridInventoryWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots")
    TSubclassOf<UGridInventorySlotWidget> InventorySlotWidgetClass;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots", meta = (ClampMin = "1"))
    int32 InventorySlotColumnCount = 6;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots", meta = (ClampMin = "0"))
    int32 InventorySlotCountOverride = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory UI|Slots")
    TArray<TObjectPtr<UGridInventorySlotWidget>> GeneratedInventorySlotWidgets;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Slots")
    TObjectPtr<UUniformGridPanel> InventorySlotsGridPanel;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slots")
    TArray<TObjectPtr<UGridInventorySlotWidget>> RegisteredInventorySlots;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slots")
    TObjectPtr<UGridInventorySlotWidget> MainHandSlotWidget;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slots")
    TObjectPtr<UGridInventorySlotWidget> OffHandSlotWidget;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slots")
    TObjectPtr<UGridInventorySlotWidget> CursorSlotWidget;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Party")
    TArray<TObjectPtr<UGridPartyMemberWidget>> RegisteredPartyMemberWidgets;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Context Actions")
    FGridItemInstance LastContextItem;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Context Actions")
    FGridFacingTargetContext LastFacingTargetContext;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Context Actions")
    TArray<FGridItemContextAction> LastContextActions;

    UPROPERTY (BlueprintAssignable, Category = "Inventory|Context Actions")
    FOnGridInventoryContextActionsRequested OnContextActionsRequested;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    void InitializeInventoryWidget (AGrimrockPartyPawn* InPartyPawn);

    UFUNCTION (BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
    void RefreshInventory ();

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    int32 GetSelectedCharacterIndex () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    int32 GetInventorySlotCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool GetInventoryItemAtSlot (int32 SlotIndex, FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool GetMainHandItem (FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool GetOffHandItem (FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool GetCursorItem (FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool HasCursorItem () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetItemDisplayString (const FGridItemInstance& Item) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetCursorItemDisplayText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetMainHandDisplayText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetOffHandDisplayText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetInventorySlotDisplayText (int32 SlotIndex) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetActiveCharacterCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetMaxActiveCharacterCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool GetCharacterSummary (int32 CharacterIndex, FGridInventoryCharacterSummary& OutSummary) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool SelectCharacter (int32 CharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    FString GetCharacterDisplayText (int32 CharacterIndex) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    FString GetSelectedCharacterDisplayText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void RegisterPartyMemberWidget (UGridPartyMemberWidget* MemberWidget, int32 CharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void RefreshRegisteredPartyMemberWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void HandleRegisteredPartyMemberClicked (int32 CharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slots")
    void RegisterInventorySlotWidget (UGridInventorySlotWidget* SlotWidget, EGridInventoryUiSlotType SlotType, int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    void RebuildInventorySlotWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    void ClearGeneratedInventorySlotWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    int32 ResolveInventorySlotWidgetCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    void SetInventorySlotWidgetClass (TSubclassOf<UGridInventorySlotWidget> InClass);

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    void SetInventorySlotsGridPanel (UUniformGridPanel* InGridPanel);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slots")
    void RefreshRegisteredSlotWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slots")
    void HandleRegisteredSlotClicked (EGridInventoryUiSlotType SlotType, int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Context Actions")
    bool HandleItemSlotRightClicked (EGridInventoryUiSlotType SlotType, int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Context Actions")
    bool BuildContextActionsForSlot (
        EGridInventoryUiSlotType SlotType,
        int32 SlotIndex,
        FGridFacingTargetContext& OutFacingTarget,
        TArray<FGridItemContextAction>& OutActions);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Drag")
    bool HandleSlotDrop (
        EGridInventoryUiSlotType SourceType,
        int32 SourceIndex,
        EGridInventoryUiSlotType TargetType,
        int32 TargetIndex,
        bool bSplitStack = false,
        int32 RequestedQuantity = 0);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleInventorySlotClicked (int32 SlotIndex, bool bSplitStack = false);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleMainHandClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleOffHandClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleCursorReturnToInventoryClicked ();

protected:
    virtual void NativeConstruct () override;

private:
    void RemoveGeneratedInventorySlotsFromRegistry ();

    UPROPERTY (Transient)
    bool bInventorySlotsBuilt = false;

    UPROPERTY (Transient)
    int32 LastBuiltSlotCount = 0;

    UPROPERTY (Transient)
    int32 LastBuiltColumnCount = 0;

    UPROPERTY (Transient)
    TSubclassOf<UGridInventorySlotWidget> LastBuiltSlotWidgetClass;

    UPROPERTY (Transient)
    TObjectPtr<UUniformGridPanel> LastBuiltGridPanel;
};
