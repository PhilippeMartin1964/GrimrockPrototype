#include "Runtime/GridDoorActor.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridInteractionUtils.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/ConstructorHelpers.h"

AGridDoorActor::AGridDoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

	ChainRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ChainRoot"));
	ChainRootComponent->SetupAttachment(RootComponent);

	ChainSupportMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChainSupportMesh"));
	ChainSupportMeshComponent->SetupAttachment(ChainRootComponent);
	ChainSupportMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChainSupportMeshComponent->SetGenerateOverlapEvents(false);

	ChainMovingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChainMovingMesh"));
	ChainMovingMeshComponent->SetupAttachment(ChainRootComponent);
	ChainMovingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChainMovingMeshComponent->SetGenerateOverlapEvents(false);

	ChainInteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ChainInteractionBox"));
	ChainInteractionBox->SetupAttachment(ChainMovingMeshComponent);
	ChainInteractionBox->SetBoxExtent(FVector(35.f, 35.f, 90.f));
	ChainInteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChainInteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ChainInteractionBox->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultChainSupportMesh(
		TEXT("/Game/GrimrockPrototype/Meshes/Door/SM_Door_Chain_Support_01.SM_Door_Chain_Support_01"));
	if (DefaultChainSupportMesh.Succeeded())
	{
		ChainSupportMesh = DefaultChainSupportMesh.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultChainMovingMesh(
		TEXT("/Game/GrimrockPrototype/Meshes/Door/SM_Door_Chain_Moving_01.SM_Door_Chain_Moving_01"));
	if (DefaultChainMovingMesh.Succeeded())
	{
		ChainMovingMesh = DefaultChainMovingMesh.Object;
	}
}

void AGridDoorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsAnimating)
	{
		UpdateAnimation(DeltaSeconds);
	}

	if (bIsChainAnimating)
	{
		UpdateChainAnimation(DeltaSeconds);
	}

	if (bIsChainSwinging)
	{
		UpdateChainSwingAnimation(DeltaSeconds);
	}
}

void AGridDoorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopDoorMotionSound();
	Super::EndPlay(EndPlayReason);
}

void AGridDoorActor::InitializeDoor(const FGridLevelObjectData& ObjectData, UStaticMesh* InMovingMesh, UMaterialInterface* InMovingMaterial,
	UStaticMesh* InFixedMesh, UMaterialInterface* InFixedMaterial, const FVector& ClosedWorldLocation, const FRotator& WorldRotation, bool bStartOpen)
{
	StopDoorMotionSound();

	ObjectId = ObjectData.ObjectId;
	CellX = ObjectData.CellX;
	CellY = ObjectData.CellY;
	Edge = ObjectData.Edge;

	SetActorLocation(ClosedWorldLocation);
	SetActorRotation(WorldRotation);

	SetFixedMesh(InFixedMesh, InFixedMaterial);
	SetMovingMesh(InMovingMesh, InMovingMaterial);

	OpenHeight = ObjectData.Behavior.DoorAnimation.OpenHeight;
	MoveDuration = ObjectData.Behavior.DoorAnimation.MoveDuration;

	MovingClosedRelativeLocation = FVector::ZeroVector;
	MovingOpenRelativeLocation = FVector(0.f, 0.f, OpenHeight);

	MoveStartRelativeLocation = FVector::ZeroVector;
	MoveTargetRelativeLocation = FVector::ZeroVector;
	MoveElapsed = 0.f;
	CurrentMoveDuration = 0.f;

	bIsOpen = bStartOpen;
	bIsAnimating = false;

	SetMovingRelativeLocation(bIsOpen ? MovingOpenRelativeLocation : MovingClosedRelativeLocation);

	InitializeChainMechanism(ObjectData.Behavior.DoorAnimation);
	RefreshTickEnabled();
}

