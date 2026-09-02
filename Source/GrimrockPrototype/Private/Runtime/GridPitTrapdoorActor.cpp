#include "Runtime/GridPitTrapdoorActor.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGridPitTrapdoorActor::AGridPitTrapdoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

	LeftHingeComponent = CreateDefaultSubobject<USceneComponent>(TEXT("LeftTrapdoorHinge"));
	LeftHingeComponent->SetupAttachment(SceneRoot);

	RightHingeComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RightTrapdoorHinge"));
	RightHingeComponent->SetupAttachment(SceneRoot);

	LeftLeafMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftTrapdoorLeaf"));
	LeftLeafMeshComponent->SetupAttachment(LeftHingeComponent);
	LeftLeafMeshComponent->SetMobility(EComponentMobility::Movable);

	RightLeafMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightTrapdoorLeaf"));
	RightLeafMeshComponent->SetupAttachment(RightHingeComponent);
	RightLeafMeshComponent->SetMobility(EComponentMobility::Movable);
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

	LeftHingeLocation = ObjectData.Behavior.PitAnimation.LeftHingeLocation;
	RightHingeLocation = ObjectData.Behavior.PitAnimation.RightHingeLocation;
	OpenAngleDegrees = FMath::Clamp(ObjectData.Behavior.PitAnimation.OpenAngleDegrees, 0.0f, 120.0f);
	MoveDuration = FMath::Max(0.0f, ObjectData.Behavior.PitAnimation.MoveDuration);

	// PIT03.2 explicitly retires the inherited single MovingMesh path.
	SetMovingMesh(nullptr, nullptr);
	if (MovingMeshComponent)
	{
		MovingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MovingMeshComponent->SetVisibility(false, true);
	}

	if (Archetype)
	{
		SetFixedMesh(Archetype->FixedMesh ? Archetype->FixedMesh.Get() : Archetype->PreviewMesh.Get(),
			Archetype->FixedMaterial ? Archetype->FixedMaterial.Get() : Archetype->PreviewMaterial.Get());

		LeftLeafMeshComponent->SetStaticMesh(Archetype->PitLeftLeafMesh.Get());
		RightLeafMeshComponent->SetStaticMesh(Archetype->PitRightLeafMesh.Get());
		if (Archetype->PitLeftLeafMaterial)
		{
			LeftLeafMeshComponent->SetMaterial(0, Archetype->PitLeftLeafMaterial.Get());
		}
		if (Archetype->PitRightLeafMaterial)
		{
			RightLeafMeshComponent->SetMaterial(0, Archetype->PitRightLeafMaterial.Get());
		}
	}

	if (FixedMeshComponent)
	{
		FixedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ConfigureLeafGeometry();

	const bool bHasCover = HasCompleteTrapdoorCover();
	if (LeftLeafMeshComponent)
	{
		LeftLeafMeshComponent->SetVisibility(bHasCover, true);
	}
	if (RightLeafMeshComponent)
	{
		RightLeafMeshComponent->SetVisibility(bHasCover, true);
	}

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

	const bool bHasOnlyOneLeaf =
		(LeftLeafMeshComponent && LeftLeafMeshComponent->GetStaticMesh() != nullptr) !=
		(RightLeafMeshComponent && RightLeafMeshComponent->GetStaticMesh() != nullptr);
	if (bHasOnlyOneLeaf)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GridPit dual-leaf cover incomplete ObjectId=%s Cell=(%d,%d): both Left Leaf Mesh and Right Leaf Mesh are required; Pit stays Open."),
			*ObjectId.ToString(), CellX, CellY);
	}
}

void AGridPitTrapdoorActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform)
{
	(void)Mesh;
	(void)Material;
	AGridRuntimeObjectActor::InitializeGridObject(ObjectData, nullptr, nullptr, WorldTransform);
}

bool AGridPitTrapdoorActor::HasCompleteTrapdoorCover() const
{
	return LeftLeafMeshComponent && RightLeafMeshComponent &&
		LeftLeafMeshComponent->GetStaticMesh() != nullptr && RightLeafMeshComponent->GetStaticMesh() != nullptr;
}

float AGridPitTrapdoorActor::GetLeftLeafPitch() const
{
	return LeftHingeComponent ? LeftHingeComponent->GetRelativeRotation().Pitch : 0.0f;
}

float AGridPitTrapdoorActor::GetRightLeafPitch() const
{
	return RightHingeComponent ? RightHingeComponent->GetRelativeRotation().Pitch : 0.0f;
}

FVector AGridPitTrapdoorActor::GetLeftHingeLocation() const
{
	return LeftHingeComponent ? LeftHingeComponent->GetRelativeLocation() : FVector::ZeroVector;
}

FVector AGridPitTrapdoorActor::GetRightHingeLocation() const
{
	return RightHingeComponent ? RightHingeComponent->GetRelativeLocation() : FVector::ZeroVector;
}

void AGridPitTrapdoorActor::ConfigureLeafGeometry()
{
	if (LeftHingeComponent)
	{
		LeftHingeComponent->SetRelativeLocation(LeftHingeLocation);
	}
	if (RightHingeComponent)
	{
		RightHingeComponent->SetRelativeLocation(RightHingeLocation);
	}

	// The authored leaf meshes already have the correct closed transform in Pit-local
	// space. Moving the parent hinge must therefore be exactly compensated on the
	// child mesh: HingeLocation + LeafRelativeLocation = (0,0,0) while closed.
	// When the hinge rotates, that same compensation makes the mesh orbit around H.
	if (LeftLeafMeshComponent)
	{
		LeftLeafMeshComponent->SetRelativeLocation(-LeftHingeLocation);
		LeftLeafMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
		LeftLeafMeshComponent->SetVisibility(LeftLeafMeshComponent->GetStaticMesh() != nullptr, true);
	}
	if (RightLeafMeshComponent)
	{
		RightLeafMeshComponent->SetRelativeLocation(-RightHingeLocation);
		RightLeafMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
		RightLeafMeshComponent->SetVisibility(RightLeafMeshComponent->GetStaticMesh() != nullptr, true);
	}
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
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const float Angle = OpenAngleDegrees * ClampedAlpha;

	// Local Y is the hinge axis. The left/right signs are opposite so both
	// centre-facing leaf edges fold downward into the pit.
	if (LeftHingeComponent)
	{
		LeftHingeComponent->SetRelativeRotation(FRotator(-Angle, 0.0f, 0.0f));
	}
	if (RightHingeComponent)
	{
		RightHingeComponent->SetRelativeRotation(FRotator(Angle, 0.0f, 0.0f));
	}
}

void AGridPitTrapdoorActor::RefreshTrapdoorCollision()
{
	// Opening is hazardous immediately, so leaf collision is removed as soon as
	// motion starts toward Open. During Closing it stays disabled until Closed.
	const bool bEnableCollision = !bIsOpen && !bIsAnimating && HasCompleteTrapdoorCover();
	const ECollisionEnabled::Type CollisionMode = bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;
	if (LeftLeafMeshComponent)
	{
		LeftLeafMeshComponent->SetCollisionEnabled(CollisionMode);
	}
	if (RightLeafMeshComponent)
	{
		RightLeafMeshComponent->SetCollisionEnabled(CollisionMode);
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
