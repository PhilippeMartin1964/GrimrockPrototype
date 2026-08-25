#include "Magic/GridSpellPresentationComponent.h"

#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Runtime/Combat/GridCombatProjectileActor.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	template <typename ObjectType> ObjectType* ResolveFirstConfigured(const TArray<TSoftObjectPtr<ObjectType>>& Variations)
	{
		for (const TSoftObjectPtr<ObjectType>& Candidate : Variations)
		{
			if (!Candidate.IsNull())
			{
				return Candidate.LoadSynchronous();
			}
		}
		return nullptr;
	}
}

UGridSpellPresentationComponent::UGridSpellPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridSpellPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopActivePresentation();
	Super::EndPlay(EndPlayReason);
}

bool UGridSpellPresentationComponent::PresentSpell(const FGridSpellPresentationPlan& Plan, const FGridSpellPresentationProfile& Profile)
{
	if (!Plan.IsValid() || !Profile.IsValid())
	{
		return false;
	}

	StopActivePresentation();
	ActivePlan = Plan;
	ActiveProfile = Profile;
	EmitEvent(EGridSpellPresentationEvent::CastStarted);

	UWorld* World = GetWorld();
	if (Plan.bLaunchProjectile)
	{
		if (World && bProjectileEnabled && !Profile.Projectile.VisualMesh.IsNull())
		{
			UStaticMesh* Mesh = Profile.Projectile.VisualMesh.LoadSynchronous();
			if (IsValid(Mesh))
			{
				AGridCombatProjectileActor* Projectile =
					World->SpawnActor<AGridCombatProjectileActor>(AGridCombatProjectileActor::StaticClass(), Plan.SourceWorldLocation, FRotator::ZeroRotator);
				if (Projectile &&
					Projectile->InitializeProjectilePresentation(Mesh, Plan.SourceWorldLocation, Plan.TargetWorldLocation,
						Profile.Projectile.TravelDurationSeconds, Profile.Projectile.VisualScale, Profile.Projectile.RotationOffset))
				{
					ActiveProjectile = Projectile;
				}
				else if (Projectile)
				{
					Projectile->Destroy();
				}
			}
		}

		EmitEvent(EGridSpellPresentationEvent::ProjectileLaunched);
		if (World)
		{
			World->GetTimerManager().SetTimer(
				ImpactTimerHandle, this, &UGridSpellPresentationComponent::HandleImpact, Profile.Projectile.TravelDurationSeconds, false);
		}
		else
		{
			HandleImpact();
		}
	}
	else
	{
		HandleImpact();
	}
	return true;
}

void UGridSpellPresentationComponent::ResetPresentation()
{
	StopActivePresentation();
	LastPresentationRequest = FGridSpellPresentationRequest();
	PresentationRequestCount = 0;
	NextSequenceNumber = 1;
}

void UGridSpellPresentationComponent::EmitEvent(EGridSpellPresentationEvent Event)
{
	FGridSpellPresentationRequest Request;
	Request.SequenceNumber = NextSequenceNumber++;
	Request.Event = Event;
	Request.SpellId = ActivePlan.SpellId;
	Request.ResolvedTarget = ActivePlan.ResolvedTarget;
	Request.WorldLocation = Event == EGridSpellPresentationEvent::CastStarted || Event == EGridSpellPresentationEvent::ProjectileLaunched
		? ActivePlan.SourceWorldLocation
		: ActivePlan.TargetWorldLocation;
	Request.ProjectileActor = ActiveProjectile.Get();

	const FGridPlayerAttackAudioDefinition* Audio = nullptr;
	const FGridPlayerAttackVFXDefinition* VFX = nullptr;
	if (Event == EGridSpellPresentationEvent::CastStarted)
	{
		Audio = &ActiveProfile.CastAudio;
		VFX = &ActiveProfile.CastVFX;
	}
	else if (Event == EGridSpellPresentationEvent::Impact)
	{
		Audio = &ActiveProfile.ImpactAudio;
		VFX = &ActiveProfile.ImpactVFX;
	}

	if (Audio && bAudioEnabled && Audio->IsValid())
	{
		Request.ResolvedSound = ResolveFirstConfigured(Audio->Sounds);
		if (Request.ResolvedSound && GetWorld())
		{
			UGameplayStatics::PlaySoundAtLocation(this, Request.ResolvedSound, Request.WorldLocation, Audio->VolumeMultiplier, Audio->PitchMin);
		}
	}

	if (VFX && bVFXEnabled && VFX->IsValid())
	{
		Request.ResolvedSystem = ResolveFirstConfigured(VFX->Systems);
		if (Request.ResolvedSystem && GetWorld())
		{
			UNiagaraComponent* Niagara = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Request.ResolvedSystem,
				Request.WorldLocation + VFX->LocationOffset, VFX->RotationOffset, VFX->Scale, true, true, ENCPoolMethod::None, true);
			if (Niagara)
			{
				ActiveNiagaraComponents.Add(Niagara);
			}
		}
	}

	LastPresentationRequest = Request;
	++PresentationRequestCount;
	OnPresentationRequested.Broadcast(Request);
}

void UGridSpellPresentationComponent::HandleImpact()
{
	EmitEvent(EGridSpellPresentationEvent::Impact);
	EmitEvent(EGridSpellPresentationEvent::Completed);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ImpactTimerHandle);
	}
}

void UGridSpellPresentationComponent::StopActivePresentation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ImpactTimerHandle);
	}
	if (ActiveProjectile.IsValid())
	{
		ActiveProjectile->Destroy();
	}
	ActiveProjectile.Reset();
	for (TWeakObjectPtr<UNiagaraComponent>& Niagara : ActiveNiagaraComponents)
	{
		if (Niagara.IsValid())
		{
			Niagara->DeactivateImmediate();
			Niagara->DestroyComponent();
		}
	}
	ActiveNiagaraComponents.Reset();
	ActivePlan = FGridSpellPresentationPlan();
	ActiveProfile = FGridSpellPresentationProfile();
}