void AGridDoorActor::ConfigureDoorAudio(const UGridObjectArchetypeAsset* Archetype)
{
	StopDoorMotionSound();

	DoorOpenSounds.Reset();
	DoorCloseSounds.Reset();
	DoorAudioVolume = 1.0f;
	DoorAudioPitchVariation = 0.04f;
	DoorAudioAttenuation = nullptr;
	DoorOpenAudioOccurrence = 0;
	DoorCloseAudioOccurrence = 0;
	DoorOpenAudioPlaybackRequestCount = 0;
	DoorCloseAudioPlaybackRequestCount = 0;
	DoorAudioStopRequestCount = 0;
	bDoorMotionAudioActive = false;
	bDoorMotionAudioOpening = false;
	ActiveDoorAudioComponent = nullptr;

	if (!Archetype || Archetype->SupportedType != EGridLevelObjectType::Door)
	{
		return;
	}

	DoorOpenSounds = Archetype->DoorOpenSounds;
	DoorCloseSounds = Archetype->DoorCloseSounds;
	DoorAudioVolume = FMath::Max(0.f, Archetype->DoorAudioVolume);
	DoorAudioPitchVariation = FMath::Clamp(Archetype->DoorAudioPitchVariation, 0.f, 0.25f);
	DoorAudioAttenuation = Archetype->DoorAudioAttenuation;
}

void AGridDoorActor::SetDoorOpenState(bool bOpen)
{
	if (!MovingMeshComponent)
	{
		return;
	}

	const FVector DesiredTarget = bOpen ? MovingOpenRelativeLocation : MovingClosedRelativeLocation;

	if (!bIsAnimating && bIsOpen == bOpen)
	{
		return;
	}

	// Repeated commands toward the already active target are presentation no-ops.
	// A genuine reversal has a different target and is allowed to start a new sound.
	if (bIsAnimating && MoveTargetRelativeLocation.Equals(DesiredTarget, 0.1f))
	{
		return;
	}

	// Any genuine target change owns the movement audio too. Stop the previous
	// voice before evaluating whether the reverse command actually has travel.
	// This also fixes an immediate Open -> Close before the first animation Tick.
	if (bIsAnimating)
	{
		StopDoorMotionSound();
	}

	const FVector CurrentLocation = MovingMeshComponent->GetRelativeLocation();

	if (CurrentLocation.Equals(DesiredTarget, 0.1f))
	{
		bIsOpen = bOpen;
		bIsAnimating = false;
		MoveElapsed = 0.f;
		CurrentMoveDuration = 0.f;
		MovingMeshComponent->SetRelativeLocation(DesiredTarget);
		RefreshTickEnabled();
		return;
	}

	const float FullTravelDistance = FVector::Dist(MovingClosedRelativeLocation, MovingOpenRelativeLocation);

	const float RemainingDistance = FVector::Dist(CurrentLocation, DesiredTarget);

	if (FullTravelDistance <= KINDA_SMALL_NUMBER || RemainingDistance <= KINDA_SMALL_NUMBER)
	{
		bIsOpen = bOpen;
		bIsAnimating = false;
		MoveElapsed = 0.f;
		CurrentMoveDuration = 0.f;
		MovingMeshComponent->SetRelativeLocation(DesiredTarget);
		RefreshTickEnabled();
		return;
	}

	MoveStartRelativeLocation = CurrentLocation;
	MoveTargetRelativeLocation = DesiredTarget;
	MoveElapsed = 0.f;

	const float TravelRatio = FMath::Clamp(RemainingDistance / FullTravelDistance, 0.f, 1.f);
	CurrentMoveDuration = FMath::Max(0.01f, MoveDuration * TravelRatio);

	bIsAnimating = true;
	PlayDoorMotionSound(bOpen);
	RefreshTickEnabled();
}

void AGridDoorActor::SnapDoorOpenState(bool bOpen)
{
	StopDoorMotionSound();

	bIsOpen = bOpen;
	bIsAnimating = false;
	MoveElapsed = 0.f;
	CurrentMoveDuration = 0.f;
	SetMovingRelativeLocation(bIsOpen ? MovingOpenRelativeLocation : MovingClosedRelativeLocation);
	RefreshTickEnabled();
}

void AGridDoorActor::OpenDoor()
{
	SetDoorOpenState(true);
}

void AGridDoorActor::CloseDoor()
{
	SetDoorOpenState(false);
}

