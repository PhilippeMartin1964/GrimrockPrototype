#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Magic/GridSpellPresentation.h"
#include "TimerManager.h"
#include "GridSpellPresentationComponent.generated.h"

class AGridCombatProjectileActor;
class UNiagaraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridSpellPresentationRequestedSignature,
    FGridSpellPresentationRequest, Request);

/**
 * Presentation-only runtime component for accepted/resolved spell casts.
 * It deliberately has no API capable of mutating gameplay resources or effects.
 */
UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridSpellPresentationComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridSpellPresentationComponent ();
    virtual void EndPlay (const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation")
    bool bAudioEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation")
    bool bVFXEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Spell|Presentation")
    bool bProjectileEnabled = true;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    FGridSpellPresentationRequest LastPresentationRequest;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Spell|Presentation")
    int32 PresentationRequestCount = 0;

    UPROPERTY (BlueprintAssignable, Category = "Spell|Presentation")
    FGridSpellPresentationRequestedSignature OnPresentationRequested;

    UFUNCTION (BlueprintCallable, Category = "Spell|Presentation")
    bool PresentSpell (
        const FGridSpellPresentationPlan& Plan,
        const FGridSpellPresentationProfile& Profile);

    UFUNCTION (BlueprintCallable, Category = "Spell|Presentation")
    void ResetPresentation ();

private:
    FGridSpellPresentationPlan ActivePlan;
    FGridSpellPresentationProfile ActiveProfile;
    FTimerHandle ImpactTimerHandle;
    TWeakObjectPtr<AGridCombatProjectileActor> ActiveProjectile;
    TArray<TWeakObjectPtr<UNiagaraComponent>> ActiveNiagaraComponents;
    int32 NextSequenceNumber = 1;

    void EmitEvent (EGridSpellPresentationEvent Event);
    void HandleImpact ();
    void StopActivePresentation ();
};
