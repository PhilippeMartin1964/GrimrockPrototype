#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridReceptacleActor.h"
#include "GridWallLockActor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN (LogGridWallLock, Log, All);

UCLASS (Blueprintable)
class GRIMROCKPROTOTYPE_API AGridWallLockActor : public AGridReceptacleActor
{
    GENERATED_BODY ()

public:
    virtual void InitializeGridObject (
        const FGridLevelObjectData& ObjectData,
        UStaticMesh* Mesh,
        UMaterialInterface* Material,
        const FTransform& WorldTransform) override;

    bool TryInteractWithParty (AGrimrockPartyPawn* PartyPawn);

    virtual bool CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;
    virtual void Interact_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;
    virtual EGridInteractionCursor GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const override;
    virtual FText GetInteractionText_Implementation (UPrimitiveComponent* HitComponent) const override;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Lock|Runtime")
    bool bIsUnlocked = false;

private:
    bool bConsumeKeyOnUnlock = false;

    UPROPERTY (Transient)
    TArray<FName> AcceptedKeyDefinitionIds;

    FText LockedMessage;
    FText UnlockedMessage;
    FText MissingKeyMessage;

    FText GetEffectiveLockedMessage () const;
    FText GetEffectiveUnlockedMessage () const;
    FText GetEffectiveMissingKeyMessage () const;
    void ShowFeedback (AGrimrockPartyPawn* PartyPawn, const FText& Message) const;
};