bool AGridDoorActor::PlayDoorMotionSound(bool bOpening)
{
	// A door owns at most one movement voice. This makes audio follow the same
	// authority as the physical movement instead of allowing autonomous one-shots.
	StopDoorMotionSound();

	const TArray<TObjectPtr<USoundBase>>& Sounds = bOpening ? DoorOpenSounds : DoorCloseSounds;
	if (Sounds.IsEmpty())
	{
		return false;
	}

	int32& Occurrence = bOpening ? DoorOpenAudioOccurrence : DoorCloseAudioOccurrence;
	int32& RequestCount = bOpening ? DoorOpenAudioPlaybackRequestCount : DoorCloseAudioPlaybackRequestCount;
	const int32 StartIndex = Occurrence % Sounds.Num();

	USoundBase* SelectedSound = nullptr;
	for (int32 Offset = 0; Offset < Sounds.Num(); ++Offset)
	{
		const int32 Index = (StartIndex + Offset) % Sounds.Num();
		if (Sounds[Index])
		{
			SelectedSound = Sounds[Index].Get();
			break;
		}
	}

	if (!SelectedSound)
	{
		return false;
	}

	const int32 ThisOccurrence = Occurrence++;
	++RequestCount;
	bDoorMotionAudioActive = true;
	bDoorMotionAudioOpening = bOpening;

	if (bNativeDoorAudioPlaybackEnabled)
	{
		ActiveDoorAudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, SelectedSound, GetActorLocation(), FRotator::ZeroRotator, DoorAudioVolume,
			SelectDoorAudioPitch(ThisOccurrence), 0.f, DoorAudioAttenuation, nullptr, true);
	}

	return true;
}

bool AGridDoorActor::StopDoorMotionSound()
{
	const bool bHadLogicalVoice = bDoorMotionAudioActive;

	if (IsValid(ActiveDoorAudioComponent))
	{
		ActiveDoorAudioComponent->Stop();
	}
	ActiveDoorAudioComponent = nullptr;

	if (bHadLogicalVoice)
	{
		++DoorAudioStopRequestCount;
	}

	bDoorMotionAudioActive = false;
	bDoorMotionAudioOpening = false;
	return bHadLogicalVoice;
}

