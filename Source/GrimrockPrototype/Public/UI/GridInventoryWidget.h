#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridPartyMemberWidget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridInventoryWidget.generated.h"

class AGrimrockPartyPawn;
class UGridPartyInventoryComponent;
class UGridInventorySlotWidget;
class UGridPartyMemberWidget;

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridInventoryWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

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

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slots")
    void RefreshRegisteredSlotWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slots")
    void HandleRegisteredSlotClicked (EGridInventoryUiSlotType SlotType, int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Drag")
    bool HandleSlotDrop (
        EGridInventoryUiSlotType SourceType,
        int32 SourceIndex,
        EGridInventoryUiSlotType TargetType,
        int32 TargetIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleInventorySlotClicked (int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleMainHandClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleOffHandClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleCursorReturnToInventoryClicked ();
};
