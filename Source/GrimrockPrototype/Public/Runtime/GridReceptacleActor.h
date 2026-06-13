#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridReceptacleTypes.h"
#include "GridReceptacleActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class AGrimrockPartyPawn;
class AGridItemActor;
class UGridItemDefinitionAsset;
class UGridItemTransferService;

/**
 * Item initially contained in a receptacle.
 *
 * Used by TorchHolder, alcoves, niches, altars, offering bowls, chests, etc.
 * The official item identity is ItemDefinitionId / ItemDefinition.
 */
USTRUCT (BlueprintType)
struct FGridInitialReceptacleItem
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    TObjectPtr<UGridItemDefinitionAsset> ItemDefinition = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ItemDefinitionId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
    int32 Quantity = 1;
};

/**
 * Runtime item contained by a receptacle.
 *
 * Single source of truth for contained items.
 * No separate ContainedItemId / ContainedItemArchetypeId / ContainedItemActors.
 */
USTRUCT (BlueprintType)
struct FGridContainedReceptacleItem
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    FGuid RuntimeObjectId;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    FName ItemDefinitionId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    FName ItemArchetypeId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TObjectPtr<UGridItemDefinitionAsset> ItemDefinition = nullptr;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TObjectPtr<AGridItemActor> ItemActor = nullptr;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    bool bWasInitialItem = false;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    int32 Quantity = 1;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    float Weight = 0.0f;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    FText DisplayName;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    bool bLightsEnabled = true;
};

/**
 * Generic runtime receptacle:
 * torch holder, wall niche, alcove, pedestal, altar, offering bowl, chest, etc.
 *
 * Design:
 * - supports 0..N contained items;
 * - ItemDefinitionId is the official runtime identity;
 * - ItemActor is only an optional visual/runtime representation;
 * - initial content is resolved from item definition assets.
 */
UCLASS (Blueprintable)
class GRIMROCKPROTOTYPE_API AGridReceptacleActor : public AGridRuntimeObjectActor, public IGridInteractableInterface
{
    GENERATED_BODY ()

public:
    AGridReceptacleActor ();

public:
    // ============================================================
    // Components
    // ============================================================

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> ItemSocketRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> ItemAttachPoint;

    /**
     * Optional fallback static mesh preview for very simple receptacles.
     * Prefer spawned AGridItemActor visuals for real items such as torches.
     */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> ContainedItemMesh;

public:
    // ============================================================
    // Configuration
    // ============================================================

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Configuration")
    EGridReceptacleKind ReceptacleKind = EGridReceptacleKind::Presentation;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Configuration")
    EGridReceptacleStorageMode StorageMode = EGridReceptacleStorageMode::SingleSlot;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Configuration")
    EGridReceptacleVisualPlacementMode VisualPlacementMode = EGridReceptacleVisualPlacementMode::AttachedSocket;

    /**
     * Content behavior after insertion.
     * Legacy, AcceptAny and Filtered are retained for existing assets.
     */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Configuration")
    EGridReceptacleItemPolicy ItemPolicy = EGridReceptacleItemPolicy::Legacy;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bCanInsertItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bCanRemoveItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bAcceptAnyItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Visual")
    TSubclassOf<AGridItemActor> ContainedItemActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Visual")
    bool bSimulatePhysicsWhenPlaced = false;

    /**
     * Maximum number of items this receptacle can contain.
     *
     * 1  = torch holder / single-slot receptacle.
     * >1 = alcove / niche / altar / chest.
     * <=0 = unlimited capacity.
     */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    int32 MaxContainedItems = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle", meta = (ClampMin = "0.0"))
    float PhysicalPlacementSurfaceOffset = 10.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    FRotator PhysicalPlacementInitialRotationOffset = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle|Initial Items")
    TArray<FGridInitialReceptacleItem> InitialContainedItems;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle|Legacy")
    FName ContainedItemArchetypeId = NAME_None;

public:
    // ============================================================
    // Runtime State
    // ============================================================

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle|Runtime")
    TArray<FGridContainedReceptacleItem> ContainedItems;

public:
    // ============================================================
    // AGridRuntimeObjectActor
    // ============================================================
    virtual void InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform) override;

protected:
    virtual void BeginPlay () override;
    virtual void EndPlay (const EEndPlayReason::Type EndPlayReason) override;

public:
    // ============================================================
    // Queries
    // ============================================================

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    bool HasItem () const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    bool HasAnyItem () const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    int32 GetContainedItemCount () const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    bool IsFull () const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    bool IsEmpty () const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    bool IsValidContainedItemIndex (int32 ItemIndex) const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    FName GetContainedItemDefinitionId (int32 ItemIndex = 0) const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    AGridItemActor* GetContainedItemActor (int32 ItemIndex = 0) const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    bool ContainsItemDefinition (FName ItemDefinitionId) const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    bool ContainsItemTag (FName ItemTag) const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    bool ContainsItemType (EGridItemType ItemType) const;

    UFUNCTION (BlueprintPure, Category = "Receptacle")
    float GetContainedTotalWeight () const;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool CanAcceptItem (FName ItemDefinitionId) const;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool CanAcceptItemInstance (const FGridItemInstance& Item) const;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool EvaluateItemAcceptance (
        const FGridItemInstance& Item,
        FGridReceptacleAcceptanceResult& OutResult,
        bool bLogDiagnostics = false) const;

    UFUNCTION (BlueprintCallable, Category = "Grid|Receptacle")
    bool CanAcceptCursorItemFromParty (const AGrimrockPartyPawn* PartyPawn) const;

