#include "Runtime/GridItemVisualEffectsComponent.h"

#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

UGridItemVisualEffectsComponent::UGridItemVisualEffectsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridItemVisualEffectsComponent::ApplyConfig(const FGridItemVisualEffectsConfig& InConfig)
{
    RuntimeConfig = InConfig.IsValid() ? InConfig : FGridItemVisualEffectsConfig();

    for (int32 Index = RuntimeConfig.Effects.Num(); Index < EffectComponents.Num(); ++Index)
    {
        if (UNiagaraComponent* Previous = EffectComponents[Index].Get())
        {
            Previous->Deactivate();
            Previous->DestroyComponent();
        }
    }
    EffectComponents.SetNum(RuntimeConfig.Effects.Num());
    RefreshEffects();
}

void UGridItemVisualEffectsComponent::SetItemLightState(bool bEnabled)
{
    if (bItemLightEnabled != bEnabled)
    {
        bItemLightEnabled = bEnabled;
        RefreshEffects();
    }
}

void UGridItemVisualEffectsComponent::SetPresentationEnabled(bool bEnabled)
{
    if (bPresentationEnabled != bEnabled)
    {
        bPresentationEnabled = bEnabled;
        RefreshEffects();
    }
}

UNiagaraComponent* UGridItemVisualEffectsComponent::GetEffectComponent(int32 Index) const
{
    return EffectComponents.IsValidIndex(Index) ? EffectComponents[Index].Get() : nullptr;
}

void UGridItemVisualEffectsComponent::RefreshEffects()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    for (int32 Index = 0; Index < RuntimeConfig.Effects.Num(); ++Index)
    {
        const FGridItemNiagaraEffectConfig& Effect = RuntimeConfig.Effects[Index];
        UNiagaraComponent* Niagara = EffectComponents[Index].Get();
        const bool bShouldShow = bPresentationEnabled &&
            (Effect.ActivationMode == EGridItemVfxActivationMode::Always || bItemLightEnabled);

        if (!bShouldShow)
        {
            if (Niagara)
            {
                Niagara->SetVisibility(false, true);
                Niagara->Deactivate();
            }
            continue;
        }

        UNiagaraSystem* System = Effect.NiagaraSystem.LoadSynchronous();
        if (!System)
        {
            if (Niagara)
            {
                Niagara->SetVisibility(false, true);
                Niagara->Deactivate();
            }
            continue;
        }

        if (!Niagara)
        {
            Niagara = NewObject<UNiagaraComponent>(Owner);
            Niagara->SetAutoActivate(false);
            Niagara->SetupAttachment(this);
            Niagara->SetAsset(System);
            Niagara->SetRelativeTransform(Effect.RelativeTransform);
            Niagara->RegisterComponent();
            EffectComponents[Index] = Niagara;
            Niagara->SetVisibility(true, true);
            Niagara->Activate(true);
            continue;
        }

        const bool bSystemChanged = Niagara->GetAsset() != System;
        const bool bWasHidden = !Niagara->IsVisible();
        if (bSystemChanged)
        {
            Niagara->Deactivate();
            Niagara->SetAsset(System);
        }
        Niagara->SetRelativeTransform(Effect.RelativeTransform);
        Niagara->SetVisibility(true, true);
        if (bSystemChanged || bWasHidden || !Niagara->IsActive())
        {
            Niagara->Activate(true);
        }
    }
}
