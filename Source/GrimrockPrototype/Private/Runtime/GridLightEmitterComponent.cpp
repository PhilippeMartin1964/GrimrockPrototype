#include "Runtime/GridLightEmitterComponent.h"

#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

UGridLightEmitterComponent::UGridLightEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UGridLightEmitterComponent::BeginPlay()
{
	Super::BeginPlay();

	FlickerPhase = FMath::FRandRange(0.f, 1000.f);
	SetLightEnabled(bEnableOnBeginPlay);
}

void UGridLightEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bLightEnabled && bUsePointLight && PointLightComponent)
	{
		if (bEnableLightFlicker)
		{
			UpdatePointLightOutput();
		}
		UpdatePointLightFlickerPosition();
		UpdatePointLightColor();
	}
}

void UGridLightEmitterComponent::SetLightEnabled(bool bEnabled)
{
	bLightEnabled = bEnabled;

	if (bEnabled && NiagaraSystem && !NiagaraComponent)
	{
		NiagaraComponent = NewObject<UNiagaraComponent>(GetOwner(), TEXT("GridItemNiagara"));
		if (NiagaraComponent)
		{
			NiagaraComponent->SetupAttachment(this);
			NiagaraComponent->SetAsset(NiagaraSystem);
			NiagaraComponent->RegisterComponent();
			RefreshEmitterTransforms();
		}
	}

	if (NiagaraComponent)
	{
		RefreshEmitterTransforms();
		NiagaraComponent->SetVisibility(bEnabled, true);
		if (bEnabled)
		{
			NiagaraComponent->Activate(true);
		}
		else
		{
			NiagaraComponent->Deactivate();
		}
	}

	if (bUsePointLight && !PointLightComponent)
	{
		PointLightComponent = NewObject<UPointLightComponent>(GetOwner(), TEXT("GridItemPointLight"));
		if (PointLightComponent)
		{
			PointLightComponent->SetupAttachment(this);
			PointLightComponent->bUseInverseSquaredFalloff = false;
			PointLightComponent->LightFalloffExponent = 4.f;
			PointLightComponent->RegisterComponent();
			RefreshEmitterTransforms();
		}
	}

	if (PointLightComponent)
	{
		RefreshEmitterTransforms();
		UpdatePointLightOutput();
		UpdatePointLightColor();
		PointLightComponent->SetVisibility(bEnabled && bUsePointLight);
	}
}

void UGridLightEmitterComponent::RefreshEmitterTransforms()
{
	if (NiagaraComponent)
	{
		NiagaraComponent->SetRelativeLocation(NiagaraRelativeLocation);
		NiagaraComponent->SetRelativeRotation(NiagaraRelativeRotation);
	}

	if (PointLightComponent)
	{
		PointLightComponent->SetRelativeLocation(PointLightRelativeLocation);
		PointLightComponent->SetRelativeRotation(PointLightRelativeRotation);
	}
}

float UGridLightEmitterComponent::GetEffectiveBaseIntensity() const
{
	return FMath::Max(0.f, BaseLightIntensity > 0.f ? BaseLightIntensity : LightIntensity);
}

float UGridLightEmitterComponent::GetEffectiveBaseRadius() const
{
	return FMath::Max(0.f, BaseAttenuationRadius > 0.f ? BaseAttenuationRadius : LightRadius);
}

FLinearColor UGridLightEmitterComponent::GetEffectiveBaseColor() const
{
	return BaseLightColor == FLinearColor::Black ? LightColor : BaseLightColor;
}

