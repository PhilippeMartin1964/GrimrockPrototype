#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Runtime/GridLightEmitterTypes.h"
#include "GridLightEmitterComponent.generated.h"

class UNiagaraComponent;
class UPointLightComponent;

/**
 * Runtime executor for FGridLightEmitterConfig.
 * Authored values live in the owning definition asset; this component owns only runtime state/components.
 */
UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridLightEmitterComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGridLightEmitterComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light")
	FGridLightEmitterConfig RuntimeConfig;

	UFUNCTION(BlueprintCallable, Category = "Light")
	void ApplyConfig(const FGridLightEmitterConfig& InConfig);

	UFUNCTION(BlueprintCallable, Category = "Light")
	void SetLightEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Light")
	void RefreshEmitterTransforms();

	UFUNCTION(BlueprintCallable, Category = "Light")
	bool IsLightEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Light")
	bool HasConfiguredEmitter() const { return RuntimeConfig.HasEmitter(); }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> PointLightComponent;

	bool bLightEnabled = false;
	float FlickerPhase = 0.f;

	void RefreshTickState();
	float GetEffectiveBaseIntensity() const;
	float GetEffectiveBaseRadius() const;
	FLinearColor GetEffectiveBaseColor() const;
	void UpdatePointLightOutput();
	void UpdatePointLightFlickerPosition();
	void UpdatePointLightColor();
};
