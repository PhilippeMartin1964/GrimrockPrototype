#include "Runtime/GridPitTrapdoorActor.h"

#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"

AGridPitTrapdoorActor::AGridPitTrapdoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);
}

void AGridPitTrapdoorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsAnimating)
	{
		UpdateAnimation(DeltaSeconds);
	}
}

void AGridPitTrapdoorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPitMotionSound();
	Super::EndPlay(EndPlayReason);
}

void AGridPitTrapdoorActor::InitializeMechanismVisuals(
	const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, const FTransform& WorldTransform)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;
	CellX = ObjectData.CellX;
	CellY = ObjectData.CellY;
	Edge = ObjectData.Edge;
	SetActorTransform(WorldTransform);

	OpenRelativeRotation = ObjectData.Behavior.PitAnimation.OpenRelativeRotation;
	MoveDuration = FMath::Max(0.0f, ObjectData.Behavior.PitAnimation.MoveDuration);
	ClosedRelativeRotation = FRotator::ZeroRotator;

	if (Archetype)
	{
		SetFixedMesh(Archetype->FixedMesh ? Archetype->FixedMesh.Get() : Archetype->PreviewMesh.Get(),
			Archetype->FixedMaterial ? Archetype->FixedMaterial.Get() : Archetype->PreviewMaterial.Get());

		SetMovingMesh(Archetype->MovingMesh.Get(), Archetype->MovingMaterial.Get());
	}

	if (FixedMeshComponent)
	{
		FixedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	bIsOpen = ObjectData.Behavior.Pit.bInitiallyOpen;
	bTargetOpen = bIsOpen;
	bIsAnimating = false;
	CurrentOpenAlpha = bIsOpen ? 1.0f : 0.0f;
	MoveStartAlpha = CurrentOpenAlpha;
	MoveTargetAlpha = CurrentOpenAlpha;
	MoveElapsed = 0.0f;
	CurrentMoveDuration = 0.0f;

	ApplyOpenAlpha(CurrentOpenAlpha);
	RefreshMovingMeshCollision();
	RefreshTickEnabled();
}

void AGridPitTrapdoorActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform)
{
	(void)Mesh;
	(void)Material;
	// Avoid duplicating the open-pit PreviewMesh on the inherited generic MeshComponent.
	AGridRuntimeObjectActor::InitializeGridObject(ObjectData, nullptr, nullptr, WorldTransform);
}

void AGridPitTrapdoorActor::SetPitOpenVisualState(bool bOpen, bool bPlayAudio)
{
	const bool bHasCoverMesh = MovingMeshComponent && MovingMeshComponent->GetStaticMesh() != nullptr;

	// Repeating the active target is a no-op.
	if (bTargetOpen == bOpen && (bIsAnimating || bIsOpen == bOpen))
	{
		return;
	}

	bTargetOpen = bOpen;

	if (!bHasCoverMesh || MoveDuration <= KINDA_SMALL_NUMBER)
	{
		const bool bWasOpen = bIsOpen;
		SnapPitOpenState(bOpen);
		if (bWasOpen != bIsOpen)
		{
			OnPitAnimationFinished.Broadcast(ObjectId, bWasOpen, bIsOpen);
		}
		return;
	}

	const float DesiredAlpha = bOpen ? 1.0f : 0.0f;
	if (FMath::IsNearlyEqual(CurrentOpenAlpha, DesiredAlpha, KINDA_SMALL_NUMBER))
	{
		const bool bWasOpen = bIsOpen;
		bIsAnimating = false;
		bIsOpen = bOpen;
		CurrentOpenAlpha = DesiredAlpha;
		ApplyOpenAlpha(CurrentOpenAlpha);
		RefreshMovingMeshCollision();
		RefreshTickEnabled();
		// Even when gameplay returns to the already-settled endpoint (for example
		// Open -> Close -> Open before the first Tick), the runtime must clear the
		// pending command associated with the cancelled direction.
		OnPitAnimationFinished.Broadcast(ObjectId, bWasOpen, bIsOpen);
		return;
	}

	StopPitMotionSound();

	MoveStartAlpha = CurrentOpenAlpha;
	MoveTargetAlpha = DesiredAlpha;
	MoveElapsed = 0.0f;
	CurrentMoveDuration = FMath::Max(0.01f, MoveDuration * FMath::Abs(MoveTargetAlpha - MoveStartAlpha));
	bIsAnimating = true;

	const float AudioStartTime = bOpen ? CurrentOpenAlpha * MoveDuration : (1.0f - CurrentOpenAlpha) * MoveDuration;
	StartPitMotionSound(bOpen, AudioStartTime, bPlayAudio);

	// Gameplay collision follows the last settled state until the new endpoint is reached.
	RefreshMovingMeshCollision();
	RefreshTickEnabled();

	UE_LOG(LogTemp, Log,
		TEXT("GridPit animation start ObjectId=%s Cell=(%d,%d) Direction=%s StartAlpha=%.3f TargetAlpha=%.3f FullDuration=%.3f EffectiveDuration=%.3f"),
		*ObjectId.ToString(), CellX, CellY, bOpen ? TEXT("Open") : TEXT("Close"), MoveStartAlpha, MoveTargetAlpha, MoveDuration, CurrentMoveDuration);
}

