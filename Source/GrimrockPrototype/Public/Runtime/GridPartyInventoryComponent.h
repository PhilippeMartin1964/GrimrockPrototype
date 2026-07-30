#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridPartyInventoryComponent.generated.h"

class UGridItemDefinitionAsset;
class UTexture2D;

UCLASS (ClassGroup = (Grimrock), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridPartyInventoryComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridPartyInventoryComponent ();

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Party")
    FGridPartyInventoryState PartyInventoryState;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Inventory")
    int32 DefaultInventorySlotCountPerCharacter = 40;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Party")
    int32 DefaultMaxActiveCharacters = 6;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void InitializeDefaultPartyIfNeeded ();

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    bool HasCompletedInitialCharacterCreation () const;

    UFUNCTION (BlueprintCallable, Category = "RPG|Save")
    void ResetPartyForNewGame ();

    UFUNCTION (BlueprintCallable, Category = "RPG|Save")
    bool RestorePartyInventoryState (
        const FGridPartyInventoryState& SavedState,
        UPARAM (ref) FText& OutError);

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    bool CreateInitialCharacter (
        const FRPGCharacterCreationRequest& Request,
        UPARAM (ref) FText& OutError);

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    bool SetCharacterVisualSelection (
        int32 CharacterIndex,
        ERPGCharacterPortraitGender PortraitGender,
        FName PortraitVariantId,
        TSoftObjectPtr<UTexture2D> Portrait,
        TSoftObjectPtr<UTexture2D> ClassIcon);

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    bool GetCharacterVisualSelection (
        int32 CharacterIndex,
        FRPGCharacterVisualSelection& OutSelection) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetActiveCharacterCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetMaxActiveCharacters () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetMaxActiveCharacterCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetSelectedCharacterIndex () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool SetSelectedCharacterIndex (int32 NewIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool GetCharacterSummary (int32 CharacterIndex, FGridInventoryCharacterSummary& OutSummary) const;

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
    bool HasItemDefinitionInCharacterInventory (int32 CharacterIndex, FName ItemDefinitionId) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool HasItemDefinitionInSelectedCharacterInventory (FName ItemDefinitionId) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    int32 CountItemDefinitionInCharacterInventory (int32 CharacterIndex, FName ItemDefinitionId) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    int32 CountItemDefinitionInSelectedCharacterInventory (FName ItemDefinitionId) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool RemoveItemDefinitionFromCharacterInventory (int32 CharacterIndex, FName ItemDefinitionId, int32 Quantity);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool RemoveItemDefinitionFromSelectedCharacterInventory (FName ItemDefinitionId, int32 Quantity);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool RegisterItemDefinition (UGridItemDefinitionAsset* Definition);

    bool RehydrateOwnedItemDefinitions (
        TFunctionRef<UGridItemDefinitionAsset* (FName)> Resolver,
        FName& OutMissingDefinitionId);

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
    bool TryTakeEquipmentSlotToCursor (int32 CharacterIndex, EGridEquipmentSlot SourceSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool TryTakeSelectedCharacterEquipmentSlotToCursor (EGridEquipmentSlot SourceSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool GetEquippedItem (int32 CharacterIndex, EGridEquipmentSlot Slot, FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    bool IsEquipmentSlotOccupied (int32 CharacterIndex, EGridEquipmentSlot Slot) const;

    /**
     * Extracts exactly one equipped item for an atomic transfer to the world.
     * A stacked slot keeps its remaining quantity; the extracted unit receives
     * its own runtime identity.
     */
    bool TryExtractOneEquippedItemForWorldTransfer (
        int32 CharacterIndex,
        EGridEquipmentSlot SourceSlot,
        FName ExpectedItemDefinitionId,
        FGridItemInstance& OutWorldItem);

    /** Rolls back TryExtractOneEquippedItemForWorldTransfer after spawn failure. */
    bool TryRestoreExtractedItemToEquipment (
        int32 CharacterIndex,
        EGridEquipmentSlot TargetSlot,
        const FGridItemInstance& WorldItem);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    FGridEquipmentStatBonus ComputeCharacterEquipmentStatBonus (int32 CharacterIndex) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Equipment")
    FGridDamageResistanceSet ComputeCharacterEquipmentResistances (int32 CharacterIndex) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Diagnostics")
    FString GetEquipmentDiagnosticsForCharacter (int32 CharacterIndex) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool SetCursorItem (const FGridItemInstance& Item);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool ClearCursorItem ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool HasCursorItem () const;

    const FGridItemInstance& GetCursorItem () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryTakeInventorySlotToCursor (int32 CharacterIndex, int32 InventorySlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryTakeInventorySlotQuantityToCursor (int32 CharacterIndex, int32 InventorySlotIndex, int32 Quantity);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryPlaceCursorItemInCharacterInventorySlot (int32 CharacterIndex, int32 TargetSlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Items")
    bool TryMoveCharacterInventorySlot (int32 CharacterIndex, int32 SourceSlotIndex, int32 TargetSlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryPlaceCursorItemInCharacterInventory (int32 CharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryPlaceCursorItemInSelectedCharacterInventory ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryClearCursorToSelectedCharacterInventory ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryDropCursorItem ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool CanEquipCursorItemToCharacterSlot (int32 CharacterIndex, EGridEquipmentSlot TargetSlot) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryEquipCursorItemToCharacterSlot (int32 CharacterIndex, EGridEquipmentSlot TargetSlot);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Cursor")
    bool TryEquipCursorItemToSelectedCharacterSlot (EGridEquipmentSlot TargetSlot);

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

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Inventory|Diagnostics")
    void LogEquipmentCompatibilityDiagnostics () const;

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Inventory|Diagnostics")
    void LogSelectedCharacterEquipmentStatBonusDiagnostics () const;

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Inventory|Diagnostics")
    void LogSelectedCharacterResistanceDiagnostics () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Diagnostics")
    void LogInventoryOwnershipDiagnostics () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Diagnostics")
    bool ValidateInventoryOwnership (UPARAM (ref) FString& OutError) const;

private:
    UPROPERTY (Transient)
    TMap<FName, TObjectPtr<UGridItemDefinitionAsset>> RuntimeItemDefinitionsById;

    void EnsureEquipmentCountMatchesActiveCharacters ();
    void InitializeCharacterDefaults (FGridCharacterInventoryState& CharacterState, int32 CharacterIndex) const;
    float CalculateEquipmentWeight (const FGridCharacterEquipmentState& EquipmentState) const;
};
