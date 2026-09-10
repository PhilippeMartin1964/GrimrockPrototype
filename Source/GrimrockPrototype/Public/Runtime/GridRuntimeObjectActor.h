#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "Core/GridObjectAudio.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "GridRuntimeObjectActor.generated.h"

class UAudioComponent;
class UGridWorldObjectDefinitionAsset;
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

	/** Runtime snapshot of the definition's generic audio events. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Audio")
	TMap<FName, FGridObjectAudioEvent> ObjectAudioEvents;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Audio")
	TObjectPtr<USoundAttenuation> DefaultObjectAudioAttenuation = nullptr;

public:
	/** Runtime-native base initializer; never serialized or exposed to Blueprint. */
	virtual void InitializeRuntimeWorldObjectBase(
		const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FVector& WorldLocation, const FRotator& WorldRotation);

	/** Runtime-native world-object initialization boundary introduced by MIG09-E2. */
	virtual void InitializeRuntimeWorldObject(
		const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform);

	/** Resolves shared definition behavior plus strictly instance-owned runtime overrides. */
	FGridObjectBehaviorParams ResolveEffectiveBehavior(const FGridRuntimeWorldObjectData& ObjectData) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool MatchesObjectId(FGuid InObjectId) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool MatchesCell(int32 InCellX, int32 InCellY) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool MatchesEdge(int32 InCellX, int32 InCellY, EGridEdge InEdge) const;

	/** Copies generic audio configuration from any object definition. */
	void ConfigureObjectAudio(const UGridWorldObjectDefinitionAsset* Definition);

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

	/**
	 * PUZZLE01-LUA01 generic visual primitive. Resolves MaterialAlias through the
	 * object's Definition and applies it to a named slot of the main MeshComponent.
	 * When bPersist is true only the alias is stored in level runtime state.
	 */
	bool SetRuntimeMaterialAlias(FName MaterialSlotName, FName MaterialAlias, bool bPersist, FString& OutError);

private:
	void ApplyPersistedRuntimeMaterialAliases();

	TMap<FName, int32> ObjectAudioEventOccurrences;
};