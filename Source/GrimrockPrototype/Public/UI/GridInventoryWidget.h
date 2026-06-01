#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridInventoryWidget.generated.h"

class AGrimrockPartyPawn;
class UGridPartyInventoryComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridInventoryWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

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

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleInventorySlotClicked (int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleMainHandClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleOffHandClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleCursorReturnToInventoryClicked ();
};