public:
    // ============================================================
    // Item Insertion / Removal
    // ============================================================

    /**
     * Inserts an item into the receptacle.
     *
     * This is the single official insertion path.
     * ItemDefinitionId is the source of truth.
     * ItemDefinition is optional but preferred when available.
     */
    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryInsertItem (FName ItemDefinitionId, UGridItemDefinitionAsset* ItemDefinition, AGrimrockPartyPawn* PartyPawn);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryInsertItemInstanceFromCursor (const FGridItemInstance& CursorItem, FGridItemInstance& OutAcceptedItem);

    /**
     * Takes the first contained item.
     *
     * Used by simple receptacles such as TorchHolder.
     */
    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryTakeFirstItem (AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemDefinitionId);

    /**
     * Takes a specific contained item.
     *
     * Used by alcoves / niches / multi-item receptacles.
     */
    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryTakeItemAtIndex (int32 ItemIndex, AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemDefinitionId);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool ConsumeItemAtIndex (int32 ItemIndex);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool ConsumeAllItems ();

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    void SetReceptacleItemPolicy (EGridReceptacleItemPolicy NewPolicy);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    void SetCanRemoveItem (bool bNewCanRemoveItem);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    void SetCanInsertItem (bool bNewCanInsertItem);

    /**
     * Main interaction entry point used by mouse / runtime interaction.
     *
     * Behavior:
     * - if party has a held item and insertion is possible -> insert it;
     * - otherwise, if receptacle contains item and removal is possible -> take item;
     * - otherwise do nothing.
     */
    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryInteractWithParty (AGrimrockPartyPawn* PartyPawn);

    bool TryPlaceCursorItemFromHit (AGrimrockPartyPawn* PartyPawn, const FHitResult& HitResult);

    void ApplyVisualPlacement (
        AGridItemActor* ItemActor,
        const FGridItemInstance& Item,
        const FHitResult* OptionalHit,
        int32 ItemIndex,
        bool bInitialPlacement);

public:
    // ============================================================
    // Runtime State Capture / Restore
    // ============================================================

    void CaptureRuntimeReceptacleState (FGridRuntimeReceptacleState& OutState) const;

    /**
     * Authoritative runtime clear:
     * destroys item actors and resets logical + visual state.
     */
    int32 ForceClearRuntimeContents (bool bMarkInitialItemsRemoved);

    /**
     * Restores one runtime-contained item.
     *
     * Runtime actor is responsible for resolving/spawning ItemActor if needed.
     */
    bool RestoreRuntimeContainedItem (const FGridRuntimeItemState& ItemState, AGridItemActor* ItemActor);

public:
    // ============================================================
    // Interaction Interface
    // ============================================================

    virtual bool CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;

    virtual void Interact_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;

    virtual void InteractWithHit_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult) override;

    virtual EGridInteractionCursor GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const override;

    virtual FText GetInteractionText_Implementation (UPrimitiveComponent* HitComponent) const override;

protected:
    // ============================================================
    // Internal Item Handling
    // ============================================================

    int32 AddContainedItem (
        FName ItemDefinitionId,
        UGridItemDefinitionAsset* ItemDefinition,
        AGridItemActor* ItemActor,
        bool bWasInitialItem,
        int32 Quantity,
        FGuid RuntimeObjectId = FGuid ());

    bool RemoveContainedItemAtIndex (int32 ItemIndex, FGridContainedReceptacleItem& OutRemovedItem);

    void ClearContainedActor (FGridContainedReceptacleItem& Item);

    void ClearAllContainedActors ();

    EGridReceptacleVisualPlacementMode GetEffectiveVisualPlacementMode () const;

    bool IsItemRemovalAllowed () const;

    void UpdateContainedItemInteractionCollision ();

    bool IsContainedItemHitComponent (UPrimitiveComponent* HitComponent) const;

    int32 FindContainedItemIndexForComponent (UPrimitiveComponent* HitComponent) const;

    FString GetItemAcceptanceFailureReason (FName ItemDefinitionId) const;

    void ExecuteInsertionLinks ();

    void ExecuteRemovalLinks ();

protected:
    // ============================================================
    // Initial Item Handling
    // ============================================================

    void InitializeInitialContainedItems ();

    FName ResolveInitialItemDefinitionId (const FGridInitialReceptacleItem& InitialItem) const;

    bool WasInitialItemRemoved (FName ItemDefinitionId) const;

private:
    friend class UGridItemTransferService;

    // ============================================================
    // Runtime Initial-Item Tracking
    // ============================================================

    UPROPERTY (Transient)
    TSet<FName> RemovedInitialItemDefinitionIds;

    UPROPERTY (Transient)
    bool bInitialItemsInitialized = false;

    UPROPERTY (Transient)
    TArray<FName> AcceptedItemDefinitionIds;

    TOptional<FHitResult> PendingPlacementHitResult;
};
