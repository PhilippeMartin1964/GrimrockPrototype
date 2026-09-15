#include "Runtime/GridLightEmitterComponent.h"

#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

UGridLightEmitterComponent::UGridLightEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGridLightEmitterComponent::BeginPlay()
{
	Super::BeginPlay();
	FlickerPhase = FMath::FRandRange(0.f, 1000.f);
	RefreshTickState();
}

void UGridLightEmitterComponent::ApplyConfig(const FGridLightEmitterConfig& InConfig)
{
	const bool bRestoreEnabled = bLightEnabled;
	SetLightEnabled(false);
	RuntimeConfig = InConfig;

	if (NiagaraComponent)
	{
		NiagaraComponent->SetAsset(RuntimeConfig.NiagaraSystem.IsNull() ? nullptr : RuntimeConfig.NiagaraSystem.LoadSynchronous());
	}

	RefreshEmitterTransforms();
	if (bRestoreEnabled)
	{
		SetLightEnabled(true);
	}
	else
	{
		RefreshTickState();
	}
}

void UGridLightEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bLightEnabled && RuntimeConfig.bUsePointLight && PointLightComponent)
	{
		if (RuntimeConfig.bEnableLightFlicker)
		{
			UpdatePointLightOutput();
		}
		UpdatePointLightFlickerPosition();
		UpdatePointLightColor();
	}
}

void UGridLightEmitterComponent::SetLightEnabled(bool bEnabled)
{
	bLightEnabled = bEnabled && RuntimeConfig.HasEmitter();

	if (bLightEnabled && !RuntimeConfig.NiagaraSystem.IsNull())
	{
		UNiagaraSystem* DesiredSystem = RuntimeConfig.NiagaraSystem.LoadSynchronous();
		if (DesiredSystem && !NiagaraComponent)
		{
			NiagaraComponent = NewObject<UNiagaraComponent>(GetOwner(), TEXT("GridItemNiagara"));
			if (NiagaraComponent)
			{
				NiagaraComponent->SetupAttachment(this);
				NiagaraComponent->SetAsset(DesiredSystem);
				NiagaraComponent->RegisterComponent();
			}
		}
		else if (NiagaraComponent && NiagaraComponent->GetAsset() != DesiredSystem)
		{
			NiagaraComponent->SetAsset(DesiredSystem);
		}
	}

	if (NiagaraComponent)
	{
		RefreshEmitterTransforms();
		const bool bShowNiagara = bLightEnabled && !RuntimeConfig.NiagaraSystem.IsNull();
		NiagaraComponent->SetVisibility(bShowNiagara, true);
		if (bShowNiagara)
		{
			NiagaraComponent->Activate(true);
		}
		else
		{
			NiagaraComponent->Deactivate();
		}
	}

	if (bLightEnabled && RuntimeConfig.bUsePointLight && !PointLightComponent)
	{
		PointLightComponent = NewObject<UPointLightComponent>(GetOwner(), TEXT("GridItemPointLight"));
		if (PointLightComponent)
		{
			PointLightComponent->SetupAttachment(this);
			PointLightComponent->bUseInverseSquaredFalloff = false;
			PointLightComponent->LightFalloffExponent = 4.f;
			PointLightComponent->RegisterComponent();
		}
	}

	if (PointLightComponent)
	{
		RefreshEmitterTransforms();
		UpdatePointLightOutput();
		UpdatePointLightColor();
		PointLightComponent->SetVisibility(bLightEnabled && RuntimeConfig.bUsePointLight);
	}

	RefreshTickState();
}

void UGridLightEmitterComponent::RefreshEmitterTransforms()
{
	if (NiagaraComponent)
	{
		NiagaraComponent->SetRelativeLocation(RuntimeConfig.NiagaraRelativeLocation);
		NiagaraComponent->SetRelativeRotation(RuntimeConfig.NiagaraRelativeRotation);
	}

	if (PointLightComponent)
	{
		PointLightComponent->SetRelativeLocation(RuntimeConfig.PointLightRelativeLocation);
		PointLightComponent->SetRelativeRotation(RuntimeConfig.PointLightRelativeRotation);
	}
}

void UGridLightEmitterComponent::RefreshTickState()
{
	SetComponentTickEnabled(bLightEnabled && RuntimeConfig.RequiresRuntimeTick());
}

float UGridLightEmitterComponent::GetEffectiveBaseIntensity() const
{
	return FMath::Max(0.f, RuntimeConfig.BaseLightIntensity > 0.f ? RuntimeConfig.BaseLightIntensity : RuntimeConfig.LightIntensity);
}

float UGridLightEmitterComponent::GetEffectiveBaseRadius() const
{
	return FMath::Max(0.f, RuntimeConfig.BaseAttenuationRadius > 0.f ? RuntimeConfig.BaseAttenuationRadius : RuntimeConfig.LightRadius);
}

