#pragma once

#include "CoreMinimal.h"
#include "GridLightEmitterTypes.generated.h"

class UNiagaraSystem;

/**
 * ITEM-LIGHT01: reusable authored presentation for any item that can emit light.
 * LIGHT-CONFIG02: LightIntensity/LightRadius/LightColor are the single point-light base authorities.
 * The item definition owns this data; UGridLightEmitterComponent only executes it at runtime.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridLightEmitterConfig
{
	GENERATED_BODY()

	/** Initial gameplay state copied into a newly created FGridItemInstance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	bool bDefaultEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Niagara")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Niagara")
	FVector NiagaraRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Niagara")
	FRotator NiagaraRelativeRotation = FRotator::ZeroRotator;

	/** Point-light-only emitters are valid even without a Niagara system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light")
	bool bUsePointLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
	FVector PointLightRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
	FRotator PointLightRelativeRotation = FRotator::ZeroRotator;

	/** Single authored base intensity. Flicker adds/subtracts FlickerIntensityAmount around this value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float LightIntensity = 850.f;

	/** Single authored base attenuation radius. Flicker adds/subtracts FlickerRadiusAmount around this value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float LightRadius = 325.f;

	/** Single authored base point-light color. Color flicker blends from this color toward the flame palette. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point Light", meta = (EditCondition = "bUsePointLight"))
	FLinearColor LightColor = FLinearColor(1.f, 0.55f, 0.22f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight"))
	bool bEnableLightFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float FlickerIntensityAmount = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float FlickerSpeed = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float FlickerSecondarySpeed = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float FlickerRadiusAmount = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight"))
	bool bEnableLightPositionFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight"))
	FVector PointLightFlickerPositionAmplitude = FVector(2.f, 2.f, 4.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float PositionFlickerSpeed = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float PositionFlickerSecondarySpeed = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight"))
	bool bEnableLightColorFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight"))
	FLinearColor FlickerWarmColor = FLinearColor(1.f, 0.42f, 0.12f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight"))
	FLinearColor FlickerHotColor = FLinearColor(1.f, 0.78f, 0.32f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0", ClampMax = "1.0"))
	float ColorFlickerAmount = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Flicker", meta = (EditCondition = "bUsePointLight", ClampMin = "0.0"))
	float ColorFlickerSpeed = 2.f;

	bool HasEmitter() const
	{
		return bUsePointLight || !NiagaraSystem.IsNull();
	}

	bool RequiresRuntimeTick() const
	{
		return bUsePointLight && (bEnableLightFlicker || bEnableLightPositionFlicker || bEnableLightColorFlicker);
	}

	bool IsValid() const
	{
		return FMath::IsFinite(LightIntensity) && LightIntensity >= 0.f && FMath::IsFinite(LightRadius) && LightRadius >= 0.f &&
			FMath::IsFinite(FlickerIntensityAmount) && FlickerIntensityAmount >= 0.f && FMath::IsFinite(FlickerSpeed) && FlickerSpeed >= 0.f &&
			FMath::IsFinite(FlickerSecondarySpeed) && FlickerSecondarySpeed >= 0.f && FMath::IsFinite(FlickerRadiusAmount) && FlickerRadiusAmount >= 0.f &&
			FMath::IsFinite(PositionFlickerSpeed) && PositionFlickerSpeed >= 0.f && FMath::IsFinite(PositionFlickerSecondarySpeed) && PositionFlickerSecondarySpeed >= 0.f &&
			FMath::IsFinite(ColorFlickerAmount) && ColorFlickerAmount >= 0.f && ColorFlickerAmount <= 1.f && FMath::IsFinite(ColorFlickerSpeed) &&
			ColorFlickerSpeed >= 0.f;
	}
};
