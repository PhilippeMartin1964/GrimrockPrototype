#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Runtime/GridItemVisualEffectsTypes.h"
#include "GridItemVisualEffectsComponent.generated.h"

class UNiagaraComponent;

/**
 * ITEM-VFX01.1: no-tick executor for Niagara effects authored on item definitions.
 * This component must be attached to the item physics mesh.
 */
UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridItemVisualEffectsComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UGridItemVisualEffectsComponent();

    /** Reapply without duplicating components; remove effects no longer configured. */
    void ApplyConfig(const FGridItemVisualEffectsConfig& InConfig);

    /** Only WhenLightEnabled effects follow the item's runtime light state. */
    void SetItemLightState(bool bEnabled);

    /** Transient presentation mask, separate from gameplay light state. */
    void SetPresentationEnabled(bool bEnabled);

    int32 GetConfiguredEffectCount() const { return RuntimeConfig.Effects.Num(); }
    UNiagaraComponent* GetEffectComponent(int32 Index) const;

private:
    UPROPERTY(Transient)
    FGridItemVisualEffectsConfig RuntimeConfig;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UNiagaraComponent>> EffectComponents;

    bool bItemLightEnabled = false;
    bool bPresentationEnabled = true;

    void RefreshEffects();
};
