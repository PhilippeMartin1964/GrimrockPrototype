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
	AGridMechanismActor::InitializeMechanismVisuals(ObjectData, Archetype, WorldTransform);

	// Behavior still owns gameplay defaults until WORLDOBJ-MIG04 removes the specialized animation parameters.
	LeftHingeLocation = ObjectData.Behavior.PitAnimation.LeftHingeLocation;
	RightHingeLocation = ObjectData.Behavior.PitAnimation.RightHingeLocation;
	OpenAngleDegrees = FMath::Clamp(ObjectData.Behavior.PitAnimation.OpenAngleDegrees, 0.0f, 120.0f);
	MoveDuration = FMath::Max(0.0f, ObjectData.Behavior.PitAnimation.MoveDuration);

	if (FixedMeshComponent)
	{
		FixedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// WORLDOBJ-MIG03.4: the trapdoor leaves are exactly MovingParts[0] and [1].
	const float TargetDuration = GetTargetMotionDuration();
	if (TargetDuration > KINDA_SMALL_NUMBER)
	{
		MoveDuration = TargetDuration;
	}
	if (GetMovingPartMotion(0).Type == EGridWorldObjectMotionType::Rotation)
	{
		OpenAngleDegrees = FMath::Abs(GetMovingPartMotion(0).Amount);
		LeftHingeLocation = GetMovingPartMotion(0).Pivot;
	}
	if (GetMovingPartMotion(1).Type == EGridWorldObjectMotionType::Rotation)
	{
		RightHingeLocation = GetMovingPartMotion(1).Pivot;
	}

	const bool bHasCover = HasCompleteTrapdoorCover();
	bIsOpen = bHasCover ? ObjectData.Behavior.Pit.bInitiallyOpen : true;
	bTargetOpen = bIsOpen;
	bIsAnimating = false;
	CurrentOpenAlpha = bIsOpen ? 1.0f : 0.0f;
	MoveStartAlpha = CurrentOpenAlpha;
	MoveTargetAlpha = CurrentOpenAlpha;
	MoveElapsed = 0.0f;
	CurrentMoveDuration = 0.0f;

	ApplyOpenAlpha(CurrentOpenAlpha);
	RefreshTrapdoorCollision();
	RefreshTickEnabled();

	const bool bHasPrimary = MovingMeshComponent && MovingMeshComponent->GetStaticMesh() != nullptr;
	const bool bHasSecondary = SecondaryMovingMeshComponent && SecondaryMovingMeshComponent->GetStaticMesh() != nullptr;
	if (bHasPrimary != bHasSecondary)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GridPit dual-leaf cover incomplete ObjectId=%s Cell=(%d,%d): MovingPart[0] and MovingPart[1] are both required; Pit stays Open."),
			*ObjectId.ToString(), CellX, CellY);
	}
}

void AGridPitTrapdoorActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	(void)Mesh;
	AGridRuntimeObjectActor::InitializeGridObject(ObjectData, nullptr, WorldTransform);
}

bool AGridPitTrapdoorActor::HasCompleteTrapdoorCover() const
{
	return MovingMeshComponent && SecondaryMovingMeshComponent && MovingMeshComponent->GetStaticMesh() != nullptr &&
		SecondaryMovingMeshComponent->GetStaticMesh() != nullptr;
}

float AGridPitTrapdoorActor::GetLeftLeafPitch() const
{
	return MovingMeshComponent ? MovingMeshComponent->GetRelativeRotation().Pitch : 0.0f;
}

float AGridPitTrapdoorActor::GetRightLeafPitch() const
{
	return SecondaryMovingMeshComponent ? SecondaryMovingMeshComponent->GetRelativeRotation().Pitch : 0.0f;
}

FVector AGridPitTrapdoorActor::GetLeftHingeLocation() const
{
	return GetMovingPartMotion(0).Pivot;
}

FVector AGridPitTrapdoorActor::GetRightHingeLocation() const
{
	return GetMovingPartMotion(1).Pivot;
}