float AGridDoorActor::SelectDoorAudioPitch(int32 OccurrenceNumber) const
{
	if (DoorAudioPitchVariation <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	// Presentation-only deterministic pattern. It never consumes gameplay RNG.
	static constexpr float PitchOffsets[] = { -1.0f, 0.35f, 1.0f, -0.45f, 0.0f };
	const int32 PatternIndex = FMath::Abs(OccurrenceNumber) % UE_ARRAY_COUNT(PitchOffsets);
	return FMath::Max(0.01f, 1.0f + PitchOffsets[PatternIndex] * DoorAudioPitchVariation);
}

void AGridDoorActor::PullChain()
{
	if (!ChainSupportMeshComponent || !ChainMovingMeshComponent || !ChainInteractionBox || !ChainSupportMeshComponent->IsVisible() ||
		!ChainMovingMeshComponent->IsVisible() || bIsAnimating || bIsChainAnimating || bIsChainSwinging)
	{
		return;
	}

	ChainAnimationElapsed = 0.f;
	bIsChainAnimating = true;
	ChainInteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RefreshTickEnabled();
}

UBoxComponent* AGridDoorActor::GetChainInteractionComponent() const
{
	return ChainInteractionBox;
}

void AGridDoorActor::InitializeChainMechanism(const FGridDoorAnimationParams& ChainParams)
{
	if (!ChainRootComponent || !ChainSupportMeshComponent || !ChainMovingMeshComponent || !ChainInteractionBox)
	{
		return;
	}

	bIsChainAnimating = false;
	bIsChainSwinging = false;
	ChainAnimationElapsed = 0.f;
	ChainSwingElapsed = 0.f;

	const bool bShowChain = ChainParams.bHasChainMechanism && ChainSupportMesh != nullptr && ChainMovingMesh != nullptr;
	ChainSupportMeshComponent->SetStaticMesh(bShowChain ? ChainSupportMesh.Get() : nullptr);
	ChainMovingMeshComponent->SetStaticMesh(bShowChain ? ChainMovingMesh.Get() : nullptr);
	if (bShowChain && ChainMaterial)
	{
		ChainSupportMeshComponent->SetMaterial(0, ChainMaterial);
		ChainMovingMeshComponent->SetMaterial(0, ChainMaterial);
	}

	ChainRestRelativeLocation = FVector::ZeroVector;
	ChainPulledRelativeLocation = FVector(0.f, 0.f, -FMath::Max(0.f, ChainParams.ChainPullDistance));

	ChainRootComponent->SetRelativeLocation(FVector::ZeroVector);
	ChainRootComponent->SetRelativeRotation(FRotator::ZeroRotator);
	ChainSupportMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	ChainSupportMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
	ChainMovingMeshComponent->SetRelativeLocation(ChainRestRelativeLocation);
	ChainMovingRestRelativeRotation = FRotator::ZeroRotator;
	ChainMovingMeshComponent->SetRelativeRotation(ChainMovingRestRelativeRotation);

	if (bShowChain)
	{
		const FBox MovingMeshBounds = ChainMovingMesh->GetBoundingBox();
		if (MovingMeshBounds.IsValid)
		{
			constexpr float InteractionMargin = 10.f;
			ChainInteractionBox->SetRelativeLocation(MovingMeshBounds.GetCenter());
			ChainInteractionBox->SetBoxExtent(MovingMeshBounds.GetExtent() + FVector(InteractionMargin), false);
		}
	}
	else
	{
		ChainInteractionBox->SetRelativeLocation(FVector::ZeroVector);
	}

	ChainSupportMeshComponent->SetVisibility(bShowChain, true);
	ChainMovingMeshComponent->SetVisibility(bShowChain, true);
	ChainInteractionBox->SetVisibility(bShowChain, true);
	ChainInteractionBox->SetCollisionEnabled(bShowChain ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	ChainInteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ChainInteractionBox->SetCollisionResponseToChannel(ECC_Visibility, bShowChain ? ECR_Block : ECR_Ignore);

	CurrentChainPullDuration = FMath::Max(0.01f, ChainParams.ChainPullDuration);
}

bool AGridDoorActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
	if (HitComponent != ChainInteractionBox || !ChainSupportMeshComponent || !ChainMovingMeshComponent || !ChainSupportMeshComponent->IsVisible() ||
		!ChainMovingMeshComponent->IsVisible() || bIsAnimating || bIsChainAnimating || bIsChainSwinging)
	{
		return false;
	}

	const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	const AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	return PartyPawn && RuntimeActor && RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn);
}

void AGridDoorActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
{
	if (CanInteract_Implementation(InstigatorPawn, HitComponent))
	{
		PullChain();
	}
}

void AGridDoorActor::InteractWithHit_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult)
{
	(void)HitResult;
	Interact_Implementation(InstigatorPawn, HitComponent);
}

EGridInteractionCursor AGridDoorActor::GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const
{
	return HitComponent == ChainInteractionBox ? EGridInteractionCursor::Pull : EGridInteractionCursor::Default;
}

FText AGridDoorActor::GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent != ChainInteractionBox)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(bIsOpen ? TEXT("Close door") : TEXT("Open door"));
}

