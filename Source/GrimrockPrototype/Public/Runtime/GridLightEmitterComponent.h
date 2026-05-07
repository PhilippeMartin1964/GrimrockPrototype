#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GridLightEmitterComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;

UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridLightEmitterComponent : public USceneComponent
{
    GENERATED_BODY ()

public:
    UGridLightEmitterComponent ();

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light")
    TObjectPtr<UNiagaraSystem> NiagaraSystem;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light|Niagara")
    FVector NiagaraRelativeLocation = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light|Niagara")
    FRotator NiagaraRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light")
    bool bEnableOnBeginPlay = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light")
    bool bUsePointLight = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
    FVector PointLightRelativeLocation = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
    FRotator PointLightRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
    float LightIntensity = 850.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
    float LightRadius = 325.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
    FLinearColor LightColor = FLinearColor (1.f, 0.55f, 0.22f, 1.f);

    UFUNCTION (BlueprintCallable, Category = "Light")
    void SetLightEnabled (bool bEnabled);

    UFUNCTION (BlueprintCallable, Category = "Light")
    void RefreshEmitterTransforms ();

    UFUNCTION (BlueprintCallable, Category = "Light")
    bool IsLightEnabled () const;

protected:
    virtual void BeginPlay () override;

private:
    UPROPERTY (Transient)
    TObjectPtr<UNiagaraComponent> NiagaraComponent;

    UPROPERTY (Transient)
    TObjectPtr<UPointLightComponent> PointLightComponent;

    bool bLightEnabled = false;
};
