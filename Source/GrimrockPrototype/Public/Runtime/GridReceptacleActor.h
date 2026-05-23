#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "GridReceptacleActor.generated.h"

class UStaticMeshComponent;
class AGrimrockPartyPawn;
class AGridItemActor;

/**
 * Generic runtime receptacle: torch holder, wall niche, pedestal, statue slot, altar, etc.
 */
UCLASS (Blueprintable)
class GRIMROCKPROTOTYPE_API AGridReceptacleActor : public AGridRuntimeObjectActor, public IGridInteractableInterface
{
    GENERATED_BODY ()

public:
    AGridReceptacleActor ();

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TObjectPtr<USceneComponent> ItemSocketRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TObjectPtr<USceneComponent> ItemAttachPoint;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TObjectPtr<UStaticMeshComponent> ContainedItemMesh;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bAcceptAnyItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    TArray<FName> AcceptedItemTags;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    TArray<FName> AcceptedArchetypeIds;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    TArray<FName> RejectedItemArchetypeIds;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    FName InitialContainedItemArchetypeId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    FName ContainedItemArchetypeId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TArray<FName> ContainedItemTags;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TObjectPtr<AGridItemActor> ContainedItemActor;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bCanInsertItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bCanRemoveItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bStartsFilled = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Receptacle")
    FName AcceptedItemId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Receptacle")
    FName ContainedItemId = NAME_None;

public:
    virtual void InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
        const FTransform& WorldTransform) override;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool HasItem () const;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool CanAcceptItem (FName ItemId) const;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool CanAcceptItemArchetype (FName ItemArchetypeId, const TArray<FName>& ItemTags) const;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryInsertItem (FName ItemId);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryInsertItemActor (AGridItemActor* ItemActor);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryRemoveItem (FName& OutRemovedItemId);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryTakeContainedItem (AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemId);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryInteractWithParty (AGrimrockPartyPawn* PartyPawn);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    void ConfigureContainedItemVisual (UStaticMesh* InMesh, UMaterialInterface* InMaterial);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    void SetInitialContainedItemActor (AGridItemActor* ItemActor);

    virtual bool CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;
    virtual void Interact_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;
    virtual EGridInteractionCursor GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const override;
    virtual FText GetInteractionText_Implementation (UPrimitiveComponent* HitComponent) const override;

protected:
    virtual void BeginPlay () override;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    void SetContainedItem (FName NewItemId);

    void ExecuteInsertionLinks ();
    void ExecuteRemovalLinks ();
    void AttachContainedItemActor ();
    void ClearContainedItemActor ();
    FString GetItemAcceptanceFailureReason (FName ItemArchetypeId, const TArray<FName>& ItemTags) const;

private:
    UPROPERTY (Transient)
    TObjectPtr<UStaticMesh> RuntimeContainedItemMesh = nullptr;

    UPROPERTY (Transient)
    TObjectPtr<UMaterialInterface> RuntimeContainedItemMaterial = nullptr;
};
