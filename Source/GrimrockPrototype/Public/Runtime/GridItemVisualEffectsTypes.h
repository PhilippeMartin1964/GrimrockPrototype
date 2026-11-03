#pragma once

#include "CoreMinimal.h"
#include "GridItemVisualEffectsTypes.generated.h"

class UNiagaraSystem;

/** ITEM-VFX01.1: per-effect visibility policy. */
UENUM(BlueprintType)
enum class EGridItemVfxActivationMode : uint8
{
    Always UMETA(DisplayName = "Always (while visually present)"),
    WhenLightEnabled UMETA(DisplayName = "When item light is enabled")
};

/** Data-driven Niagara effect attached in the physical mesh's local coordinate space. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridItemNiagaraEffectConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    FName EffectId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    FTransform RelativeTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    EGridItemVfxActivationMode ActivationMode = EGridItemVfxActivationMode::Always;

    bool IsValid() const
    {
        return !NiagaraSystem.IsNull() && !RelativeTransform.ContainsNaN() &&
            (ActivationMode == EGridItemVfxActivationMode::Always ||
             ActivationMode == EGridItemVfxActivationMode::WhenLightEnabled);
    }
};

/** Item definition owns the visual effects; item instances do not serialize components. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridItemVisualEffectsConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
    TArray<FGridItemNiagaraEffectConfig> Effects;

    bool IsValid() const
    {
        TSet<FName> EffectIds;
        for (const FGridItemNiagaraEffectConfig& Effect : Effects)
        {
            if (!Effect.IsValid())
            {
                return false;
            }
            if (!Effect.EffectId.IsNone())
            {
                if (EffectIds.Contains(Effect.EffectId))
                {
                    return false;
                }
                EffectIds.Add(Effect.EffectId);
            }
        }
        return true;
    }
};