void AGridPitTrapdoorActor::SnapPitOpenState(bool bOpen)
{
	StopPitMotionSound();

	bIsOpen = bOpen;
	bTargetOpen = bOpen;
	bIsAnimating = false;
	CurrentOpenAlpha = bOpen ? 1.0f : 0.0f;
	MoveStartAlpha = CurrentOpenAlpha;
	MoveTargetAlpha = CurrentOpenAlpha;
	MoveElapsed = 0.0f;
	CurrentMoveDuration = 0.0f;

	ApplyOpenAlpha(CurrentOpenAlpha);
	RefreshMovingMeshCollision();
	RefreshTickEnabled();
}

void AGridPitTrapdoorActor::UpdateAnimation(float DeltaSeconds)
{
	if (!MovingMeshComponent)
	{
		const bool bWasOpen = bIsOpen;
		SnapPitOpenState(bTargetOpen);
		OnPitAnimationFinished.Broadcast(ObjectId, bWasOpen, bIsOpen);
		return;
	}

	const float SafeDuration = FMath::Max(0.01f, CurrentMoveDuration);
	MoveElapsed += FMath::Max(0.0f, DeltaSeconds);

	// Treat the exact scheduled duration as an endpoint even when float math
	// produces a value a few ulps below SafeDuration (common after reversals,
	// where CurrentMoveDuration is itself computed from an interpolated alpha).
	const bool bReachedEndpoint = MoveElapsed + KINDA_SMALL_NUMBER >= SafeDuration;
	const float Alpha = bReachedEndpoint ? 1.0f : FMath::Clamp(MoveElapsed / SafeDuration, 0.0f, 1.0f);
	CurrentOpenAlpha = FMath::Lerp(MoveStartAlpha, MoveTargetAlpha, Alpha);
	ApplyOpenAlpha(CurrentOpenAlpha);

	if (!bReachedEndpoint)
	{
		return;
	}

	const bool bWasOpen = bIsOpen;
	CurrentOpenAlpha = MoveTargetAlpha;
	bIsOpen = bTargetOpen;
	bIsAnimating = false;
	MoveElapsed = 0.0f;
	CurrentMoveDuration = 0.0f;

	ApplyOpenAlpha(CurrentOpenAlpha);
	RefreshMovingMeshCollision();
	RefreshTickEnabled();

	UE_LOG(LogTemp, Log, TEXT("GridPit animation complete ObjectId=%s Cell=(%d,%d) State=%s"), *ObjectId.ToString(), CellX, CellY,
		bIsOpen ? TEXT("Open") : TEXT("Closed"));

	// Always notify endpoint completion. A reversal may return to the original
	// settled gameplay state; the runtime still needs to clear its pending command.
	OnPitAnimationFinished.Broadcast(ObjectId, bWasOpen, bIsOpen);
}

void AGridPitTrapdoorActor::ApplyOpenAlpha(float Alpha)
{
	if (!MovingMeshComponent)
	{
		return;
	}

	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const FQuat ClosedQuat = ClosedRelativeRotation.Quaternion();
	const FQuat OpenQuat = OpenRelativeRotation.Quaternion();
	MovingMeshComponent->SetRelativeRotation(FQuat::Slerp(ClosedQuat, OpenQuat, ClampedAlpha).Rotator());
	MovingMeshComponent->SetVisibility(MovingMeshComponent->GetStaticMesh() != nullptr, true);
}

void AGridPitTrapdoorActor::RefreshMovingMeshCollision()
{
	if (!MovingMeshComponent)
	{
		return;
	}

	const bool bHasCoverMesh = MovingMeshComponent->GetStaticMesh() != nullptr;
	MovingMeshComponent->SetCollisionEnabled(!bIsOpen && bHasCoverMesh ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AGridPitTrapdoorActor::RefreshTickEnabled()
{
	SetActorTickEnabled(bIsAnimating);
}

void AGridPitTrapdoorActor::StartPitMotionSound(bool bOpening, float StartTimeSeconds, bool bEnableNativePlayback)
{
	const FName EventName = bOpening ? FName(TEXT("Open")) : FName(TEXT("Close"));
	const FGridObjectAudioPlaybackResult Playback =
		PlayObjectAudioEventDetailed(EventName, bEnableNativePlayback, FMath::Max(0.0f, StartTimeSeconds));
	ActivePitAudioComponent = Playback.AudioComponent;
}

void AGridPitTrapdoorActor::StopPitMotionSound()
{
	if (IsValid(ActivePitAudioComponent))
	{
		ActivePitAudioComponent->Stop();
	}
	ActivePitAudioComponent = nullptr;
}
