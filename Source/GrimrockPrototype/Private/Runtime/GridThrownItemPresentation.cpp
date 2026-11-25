#include "Runtime/GridThrownItemActor.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

void AGridThrownItemActor::StartCombatFlightPresentation(
	const FGridPlayerAttackRequest& Request, const UGridItemSubsystem& Registry, bool bPlayAudio, bool bSpawnVFX)
{
	const FGridAttackPresentationSource& Source = Request.PresentationSource;
	if (Request.PreparedThrownItemActor.Get() != this || AttackSourceGuid.IsValid() || !Source.RuntimeObjectId.IsValid() || !ThrownItemInstance.IsValid() ||
		bConversionAttempted || bEmbeddedInMonster || (!bPlayAudio && !bSpawnVFX))
	{
		return;
	}
	FGridResolvedBoundAction Bound;
	FGridResolvedItem Item;
	if (!Registry.ResolveBoundAttackForHand(Request.OffensiveItemDefinitionId, Source.ItemLevel, Source.ActionId, Request.OffensiveEquipmentSlot, Bound) ||
		!Registry.ResolveItem(Request.OffensiveItemDefinitionId, Source.ItemLevel, Item))
	{
		return;
	}
	const FGridVFXVariants& Flight = Item.ProjectileAppearance.FlightVFX;
	if (!Bound.Presentation.Projectile.HasMedia() && Flight.Systems.IsEmpty())
	{
		return;
	}

	AttackSourceGuid = Source.RuntimeObjectId;
	AttackSourceItemId = Request.OffensiveItemDefinitionId;
	AttackSourceItemLevel = Source.ItemLevel;
	AttackSourceHand = Request.OffensiveEquipmentSlot;
	bFlightAudio = bPlayAudio;
	bFlightVFX = bSpawnVFX;
	ProjectileCues = MoveTemp(Bound.Presentation.Projectile);
	PlayProjectileCue(TEXT("Release"), GetActorLocation(), GetActorRotation(), &ProjectileCues.ReleaseAudio, ProjectileCues.ReleaseVFX);

	if (bFlightVFX && !Flight.Systems.IsEmpty())
	{
		const int32 VariationIndex = GetTypeHash(AttackSourceGuid) % static_cast<uint32>(Flight.Systems.Num());
		if (UNiagaraSystem* System = Flight.Systems[VariationIndex].LoadSynchronous())
		{
			FlightEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(System, GetRootComponent(), Flight.SocketName, Flight.LocationOffset,
				Flight.RotationOffset, Flight.Scale, EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None, true, false);
			UE_LOG(LogGridThrownItem, Verbose, TEXT("[ArmamentProjectile] Stage=Flight SourceGuid=%s UnitGuid=%s Niagara=%s Spawned=%d"),
				*AttackSourceGuid.ToString(), *ThrownItemInstance.RuntimeObjectId.ToString(), *GetNameSafe(System), FlightEffect.IsValid());
		}
	}
}

void AGridThrownItemActor::PresentFlightCollision(FVector Location, FVector Normal)
{
	if (!AttackSourceGuid.IsValid() || bPresentedFlightCollision || bPresentedFlightStop)
	{
		return;
	}
	bPresentedFlightCollision = true;
	PlayProjectileCue(TEXT("Collision"), Location, Normal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector).Rotation(), &ProjectileCues.CollisionAudio,
		ProjectileCues.CollisionVFX);
}

void AGridThrownItemActor::StopCombatFlightPresentation(FVector Location)
{
	if (!AttackSourceGuid.IsValid() || bPresentedFlightStop)
	{
		return;
	}
	bPresentedFlightStop = true;
	if (UNiagaraComponent* Niagara = FlightEffect.Get())
	{
		Niagara->Deactivate();
	}
	FlightEffect.Reset();
	PlayProjectileCue(TEXT("Stop"), Location, GetActorRotation(), nullptr, ProjectileCues.StopVFX);
}

void AGridThrownItemActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UNiagaraComponent* Niagara = FlightEffect.Get())
	{
		Niagara->DeactivateImmediate();
	}
	FlightEffect.Reset();
	Super::EndPlay(EndPlayReason);
}

void AGridThrownItemActor::PlayProjectileCue(
	const TCHAR* Stage, FVector Location, FRotator Rotation, const FGridAudioVariants* Audio, const FGridVFXVariants& VFX) const
{
	const uint32 Seed = HashCombine(GetTypeHash(AttackSourceGuid), FCrc::StrCrc32(Stage));
	USoundBase* Sound = nullptr;
	if (bFlightAudio && Audio && !Audio->Sounds.IsEmpty())
	{
		Sound = Audio->Sounds[Seed % static_cast<uint32>(Audio->Sounds.Num())].LoadSynchronous();
		if (Sound)
		{
			FRandomStream Pitch(static_cast<int32>(Seed));
			UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, Audio->VolumeMultiplier, Pitch.FRandRange(Audio->PitchMin, Audio->PitchMax));
		}
	}
	UNiagaraSystem* System = nullptr;
	if (bFlightVFX && !VFX.Systems.IsEmpty())
	{
		System = VFX.Systems[Seed % static_cast<uint32>(VFX.Systems.Num())].LoadSynchronous();
		if (System)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, System, Location + Rotation.RotateVector(VFX.LocationOffset), Rotation + VFX.RotationOffset, VFX.Scale);
		}
	}
	UE_LOG(LogGridThrownItem, Verbose,
		TEXT("[ArmamentProjectile] Stage=%s Source=%s SourceGuid=%s Level=%d Hand=%d Unit=%s UnitGuid=%s UnitLevel=%d Sound=%s Niagara=%s"), Stage,
		*AttackSourceItemId.ToString(), *AttackSourceGuid.ToString(), AttackSourceItemLevel, static_cast<int32>(AttackSourceHand),
		*ThrownItemInstance.ItemDefinitionId.ToString(), *ThrownItemInstance.RuntimeObjectId.ToString(), ThrownItemInstance.ItemLevel, *GetNameSafe(Sound),
		*GetNameSafe(System));
}
