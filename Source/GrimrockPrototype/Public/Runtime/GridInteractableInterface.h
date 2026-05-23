#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridInteractableInterface.generated.h"

class APawn;
class UPrimitiveComponent;

UENUM (BlueprintType)
enum class EGridInteractionCursor : uint8
{
    None,
    Default,
    Use,
    Push,
    Pull,
    Take,
    Read,
    Locked,
    Forbidden
};

UINTERFACE (BlueprintType)
class GRIMROCKPROTOTYPE_API UGridInteractableInterface : public UInterface
{
    GENERATED_BODY ()
};

class GRIMROCKPROTOTYPE_API IGridInteractableInterface
{
    GENERATED_BODY ()

public:
    UFUNCTION (BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
    bool CanInteract (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const;
    virtual bool CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
    {
        (void)InstigatorPawn;
        (void)HitComponent;
        return true;
    }

    UFUNCTION (BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
    void Interact (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent);
    virtual void Interact_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
    {
        (void)InstigatorPawn;
        (void)HitComponent;
    }

    UFUNCTION (BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
    EGridInteractionCursor GetInteractionCursor (UPrimitiveComponent* HitComponent) const;
    virtual EGridInteractionCursor GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const
    {
        (void)HitComponent;
        return EGridInteractionCursor::Default;
    }

    UFUNCTION (BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
    FText GetInteractionText (UPrimitiveComponent* HitComponent) const;
    virtual FText GetInteractionText_Implementation (UPrimitiveComponent* HitComponent) const
    {
        (void)HitComponent;
        return FText::GetEmpty ();
    }
};
