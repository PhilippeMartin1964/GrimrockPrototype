#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "GridReceptacleActor.generated.h"

class UStaticMeshComponent;
class AGrimrockPartyPawn;

/**
 * Generic runtime receptacle: torch holder, wall niche, pedestal, statue slot, altar, etc.
 *
 * Data convention for the first implementation:
 * - ObjectData.Type              = EGridLevelObjectType::Receptacle
 * - ObjectData.Tag               = AcceptedItemId, e.g. "Torch"
 * - ObjectData.bInitiallyActive  = starts filled
 * - ObjectData.bInitiallyEnabled = can exist/spawn
 * - ObjectData.Behavior later can override fine behavior when the scripting layer grows.
 */
UCLASS (Blueprintable)
class GRIMROCKPROTOTYPE_API AGridReceptacleActor : public AGridRuntimeObjectActor
{
    GENERATED_BODY ()

public:
    AGridReceptacleActor ();

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TObjectPtr<USceneComponent> ItemSocketRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    TObjectPtr<UStaticMeshComponent> ContainedItemMesh;

    /** Empty means: accepts any item. For a torch holder, use "Torch". */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    FName AcceptedItemId = TEXT ("Torch");

    /** Empty means no item is currently inserted. */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    FName ContainedItemId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bCanInsertItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bCanRemoveItem = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Receptacle")
    bool bStartsFilled = false;

public:
    virtual void InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
        const FTransform& WorldTransform) override;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool HasItem () const;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool CanAcceptItem (FName ItemId) const;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryInsertItem (FName ItemId);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryRemoveItem (FName& OutRemovedItemId);

    /** Convenience method for the current prototype: toggles Torch in/out. */
    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    bool TryToggleTorchForParty (AGrimrockPartyPawn* PartyPawn);

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    void ConfigureContainedItemVisual (UStaticMesh* InMesh, UMaterialInterface* InMaterial);

protected:
    virtual void BeginPlay () override;

    UFUNCTION (BlueprintCallable, Category = "Receptacle")
    void SetContainedItem (FName NewItemId);

    void ExecuteInsertionLinks ();
    void ExecuteRemovalLinks ();

private:
    UPROPERTY (Transient)
    TObjectPtr<UStaticMesh> RuntimeContainedItemMesh = nullptr;

    UPROPERTY (Transient)
    TObjectPtr<UMaterialInterface> RuntimeContainedItemMaterial = nullptr;
};
