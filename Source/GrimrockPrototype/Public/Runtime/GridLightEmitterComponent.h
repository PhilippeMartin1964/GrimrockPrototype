#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Runtime/GridLightEmitterTypes.h"
#include "GridLightEmitterComponent.generated.h"

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

	/** Hide a held item's PointLight while a party proxy renders the illumination.
	 * The authored item light state and independent VisualEffects are unchanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Light|Presentation")
	void SetPointLightPresentationEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Light|Presentation")
	bool IsPointLightPresentationEnabled() const { return bPointLightPresentationEnabled; }

	/** Runtime-only shadow policy used by presentation proxies such as party illumination. */
	UFUNCTION(BlueprintCallable, Category = "Light|Presentation")
	void SetPointLightCastShadows(bool bInCastShadows);

	UFUNCTION(BlueprintPure, Category = "Light|Presentation")
	bool GetPointLightCastShadows() const { return bPointLightCastShadows; }

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
	TObjectPtr<UPointLightComponent> PointLightComponent;

	bool bLightEnabled = false;
	bool bPointLightPresentationEnabled = true;
	bool bPointLightCastShadows = true;
	float FlickerPhase = 0.f;

	void RefreshTickState();
	void UpdatePointLightOutput();
	void UpdatePointLightFlickerPosition();
	void UpdatePointLightColor();
};