void UGridLightEmitterComponent::UpdatePointLightOutput()
{
	if (!PointLightComponent)
	{
		return;
	}

	const float BaseIntensity = GetEffectiveBaseIntensity();
	const float BaseRadius = GetEffectiveBaseRadius();

	float Intensity = BaseIntensity;
	float Radius = BaseRadius;

	if (bLightEnabled && bEnableLightFlicker)
	{
		const UWorld* World = GetWorld();
		const float TimeSeconds = World ? World->GetTimeSeconds() : 0.f;
		const float Time = TimeSeconds + FlickerPhase;

		const float PrimaryNoise = FMath::PerlinNoise1D(Time * FlickerSpeed);
		const float SecondaryWave = FMath::Sin(Time * FlickerSecondarySpeed) * 0.35f;
		const float SlowWave = FMath::Sin(Time * 1.37f) * 0.15f;
		const float Flicker = FMath::Clamp(PrimaryNoise + SecondaryWave + SlowWave, -1.f, 1.f);

		Intensity = BaseIntensity + (Flicker * FlickerIntensityAmount);
		Radius = BaseRadius + (Flicker * FlickerRadiusAmount);
	}

	PointLightComponent->SetIntensity(FMath::Max(0.f, Intensity));
	PointLightComponent->SetAttenuationRadius(FMath::Max(0.f, Radius));
}

void UGridLightEmitterComponent::UpdatePointLightFlickerPosition()
{
	if (!PointLightComponent)
	{
		return;
	}

	if (!bLightEnabled || !bEnableLightPositionFlicker)
	{
		PointLightComponent->SetRelativeLocation(PointLightRelativeLocation);
		return;
	}

	const UWorld* World = GetWorld();
	const float TimeSeconds = World ? World->GetTimeSeconds() : 0.f;
	const float Time = TimeSeconds + FlickerPhase;

	const float XNoise = FMath::PerlinNoise1D((Time * PositionFlickerSpeed) + 11.17f);
	const float YNoise = FMath::PerlinNoise1D((Time * (PositionFlickerSpeed * 0.83f)) + 37.91f);
	const float ZNoise = FMath::PerlinNoise1D((Time * (PositionFlickerSecondarySpeed * 0.55f)) + 73.43f);

	const float SoftX = XNoise + (FMath::Sin((Time * PositionFlickerSecondarySpeed) + 0.4f) * 0.2f);
	const float SoftY = YNoise + (FMath::Sin((Time * (PositionFlickerSecondarySpeed * 0.77f)) + 1.9f) * 0.2f);
	const float SoftZ = ZNoise + (FMath::Sin((Time * (PositionFlickerSpeed * 1.41f)) + 2.7f) * 0.15f);

	const FVector Offset(FMath::Clamp(SoftX, -1.f, 1.f) * PointLightFlickerPositionAmplitude.X,
		FMath::Clamp(SoftY, -1.f, 1.f) * PointLightFlickerPositionAmplitude.Y, FMath::Clamp(SoftZ, -1.f, 1.f) * PointLightFlickerPositionAmplitude.Z);

	PointLightComponent->SetRelativeLocation(PointLightRelativeLocation + Offset);
}

void UGridLightEmitterComponent::UpdatePointLightColor()
{
	if (!PointLightComponent)
	{
		return;
	}

	const FLinearColor EffectiveBaseColor = GetEffectiveBaseColor();

	if (!bLightEnabled || !bEnableLightColorFlicker)
	{
		PointLightComponent->SetLightColor(EffectiveBaseColor);
		return;
	}

	const UWorld* World = GetWorld();
	const float TimeSeconds = World ? World->GetTimeSeconds() : 0.f;
	const float Time = TimeSeconds + FlickerPhase;

	const float Noise = FMath::PerlinNoise1D((Time * ColorFlickerSpeed) + 149.37f);
	const float Wave = FMath::Sin((Time * (ColorFlickerSpeed * 1.7f)) + 0.8f) * 0.25f;
	const float FlickerAlpha = FMath::Clamp((Noise + Wave + 1.f) * 0.5f, 0.f, 1.f);

	const FLinearColor FlameColor = FLinearColor::LerpUsingHSV(FlickerWarmColor, FlickerHotColor, FlickerAlpha);
	const float BlendAmount = FMath::Clamp(ColorFlickerAmount, 0.f, 1.f);
	const FLinearColor ResultColor = FLinearColor::LerpUsingHSV(EffectiveBaseColor, FlameColor, BlendAmount);

	PointLightComponent->SetLightColor(ResultColor);
}

bool UGridLightEmitterComponent::IsLightEnabled() const
{
	return bLightEnabled;
}
