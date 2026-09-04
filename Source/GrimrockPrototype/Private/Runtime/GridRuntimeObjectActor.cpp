#include "Runtime/GridRuntimeObjectActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

AGridRuntimeObjectActor::AGridRuntimeObjectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGridRuntimeObjectActor::InitializeGridObjectBase(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& WorldLocation, const FRotator& WorldRotation)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;
	CellX = ObjectData.CellX;
	CellY = ObjectData.CellY;
	Edge = ObjectData.Edge;

	// MATERIAL-OWNERSHIP01: mesh material slots are the sole source of truth.
	// Keep the parameter temporarily for source compatibility with existing call sites.
	(void)Material;
	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(Mesh);
	}

	SetActorLocation(WorldLocation);
	SetActorRotation(WorldRotation);
}

bool AGridRuntimeObjectActor::MatchesObjectId(FGuid InObjectId) const
{
	return ObjectId.IsValid() && ObjectId == InObjectId;
}

bool AGridRuntimeObjectActor::MatchesCell(int32 InCellX, int32 InCellY) const
{
	return CellX == InCellX && CellY == InCellY;
}

bool AGridRuntimeObjectActor::MatchesEdge(int32 InCellX, int32 InCellY, EGridEdge InEdge) const
{
	return CellX == InCellX && CellY == InCellY && Edge == InEdge;
}

void AGridRuntimeObjectActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform)
{
	InitializeGridObjectBase(ObjectData, Mesh, Material, WorldTransform.GetLocation(), WorldTransform.GetRotation().Rotator());
}


void AGridRuntimeObjectActor::ConfigureObjectAudio(const UGridObjectArchetypeAsset* Archetype)
{
	ObjectAudioEvents.Reset();
	ObjectAudioEventOccurrences.Reset();
	DefaultObjectAudioAttenuation = nullptr;

	if (!Archetype)
	{
		return;
	}

	ObjectAudioEvents = Archetype->AudioEvents;
	// One object = one attenuation profile. Legacy Door attenuation is only a
	// compatibility fallback for assets that have not yet been resaved.
	DefaultObjectAudioAttenuation = Archetype->DefaultAudioAttenuation
		? Archetype->DefaultAudioAttenuation
		: (Archetype->SupportedType == EGridLevelObjectType::Door ? Archetype->DoorAudioAttenuation : nullptr);

	// Preserve already-authored door assets that still contain the legacy fields.
	for (const FName EventName : { FName(TEXT("Open")), FName(TEXT("Close")) })
	{
		if (ObjectAudioEvents.Contains(EventName))
		{
			continue;
		}

		FGridObjectAudioEvent LegacyResolvedEvent;
		if (Archetype->ResolveAudioEvent(EventName, LegacyResolvedEvent))
		{
			ObjectAudioEvents.Add(EventName, MoveTemp(LegacyResolvedEvent));
		}
	}
}

bool AGridRuntimeObjectActor::HasObjectAudioEvent(FName EventName) const
{
	const FGridObjectAudioEvent* Event = ObjectAudioEvents.Find(EventName);
	return Event && Event->HasPlayableSound();
}

UAudioComponent* AGridRuntimeObjectActor::PlayObjectAudioEvent(FName EventName)
{
	return PlayObjectAudioEventDetailed(EventName, true).AudioComponent;
}

FGridObjectAudioPlaybackResult AGridRuntimeObjectActor::PlayObjectAudioEventDetailed(
	FName EventName, bool bEnableNativePlayback, float StartTimeSeconds)
{
	FGridObjectAudioPlaybackResult Result;
	const float SafeStartTimeSeconds = FMath::IsFinite(StartTimeSeconds) ? FMath::Max(0.0f, StartTimeSeconds) : 0.0f;
	Result.StartTimeSeconds = SafeStartTimeSeconds;
	const FGridObjectAudioEvent* Event = ObjectAudioEvents.Find(EventName);
	if (!Event || Event->Sounds.IsEmpty())
	{
		return Result;
	}

	int32& Occurrence = ObjectAudioEventOccurrences.FindOrAdd(EventName);
	const int32 StartIndex = Occurrence % Event->Sounds.Num();

	USoundBase* SelectedSound = nullptr;
	for (int32 Offset = 0; Offset < Event->Sounds.Num(); ++Offset)
	{
		const int32 Index = (StartIndex + Offset) % Event->Sounds.Num();
		if (Event->Sounds[Index])
		{
			SelectedSound = Event->Sounds[Index].Get();
			break;
		}
	}

	if (!SelectedSound)
	{
		return Result;
	}

	static constexpr float PitchOffsets[] = { -1.0f, 0.35f, 1.0f, -0.45f, 0.0f };
	const int32 PatternIndex = FMath::Abs(Occurrence) % UE_ARRAY_COUNT(PitchOffsets);
	const float PitchVariation = FMath::Clamp(Event->PitchVariation, 0.f, 0.25f);
	const float Pitch = PitchVariation <= KINDA_SMALL_NUMBER ? 1.0f : FMath::Max(0.01f, 1.0f + PitchOffsets[PatternIndex] * PitchVariation);
	++Occurrence;

	Result.bRequested = true;
	Result.Sound = SelectedSound;
	Result.Pitch = Pitch;

	const float RawDuration = SelectedSound->GetDuration();
	Result.ExpectedDuration = FMath::IsFinite(RawDuration) && RawDuration > 0.f && Pitch > KINDA_SMALL_NUMBER ? RawDuration / Pitch : 0.f;

	if (bEnableNativePlayback)
	{
		Result.AudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, SelectedSound, GetActorLocation(), FRotator::ZeroRotator,
			FMath::Max(0.f, Event->Volume), Pitch, SafeStartTimeSeconds, DefaultObjectAudioAttenuation, nullptr, true);
	}

	return Result;
}
