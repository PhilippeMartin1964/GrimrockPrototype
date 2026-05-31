#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridPartyInventoryComponent.generated.h"

class UGridItemDefinitionAsset;

UCLASS (ClassGroup = (Grimrock), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridPartyInventoryComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridPartyInventoryComponent ();

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Party")
    FGridPartyInventoryState PartyInventoryState;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Inventory")
    int32 DefaultInventorySlotCountPerCharacter = 24;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Party")
    int32 DefaultMaxActiveCharacters = 6;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Items")
    TArray<TObjectPtr<UGridItemDefinitionAsset>> ItemDefinitions;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void InitializeDefaultPartyIfNeeded ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetActiveCharacterCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetMaxActiveCharacters () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetSelectedCharacterIndex () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool SetSelectedCharacterIndex (int32 NewIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool IsValidCharacterIndex (int32 Index) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool CanAddItemToCharacterInventory (int32 CharacterIndex, const FGridItemInstance& Item) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool CanAddItemToSelectedCharacterInventory (const FGridItemInstance& Item) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool AddItemToCharacterInventory (int32 CharacterIndex, const FGridItemInstance& Item);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool AddItemToSelectedCharacterInventory (const FGridItemInstance& Item);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool RemoveItemFromCharacterInventoryByRuntimeId (int32 CharacterIndex, FGuid RuntimeObjectId, FGridItemInstance& OutRemovedItem);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool RemoveFirstItemFromCharacterInventoryByDefinitionId (int32 CharacterIndex, FName ItemDefinitionId, FGridItemInstance& OutRemovedItem);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool RemoveFirstItemFromSelectedCharacterInventoryByDefinitionId (FName ItemDefinitionId, FGridItemInstance& OutRemovedItem);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    UGridItemDefinitionAsset* FindItemDefinition (FName ItemDefinitionId) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool ApplyItemDefinitionToInstance (FGridItemInstance& ItemInstance) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool CanEquipItemToSlot (int32 CharacterIndex, const FGridItemInstance& Item, EGridEquipmentSlot TargetSlot) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool EquipItemFromInventorySlot (int32 CharacterIndex, int32 InventorySlotIndex, EGridEquipmentSlot TargetSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool UnequipItemToInventory (int32 CharacterIndex, EGridEquipmentSlot SourceSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool GetEquippedItem (int32 CharacterIndex, EGridEquipmentSlot Slot, FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool IsEquipmentSlotOccupied (int32 CharacterIndex, EGridEquipmentSlot Slot) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Diagnostics")
    FString GetEquipmentDiagnosticsForCharacter (int32 CharacterIndex) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool SetCursorItem (const FGridItemInstance& Item);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool ClearCursorItem ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool HasCursorItem () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Weight")
    void RecalculateCharacterWeight (int32 CharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Weight")
    void RecalculateAllWeights ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Diagnostics")
    FString GetPartyInventoryDiagnostics () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Diagnostics")
    void LogPartyInventoryDiagnostics () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Diagnostics")
    FString GetItemDefinitionDiagnostics () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Diagnostics")
    void LogItemDefinitionDiagnostics () const;

private:
    void EnsureEquipmentCountMatchesActiveCharacters ();
    void InitializeCharacterDefaults (FGridCharacterInventoryState& CharacterState, int32 CharacterIndex) const;
    float CalculateEquipmentWeight (const FGridCharacterEquipmentState& EquipmentState) const;
};
