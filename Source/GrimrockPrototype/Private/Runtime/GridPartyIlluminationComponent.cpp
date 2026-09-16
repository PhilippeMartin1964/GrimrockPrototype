#include "Runtime/GridPartyIlluminationComponent.h"

UGridPartyIlluminationComponent::UGridPartyIlluminationComponent()
{
	SetEmitterChannelsEnabled(false, true);
	SetPointLightCastShadows(false);
}

void UGridPartyIlluminationComponent::BeginPlay()
{
	Super::BeginPlay();
	SetEmitterChannelsEnabled(false, true);
	SetPointLightCastShadows(bCastShadows);
}

FGridLightEmitterConfig UGridPartyIlluminationComponent::BuildPartyConfig(const FGridLightEmitterConfig& SourceConfig) const
{
	FGridLightEmitterConfig PartyConfig = SourceConfig;

	// The item's flame remains on the held item. The party proxy owns only the
	// world illumination and therefore never spawns the source Niagara system.
	PartyConfig.NiagaraSystem.Reset();
	PartyConfig.NiagaraRelativeLocation = FVector::ZeroVector;
	PartyConfig.NiagaraRelativeRotation = FRotator::ZeroRotator;

	// Placement is controlled by this component's transform under the camera,
	// not by the source item's physical point-light offset.
	PartyConfig.PointLightRelativeLocation = FVector::ZeroVector;
	PartyConfig.PointLightRelativeRotation = FRotator::ZeroRotator;
	PartyConfig.bDefaultEnabled = false;

	const float SafeIntensityMultiplier = FMath::Max(0.0f, IntensityMultiplier);
	const float SafeRadiusMultiplier = FMath::Max(0.0f, RadiusMultiplier);

	PartyConfig.LightIntensity *= SafeIntensityMultiplier;
	PartyConfig.BaseLightIntensity *= SafeIntensityMultiplier;
	PartyConfig.FlickerIntensityAmount *= SafeIntensityMultiplier;

	PartyConfig.LightRadius *= SafeRadiusMultiplier;
	PartyConfig.BaseAttenuationRadius *= SafeRadiusMultiplier;
	PartyConfig.FlickerRadiusAmount *= SafeRadiusMultiplier;

	return PartyConfig;
}

void UGridPartyIlluminationComponent::ApplyIlluminationSource(const FGridLightEmitterConfig& SourceConfig, FName SourceId)
{
	if (SourceId.IsNone() || !SourceConfig.bUsePointLight)
	{
		ClearIlluminationSource();
		return;
	}

	ActiveSourceId = SourceId;
	SetEmitterChannelsEnabled(false, true);
	SetPointLightCastShadows(bCastShadows);
	ApplyConfig(BuildPartyConfig(SourceConfig));
	SetLightEnabled(true);
}

void UGridPartyIlluminationComponent::ClearIlluminationSource()
{
	ActiveSourceId = NAME_None;
	SetLightEnabled(false);
	ApplyConfig(FGridLightEmitterConfig());
	SetEmitterChannelsEnabled(false, true);
	SetPointLightCastShadows(bCastShadows);
}
