#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GridLightEmitterComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;

UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridLightEmitterComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGridLightEmitterComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Niagara")
	FVector NiagaraRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Niagara")
	FRotator NiagaraRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	bool bEnableOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	bool bUsePointLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
	FVector PointLightRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
	FRotator PointLightRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float LightIntensity = 850.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float LightRadius = 325.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
	FLinearColor LightColor = FLinearColor(1.f, 0.55f, 0.22f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker")
	bool bEnableLightFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float BaseLightIntensity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float FlickerIntensityAmount = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float FlickerSpeed = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float FlickerSecondarySpeed = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float BaseAttenuationRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float FlickerRadiusAmount = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker")
	bool bEnableLightPositionFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker")
	FVector PointLightFlickerPositionAmplitude = FVector(2.f, 2.f, 4.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float PositionFlickerSpeed = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float PositionFlickerSecondarySpeed = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker")
	bool bEnableLightColorFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker")
	FLinearColor BaseLightColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker")
	FLinearColor FlickerWarmColor = FLinearColor(1.f, 0.42f, 0.12f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker")
	FLinearColor FlickerHotColor = FLinearColor(1.f, 0.78f, 0.32f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ColorFlickerAmount = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (ClampMin = "0.0"))
	float ColorFlickerSpeed = 2.f;

	UFUNCTION(BlueprintCallable, Category = "Light")
	void SetLightEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Light")
	void RefreshEmitterTransforms();

	UFUNCTION(BlueprintCallable, Category = "Light")
	bool IsLightEnabled() const;

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

	float GetEffectiveBaseIntensity() const;
	float GetEffectiveBaseRadius() const;
	FLinearColor GetEffectiveBaseColor() const;
	void UpdatePointLightOutput();
	void UpdatePointLightFlickerPosition();
	void UpdatePointLightColor();
};
