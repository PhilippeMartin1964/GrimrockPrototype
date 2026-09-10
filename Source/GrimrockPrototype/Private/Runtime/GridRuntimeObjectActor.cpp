#include "Runtime/GridRuntimeObjectActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/GridLevelRuntimeActor.h"
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


void AGridRuntimeObjectActor::InitializeRuntimeWorldObjectBase(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FVector& WorldLocation, const FRotator& WorldRotation)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;
	CellX = ObjectData.CellX;
	CellY = ObjectData.CellY;
	Edge = ObjectData.Edge;

	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(Mesh);
	}

	SetActorLocation(WorldLocation);
	SetActorRotation(WorldRotation);

	// PUZZLE01-LUA01: aliases are persistent level state, therefore every fresh
	// runtime actor rebuild restores them immediately after its main mesh exists.
	ApplyPersistedRuntimeMaterialAliases();
}

FGridObjectBehaviorParams AGridRuntimeObjectActor::ResolveEffectiveBehavior(const FGridRuntimeWorldObjectData& ObjectData) const
{
	const AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor>(GetOwner());
	const UGridWorldObjectDefinitionAsset* Definition = RuntimeActor ? RuntimeActor->FindWorldObjectDefinition(ObjectData.WorldObjectDefinitionId) : nullptr;
	return GridObjectInstanceBehavior::Resolve(ObjectData, Definition);
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


void AGridRuntimeObjectActor::InitializeRuntimeWorldObject(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	InitializeRuntimeWorldObjectBase(ObjectData, Mesh, WorldTransform.GetLocation(), WorldTransform.GetRotation().Rotator());
}

void AGridRuntimeObjectActor::ConfigureObjectAudio(const UGridWorldObjectDefinitionAsset* Definition)
{
	ObjectAudioEvents.Reset();
	ObjectAudioEventOccurrences.Reset();
	DefaultObjectAudioAttenuation = nullptr;

	if (!Definition)
	{
		return;
	}

	ObjectAudioEvents = Definition->AudioEvents;
	// One object = one attenuation profile. Legacy Door attenuation is only a
	// compatibility fallback for assets that have not yet been resaved.
	DefaultObjectAudioAttenuation = Definition->DefaultAudioAttenuation
		? Definition->DefaultAudioAttenuation
		: (Definition->SupportedType == EGridLevelObjectType::Door ? Definition->DoorAudioAttenuation : nullptr);

	// Preserve already-authored door assets that still contain the legacy fields.
	for (const FName EventName : { FName(TEXT("Open")), FName(TEXT("Close")) })
	{
		if (ObjectAudioEvents.Contains(EventName))
		{
			continue;
		}

		FGridObjectAudioEvent LegacyResolvedEvent;
		if (Definition->ResolveAudioEvent(EventName, LegacyResolvedEvent))
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

bool AGridRuntimeObjectActor::SetRuntimeMaterialAlias(FName MaterialSlotName, FName MaterialAlias, bool bPersist, FString& OutError)
{
	OutError.Reset();
	if (!ObjectId.IsValid())
	{
		OutError = TEXT("Runtime object has no valid ObjectId.");
		return false;
	}
	if (MaterialSlotName.IsNone() || MaterialAlias.IsNone())
	{
		OutError = TEXT("Material slot name and material alias must be non-empty.");
		return false;
	}
	if (!MeshComponent || !MeshComponent->GetStaticMesh())
	{
		OutError = TEXT("Runtime object has no main static mesh for material replacement.");
		return false;
	}

	AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor>(GetOwner());
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		OutError = TEXT("Runtime object has no owning level runtime/LevelAsset.");
		return false;
	}

	const FGridWorldObjectInstance* Placement = RuntimeActor->LevelAsset->FindWorldObjectInstanceById(ObjectId);
	if (!Placement)
	{
		OutError = FString::Printf(TEXT("Runtime object %s has no world-object placement."), *ObjectId.ToString());
		return false;
	}

	const UGridWorldObjectDefinitionAsset* Definition = RuntimeActor->FindWorldObjectDefinition(Placement->WorldObjectDefinitionId);
	if (!Definition)
	{
		OutError = FString::Printf(TEXT("World-object definition '%s' is unavailable."), *Placement->WorldObjectDefinitionId.ToString());
		return false;
	}

	const TObjectPtr<UMaterialInterface>* MaterialPtr = Definition->RuntimeMaterialAliases.Find(MaterialAlias);
	UMaterialInterface* Material = MaterialPtr ? MaterialPtr->Get() : nullptr;
	if (!Material)
	{
		OutError = FString::Printf(TEXT("Material alias '%s' is not declared by definition '%s'."), *MaterialAlias.ToString(), *Definition->DefinitionId.ToString());
		return false;
	}

	const int32 MaterialIndex = MeshComponent->GetMaterialIndex(MaterialSlotName);
	if (MaterialIndex == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("Material slot '%s' does not exist on runtime object '%s'."), *MaterialSlotName.ToString(), *ObjectId.ToString());
		return false;
	}

	FGridLevelRuntimeState* RuntimeState = nullptr;
	if (bPersist)
	{
		RuntimeState = RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel();
		if (!RuntimeState)
		{
			OutError = TEXT("Current level runtime state is unavailable for persistent material override.");
			return false;
		}
	}

	MeshComponent->SetMaterial(MaterialIndex, Material);

	if (RuntimeState)
	{
		FGridRuntimeObjectVisualState& VisualState = RuntimeState->ObjectVisuals.FindOrAdd(ObjectId);
		VisualState.ObjectId = ObjectId;
		VisualState.MaterialAliasesBySlot.Add(MaterialSlotName, MaterialAlias);
	}

	return true;
}

void AGridRuntimeObjectActor::ApplyPersistedRuntimeMaterialAliases()
{
	if (!ObjectId.IsValid() || !MeshComponent || !MeshComponent->GetStaticMesh())
	{
		return;
	}

	const AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor>(GetOwner());
	const FGridLevelRuntimeState* RuntimeState = RuntimeActor ? RuntimeActor->FindRuntimeStateForCurrentLevel() : nullptr;
	const FGridRuntimeObjectVisualState* VisualState = RuntimeState ? RuntimeState->ObjectVisuals.Find(ObjectId) : nullptr;
	if (!VisualState)
	{
		return;
	}

	for (const TPair<FName, FName>& Pair : VisualState->MaterialAliasesBySlot)
	{
		FString Error;
		if (!SetRuntimeMaterialAlias(Pair.Key, Pair.Value, false, Error))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("GridRuntimeObject persisted material override skipped: ObjectId=%s Slot=%s Alias=%s Reason=%s"),
				*ObjectId.ToString(), *Pair.Key.ToString(), *Pair.Value.ToString(), *Error);
		}
	}
}