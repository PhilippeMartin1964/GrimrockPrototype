#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "Core/GridObjectAudio.h"
#include "GridRuntimeObjectActor.generated.h"

class UAudioComponent;
class UGridObjectArchetypeAsset;
class USoundAttenuation;
class USoundBase;
class UStaticMeshComponent;

struct GRIMROCKPROTOTYPE_API FGridObjectAudioPlaybackResult
{
	bool bRequested = false;
	USoundBase* Sound = nullptr;
	UAudioComponent* AudioComponent = nullptr;
	float Pitch = 1.0f;
	float StartTimeSeconds = 0.0f;
	float ExpectedDuration = 0.0f;
};

UCLASS()
class GRIMROCKPROTOTYPE_API AGridRuntimeObjectActor : public AActor
{
	GENERATED_BODY()

public:
	AGridRuntimeObjectActor();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FGuid ObjectId;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	EGridLevelObjectType ObjectType = EGridLevelObjectType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	int32 CellX = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	int32 CellY = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	EGridEdge Edge = EGridEdge::None;

	/** Runtime snapshot of the archetype's generic audio events. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Audio")
	TMap<FName, FGridObjectAudioEvent> ObjectAudioEvents;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Audio")
	TObjectPtr<USoundAttenuation> DefaultObjectAudioAttenuation = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category = "Grid")
	virtual void InitializeGridObjectBase(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FVector& WorldLocation, const FRotator& WorldRotation);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	virtual void InitializeGridObject(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool MatchesObjectId(FGuid InObjectId) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool MatchesCell(int32 InCellX, int32 InCellY) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool MatchesEdge(int32 InCellX, int32 InCellY, EGridEdge InEdge) const;

	/** Copies generic audio configuration from any object archetype. */
	void ConfigureObjectAudio(const UGridObjectArchetypeAsset* Archetype);

	UFUNCTION(BlueprintPure, Category = "Audio")
	bool HasObjectAudioEvent(FName EventName) const;

	/** Blueprint-friendly fire-and-forget playback using the generic event contract. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	UAudioComponent* PlayObjectAudioEvent(FName EventName);

	/**
	 * C++ detailed playback API. Specialized actors such as doors may keep the
	 * returned component and control interruption/tails without owning audio data.
	 */
	FGridObjectAudioPlaybackResult PlayObjectAudioEventDetailed(
		FName EventName, bool bEnableNativePlayback = true, float StartTimeSeconds = 0.0f);

private:
	TMap<FName, int32> ObjectAudioEventOccurrences;
};