FLinearColor UGridLightEmitterComponent::GetEffectiveBaseColor() const
{
	return RuntimeConfig.BaseLightColor == FLinearColor::Black ? RuntimeConfig.LightColor : RuntimeConfig.BaseLightColor;
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

	if (bLightEnabled && RuntimeConfig.bEnableLightFlicker)
	{
		const UWorld* World = GetWorld();
		const float TimeSeconds = World ? World->GetTimeSeconds() : 0.f;
		const float Time = TimeSeconds + FlickerPhase;

		const float PrimaryNoise = FMath::PerlinNoise1D(Time * RuntimeConfig.FlickerSpeed);
		const float SecondaryWave = FMath::Sin(Time * RuntimeConfig.FlickerSecondarySpeed) * 0.35f;
		const float SlowWave = FMath::Sin(Time * 1.37f) * 0.15f;
		const float Flicker = FMath::Clamp(PrimaryNoise + SecondaryWave + SlowWave, -1.f, 1.f);

		Intensity = BaseIntensity + (Flicker * RuntimeConfig.FlickerIntensityAmount);
		Radius = BaseRadius + (Flicker * RuntimeConfig.FlickerRadiusAmount);
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

	if (!bLightEnabled || !RuntimeConfig.bEnableLightPositionFlicker)
	{
		PointLightComponent->SetRelativeLocation(RuntimeConfig.PointLightRelativeLocation);
		return;
	}

	const UWorld* World = GetWorld();
	const float TimeSeconds = World ? World->GetTimeSeconds() : 0.f;
	const float Time = TimeSeconds + FlickerPhase;

	const float XNoise = FMath::PerlinNoise1D((Time * RuntimeConfig.PositionFlickerSpeed) + 11.17f);
	const float YNoise = FMath::PerlinNoise1D((Time * (RuntimeConfig.PositionFlickerSpeed * 0.83f)) + 37.91f);
	const float ZNoise = FMath::PerlinNoise1D((Time * (RuntimeConfig.PositionFlickerSecondarySpeed * 0.55f)) + 73.43f);

	const float SoftX = XNoise + (FMath::Sin((Time * RuntimeConfig.PositionFlickerSecondarySpeed) + 0.4f) * 0.2f);
	const float SoftY = YNoise + (FMath::Sin((Time * (RuntimeConfig.PositionFlickerSecondarySpeed * 0.77f)) + 1.9f) * 0.2f);
	const float SoftZ = ZNoise + (FMath::Sin((Time * (RuntimeConfig.PositionFlickerSpeed * 1.41f)) + 2.7f) * 0.15f);

	const FVector Offset(
		FMath::Clamp(SoftX, -1.f, 1.f) * RuntimeConfig.PointLightFlickerPositionAmplitude.X,
		FMath::Clamp(SoftY, -1.f, 1.f) * RuntimeConfig.PointLightFlickerPositionAmplitude.Y,
		FMath::Clamp(SoftZ, -1.f, 1.f) * RuntimeConfig.PointLightFlickerPositionAmplitude.Z);

	PointLightComponent->SetRelativeLocation(RuntimeConfig.PointLightRelativeLocation + Offset);
}

void UGridLightEmitterComponent::UpdatePointLightColor()
{
	if (!PointLightComponent)
	{
		return;
	}

	const FLinearColor EffectiveBaseColor = GetEffectiveBaseColor();

	if (!bLightEnabled || !RuntimeConfig.bEnableLightColorFlicker)
	{
		PointLightComponent->SetLightColor(EffectiveBaseColor);
		return;
	}

	const UWorld* World = GetWorld();
	const float TimeSeconds = World ? World->GetTimeSeconds() : 0.f;
	const float Time = TimeSeconds + FlickerPhase;

	const float Noise = FMath::PerlinNoise1D((Time * RuntimeConfig.ColorFlickerSpeed) + 149.37f);
	const float Wave = FMath::Sin((Time * (RuntimeConfig.ColorFlickerSpeed * 1.7f)) + 0.8f) * 0.25f;
	const float FlickerAlpha = FMath::Clamp((Noise + Wave + 1.f) * 0.5f, 0.f, 1.f);

	const FLinearColor FlameColor = FLinearColor::LerpUsingHSV(RuntimeConfig.FlickerWarmColor, RuntimeConfig.FlickerHotColor, FlickerAlpha);
	const float BlendAmount = FMath::Clamp(RuntimeConfig.ColorFlickerAmount, 0.f, 1.f);
	const FLinearColor ResultColor = FLinearColor::LerpUsingHSV(EffectiveBaseColor, FlameColor, BlendAmount);

	PointLightComponent->SetLightColor(ResultColor);
}

bool UGridLightEmitterComponent::IsLightEnabled() const
{
	return bLightEnabled;
}