void AGridDoorActor::UpdateChainAnimation(float DeltaSeconds)
{
	if (!ChainMovingMeshComponent || !ChainInteractionBox)
	{
		bIsChainAnimating = false;
		RefreshTickEnabled();
		return;
	}

	ChainAnimationElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(ChainAnimationElapsed / CurrentChainPullDuration, 0.f, 1.f);
	const float PullAlpha = Alpha <= 0.5f ? Alpha * 2.f : (1.f - Alpha) * 2.f;
	ChainMovingMeshComponent->SetRelativeLocation(FMath::Lerp(ChainRestRelativeLocation, ChainPulledRelativeLocation, PullAlpha));

	if (Alpha >= 1.f)
	{
		ChainMovingMeshComponent->SetRelativeLocation(ChainRestRelativeLocation);
		ChainAnimationElapsed = 0.f;
		bIsChainAnimating = false;
		ChainInteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		if (AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(nullptr, this))
		{
			if (!RuntimeActor->ToggleDoorOnEdge(CellX, CellY, Edge))
			{
				UE_LOG(LogTemp, Warning, TEXT("Grid door chain failed: ObjectId=%s Cell=(%d,%d) Edge=%d Reason=central toggle rejected"), *ObjectId.ToString(),
					CellX, CellY, static_cast<int32>(Edge));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Grid door chain failed: ObjectId=%s Cell=(%d,%d) Edge=%d Reason=runtime actor not found"), *ObjectId.ToString(),
				CellX, CellY, static_cast<int32>(Edge));
		}
		bIsChainSwinging = true;
		ChainSwingElapsed = 0.f;
		RefreshTickEnabled();
	}
}

void AGridDoorActor::UpdateChainSwingAnimation(float DeltaSeconds)
{
	if (!ChainMovingMeshComponent)
	{
		bIsChainSwinging = false;
		RefreshTickEnabled();
		return;
	}

	constexpr float SwingDuration = 3.5f;
	constexpr float SwingAmplitudeDegrees = 1.5f;
	constexpr float SwingFrequency = 2.8f;

	ChainSwingElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(ChainSwingElapsed / SwingDuration, 0.f, 1.f);
	const float Damping = 1.f - Alpha;
	const float Angle = FMath::Sin(ChainSwingElapsed * SwingFrequency) * SwingAmplitudeDegrees * Damping;

	ChainMovingMeshComponent->SetRelativeRotation(ChainMovingRestRelativeRotation + FRotator(0.f, 0.f, Angle));

	if (Alpha >= 1.f)
	{
		ChainMovingMeshComponent->SetRelativeRotation(ChainMovingRestRelativeRotation);
		bIsChainSwinging = false;
		ChainSwingElapsed = 0.f;
		RefreshTickEnabled();
	}
}

void AGridDoorActor::RefreshTickEnabled()
{
	SetActorTickEnabled(bIsAnimating || bIsChainAnimating || bIsChainSwinging);
}

void AGridDoorActor::UpdateAnimation(float DeltaSeconds)
{
	if (!MovingMeshComponent)
	{
		return;
	}

	const float SafeDuration = FMath::Max(0.01f, CurrentMoveDuration);

	MoveElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(MoveElapsed / SafeDuration, 0.f, 1.f);

	MovingMeshComponent->SetRelativeLocation(FMath::Lerp(MoveStartRelativeLocation, MoveTargetRelativeLocation, Alpha));

	if (Alpha >= 1.f)
	{
		MovingMeshComponent->SetRelativeLocation(MoveTargetRelativeLocation);

		bIsOpen = MoveTargetRelativeLocation.Equals(MovingOpenRelativeLocation, 0.1f);
		bIsAnimating = false;
		MoveElapsed = 0.f;
		CurrentMoveDuration = 0.f;

		// A sample is presentation, never the timing authority. If it is longer
		// than the actual (possibly partial) travel, cut it at the real endpoint.
		StopDoorMotionSound();

		RefreshTickEnabled();
		OnDoorAnimationFinished.Broadcast(CellX, CellY, Edge);
	}
}

void AGridDoorActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform)
{
	StopDoorMotionSound();
	Super::InitializeGridObject(ObjectData, Mesh, Material, WorldTransform);

	OpenHeight = ObjectData.Behavior.DoorAnimation.OpenHeight;
	MoveDuration = ObjectData.Behavior.DoorAnimation.MoveDuration;

	MovingClosedRelativeLocation = FVector::ZeroVector;
	MovingOpenRelativeLocation = FVector(0.f, 0.f, OpenHeight);

	bIsOpen = ObjectData.bInitiallyActive;
	bIsAnimating = false;

	SetMovingRelativeLocation(bIsOpen ? MovingOpenRelativeLocation : MovingClosedRelativeLocation);
	InitializeChainMechanism(ObjectData.Behavior.DoorAnimation);
	RefreshTickEnabled();
}
