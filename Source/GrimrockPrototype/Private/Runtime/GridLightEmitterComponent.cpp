#include "Runtime/GridLightEmitterComponent.h"

#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

UGridLightEmitterComponent::UGridLightEmitterComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridLightEmitterComponent::BeginPlay ()
{
    Super::BeginPlay ();

    SetLightEnabled (bEnableOnBeginPlay);
}

void UGridLightEmitterComponent::SetLightEnabled (bool bEnabled)
{
    bLightEnabled = bEnabled;

    if (bEnabled && NiagaraSystem && !NiagaraComponent)
    {
        NiagaraComponent = NewObject<UNiagaraComponent> (GetOwner (), TEXT ("GridItemNiagara"));
        if (NiagaraComponent)
        {
            NiagaraComponent->SetupAttachment (this);
            NiagaraComponent->SetAsset (NiagaraSystem);
            NiagaraComponent->RegisterComponent ();
            RefreshEmitterTransforms ();
        }
    }

    if (NiagaraComponent)
    {
        RefreshEmitterTransforms ();
        NiagaraComponent->SetVisibility (bEnabled, true);
        if (bEnabled)
        {
            NiagaraComponent->Activate (true);
        } else
        {
            NiagaraComponent->Deactivate ();
        }
    }

    if (bUsePointLight && !PointLightComponent)
    {
        PointLightComponent = NewObject<UPointLightComponent> (GetOwner (), TEXT ("GridItemPointLight"));
        if (PointLightComponent)
        {
            PointLightComponent->SetupAttachment (this);
            PointLightComponent->bUseInverseSquaredFalloff = false;
            PointLightComponent->LightFalloffExponent = 4.f;
            PointLightComponent->RegisterComponent ();
            RefreshEmitterTransforms ();
        }
    }

    if (PointLightComponent)
    {
        RefreshEmitterTransforms ();
        PointLightComponent->SetIntensity (LightIntensity);
        PointLightComponent->SetAttenuationRadius (LightRadius);
        PointLightComponent->SetLightColor (LightColor);
        PointLightComponent->SetVisibility (bEnabled && bUsePointLight);
    }
}

void UGridLightEmitterComponent::RefreshEmitterTransforms ()
{
    if (NiagaraComponent)
    {
        NiagaraComponent->SetRelativeLocation (NiagaraRelativeLocation);
        NiagaraComponent->SetRelativeRotation (NiagaraRelativeRotation);
    }

    if (PointLightComponent)
    {
        PointLightComponent->SetRelativeLocation (PointLightRelativeLocation);
        PointLightComponent->SetRelativeRotation (PointLightRelativeRotation);
    }
}

bool UGridLightEmitterComponent::IsLightEnabled () const
{
    return bLightEnabled;
}