void AGridPitTrapdoorActor::ConfigureLeafGeometry()
{
	// No specialized leaf geometry remains. MovingPart LocalTransform + Motion.Pivot own the complete geometry.
}

void AGridPitTrapdoorActor::SetPitOpenVisualState(bool bOpen, bool bPlayAudio)
{
	if (!HasCompleteTrapdoorCover())
	{
		const bool bWasOpen = bIsOpen;
		SnapPitOpenState(true);
		if (bWasOpen != bIsOpen)
		{
			OnPitAnimationFinished.Broadcast(ObjectId, bWasOpen, bIsOpen);
		}
		return;
	}

	if (bTargetOpen == bOpen && (bIsAnimating || bIsOpen == bOpen))
	{
		return;
	}

	bTargetOpen = bOpen;
	if (MoveDuration <= KINDA_SMALL_NUMBER)
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
		RefreshTrapdoorCollision();
		RefreshTickEnabled();
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
	RefreshTrapdoorCollision();
	RefreshTickEnabled();

	UE_LOG(LogTemp, Log,
		TEXT("GridPit dual-leaf animation start ObjectId=%s Cell=(%d,%d) Direction=%s StartAlpha=%.3f TargetAlpha=%.3f Angle=%.1f Duration=%.3f EffectiveDuration=%.3f"),
		*ObjectId.ToString(), CellX, CellY, bOpen ? TEXT("Open") : TEXT("Close"), MoveStartAlpha, MoveTargetAlpha,
		OpenAngleDegrees, MoveDuration, CurrentMoveDuration);
}

void AGridPitTrapdoorActor::SnapPitOpenState(bool bOpen)
{
	StopPitMotionSound();

	if (!HasCompleteTrapdoorCover())
	{
		bOpen = true;
	}

	bIsOpen = bOpen;
	bTargetOpen = bOpen;
	bIsAnimating = false;
	CurrentOpenAlpha = bOpen ? 1.0f : 0.0f;
	MoveStartAlpha = CurrentOpenAlpha;
	MoveTargetAlpha = CurrentOpenAlpha;
	MoveElapsed = 0.0f;
	CurrentMoveDuration = 0.0f;

	ApplyOpenAlpha(CurrentOpenAlpha);
	RefreshTrapdoorCollision();
	RefreshTickEnabled();
}

void AGridPitTrapdoorActor::UpdateAnimation(float DeltaSeconds)
{
	if (!HasCompleteTrapdoorCover())
	{
		const bool bWasOpen = bIsOpen;
		SnapPitOpenState(true);
		OnPitAnimationFinished.Broadcast(ObjectId, bWasOpen, bIsOpen);
		return;
	}

	const float SafeDuration = FMath::Max(0.01f, CurrentMoveDuration);
	MoveElapsed += FMath::Max(0.0f, DeltaSeconds);
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
	RefreshTrapdoorCollision();
	RefreshTickEnabled();

	UE_LOG(LogTemp, Log, TEXT("GridPit dual-leaf animation complete ObjectId=%s Cell=(%d,%d) State=%s"),
		*ObjectId.ToString(), CellX, CellY, bIsOpen ? TEXT("Open") : TEXT("Closed"));

	OnPitAnimationFinished.Broadcast(ObjectId, bWasOpen, bIsOpen);
}

void AGridPitTrapdoorActor::ApplyOpenAlpha(float Alpha)
{
	ApplyAllMovingPartMotionsAlpha(FMath::Clamp(Alpha, 0.0f, 1.0f));
}

void AGridPitTrapdoorActor::RefreshTrapdoorCollision()
{
	// Opening is hazardous immediately, so leaf collision is removed as soon as
	// motion starts toward Open. During Closing it stays disabled until Closed.
	const bool bEnableCollision = !bIsOpen && !bIsAnimating && HasCompleteTrapdoorCover();
	const ECollisionEnabled::Type CollisionMode = bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;

	if (MovingMeshComponent)
	{
		MovingMeshComponent->SetCollisionEnabled(CollisionMode);
	}
	if (SecondaryMovingMeshComponent)
	{
		SecondaryMovingMeshComponent->SetCollisionEnabled(CollisionMode);
	}
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
