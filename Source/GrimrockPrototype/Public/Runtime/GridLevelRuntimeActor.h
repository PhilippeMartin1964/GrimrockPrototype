#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "GridLevelRuntimeActor.generated.h"

class AGridEditorPreviewObjectActor;
class UGridObjectArchetypeAsset;
class UGridItemDefinitionAsset;
class AGridRuntimeObjectActor;
class AGridItemActor;
class AGridThrownItemActor;
class AGridReceptacleActor;
class AGridWallLockActor;
class AGridMonsterActor;
class UGridMonsterDefinitionAsset;
class UGridActivationComponent;
class UGridMonsterEncounterComponent;
class UGridDoorSystemComponent;
class UGridEditorPreviewComponent;
class UGridPlayerAttackPresentationComponent;
class UReadableMessageWidget;
class UUserWidget;

UENUM()
enum class EGridRuntimeRebuildMode : uint8
{
	Full,
	GeometryOnly
};

USTRUCT()
struct FGridSpawnedItemRuntimeEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FIntPoint Cell = FIntPoint::ZeroValue;

	UPROPERTY(Transient)
	EGridEdge Edge = EGridEdge::None;

	UPROPERTY(Transient)
	TObjectPtr<AGridItemActor> ItemActor;

	UPROPERTY(Transient)
	FGuid ObjectId;

	UPROPERTY(Transient)
	FName ItemArchetypeId = NAME_None;

	UPROPERTY(Transient)
	TObjectPtr<UGridItemDefinitionAsset> ItemDefinitionAsset = nullptr;

	UPROPERTY(Transient)
	FName ItemDefinitionId = NAME_None;

	UPROPERTY(Transient)
	int32 Quantity = 1;
};

UCLASS()
class GRIMROCKPROTOTYPE_API AGridLevelRuntimeActor : public AActor
{
	GENERATED_BODY()

public:
	AGridLevelRuntimeActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInstancedStaticMeshComponent* FloorISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInstancedStaticMeshComponent* WallISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInstancedStaticMeshComponent* CeilingISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGridActivationComponent> ActivationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGridMonsterEncounterComponent> MonsterEncounterComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGridDoorSystemComponent> DoorSystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGridEditorPreviewComponent> EditorPreviewComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGridPlayerAttackPresentationComponent> PlayerAttackPresentationComponent;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TObjectPtr<UGridLevelAsset> LevelAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	TObjectPtr<UGridDungeonAsset> DungeonAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	FName CurrentDungeonLevelId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon|Runtime")
	bool bIsExecutingDungeonTransition = false;

	UPROPERTY(Transient)
	FGridDungeonRuntimeState DungeonRuntimeState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> FloorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> WallMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> CeilingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
	TObjectPtr<UMaterialInterface> CeilingEditorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
	TSubclassOf<AGridEditorPreviewObjectActor> EditorPreviewObjectActorClass;

	UFUNCTION(BlueprintCallable, Category = "Editor Preview")
	void SetEditorHoveredObject(FGuid ObjectId);

	UFUNCTION(BlueprintCallable, Category = "Editor Preview")
	void SetEditorSelectedObject(FGuid ObjectId);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Editor Preview")
	void CleanupOrphanEditorPreviewObjects();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FVector GridOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime|Start")
	bool bApplyLevelStartOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bRebuildInConstruction = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bEnableRuntimeDebugLog = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bEnableRuntimeDebugScreen = false;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Debug")
	void LogRuntimeDebugSummary() const;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	FString GetRuntimeDebugSummary() const;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ShowRuntimeDebugSummary(float Duration = 3.f) const;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Level|Diagnostics")
	void LogLevelAssetDiagnostics() const;

	UFUNCTION(BlueprintCallable, Category = "Level|Diagnostics")
	FString GetLevelAssetDiagnostics() const;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Runtime|Diagnostics")
	void LogPIEReadinessDiagnostics() const;

	UFUNCTION(BlueprintCallable, Category = "Runtime|Diagnostics")
	FString GetPIEReadinessDiagnostics() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Object Archetypes")
	TArray<TObjectPtr<UGridObjectArchetypeAsset>> ObjectArchetypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UReadableMessageWidget> ReadableMessageWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowReadableMessage(const FText& MessageText);

	UFUNCTION(BlueprintPure, Category = "UI")
	bool HasActiveReadableMessage() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	bool DismissReadableMessage();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	bool bReadableMessageAutoHide = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (ClampMin = "0.1"))
	float ReadableMessageDuration = 4.0f;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideReadableMessage();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UReadableMessageWidget> InteractionFeedbackWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "UI", meta = (AdvancedDisplay = "DurationSeconds"))
	void ShowInteractionFeedback(const FText& MessageText, float DurationSeconds = 1.5f);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideInteractionFeedback();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Combat")
	TSubclassOf<UReadableMessageWidget> CombatFeedbackWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "UI|Combat")
	void ShowCombatFeedback(const FGridPlayerAttackFeedbackRequest& Feedback);

	UFUNCTION(BlueprintCallable, Category = "UI|Combat")
	void HideCombatFeedback();

	UFUNCTION(BlueprintPure, Category = "Combat|Player Attack|Presentation")
	UGridPlayerAttackPresentationComponent* GetPlayerAttackPresentationComponent() const
	{
		return PlayerAttackPresentationComponent;
	}

public:
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Level")
	void RebuildLevel(EGridRuntimeRebuildMode RebuildMode = EGridRuntimeRebuildMode::Full);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Level")
	void ClearVisuals(EGridRuntimeRebuildMode RebuildMode = EGridRuntimeRebuildMode::Full);

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	FVector GetCellCenterWorld(int32 X, int32 Y, float ZOffset = 0.f) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	bool IsValidCell(int32 X, int32 Y) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	const FGridLevelCellData& GetCell(int32 X, int32 Y) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	bool IsWalkableCell(int32 X, int32 Y) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	bool TryGetNeighborCell(int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	EGridWallType GetWallOnEdge(int32 X, int32 Y, EGridEdge Edge) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime")
	bool CanMove(int32 FromX, int32 FromY, EGridEdge Direction) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime|Perception")
	bool CanSoundTraverse(int32 FromX, int32 FromY, EGridEdge Direction) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime|Rendering")
	bool ShouldHideCellFloor(int32 CellX, int32 CellY) const;

	UFUNCTION(BlueprintPure, Category = "Runtime|Diagnostics")
	int32 GetRuntimeObjectRebuildGeneration() const
	{
		return RuntimeObjectRebuildGeneration;
	}

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool HasDoorOnEdge(int32 X, int32 Y, EGridEdge Edge) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool IsDoorOpenOnEdge(int32 X, int32 Y, EGridEdge Edge) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool ToggleDoorOnEdge(int32 X, int32 Y, EGridEdge Edge);

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool OpenDoorOnEdge(int32 X, int32 Y, EGridEdge Edge);

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool CloseDoorOnEdge(int32 X, int32 Y, EGridEdge Edge);

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool TryInteractAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn);

	bool CanPartyInteractWithEdgeObject(int32 ObjectCellX, int32 ObjectCellY, EGridEdge ObjectEdge, const AGrimrockPartyPawn* PartyPawn) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	AGridReceptacleActor* FindReceptacleAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	AGridWallLockActor* FindWallLockAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge) const;

	/**
	 * Maximum horizontal reach for free world pickups (Edge=None).
	 * Free pickups may be taken from the party cell or one traversable cardinal neighbour.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime|Interaction", meta = (ClampMin = "0.0"))
	float WorldItemPickupReach = 210.0f;

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool TryPickupItemAtCell(int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn);

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool TryPickupItemActor(AGridItemActor* ItemActor, AGrimrockPartyPawn* PartyPawn);

	UFUNCTION(BlueprintCallable, Category = "Inventory|World")
	bool TryDropItemInstanceAtCell(const FGridItemInstance& ItemInstance, int32 CellX, int32 CellY, EGridEdge Edge, const FVector& LocalOffset);

	/** MON8 overload: a loot table may provide its definition directly. */
	bool TryDropItemInstanceAtCell(const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinitionAsset, int32 CellX, int32 CellY,
		EGridEdge Edge, const FVector& LocalOffset);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Throw")
	bool TrySpawnThrownItemProjectile(
		const FGridItemInstance& ItemInstance, const FVector& StartWorldLocation, const FVector& LaunchVelocity, int32 SourceCellX, int32 SourceCellY);

	/** C++ path used by combat presentation when the definition is already resolved. */
	AGridThrownItemActor* SpawnThrownItemProjectile(const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinition,
		const FVector& StartWorldLocation, const FVector& LaunchVelocity, int32 SourceCellX, int32 SourceCellY);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Throw")
	bool TryResolveWorldCellFromImpactPoint(const FVector& WorldPoint, int32& OutCellX, int32& OutCellY, FVector& OutLocalOffset) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|World")
	float GetWorldItemWeightAtCell(int32 CellX, int32 CellY, bool bIncludeEdgeItems = false) const;

	UFUNCTION(BlueprintPure, Category = "Runtime|Interaction")
	bool IsPartyOnCell(int32 CellX, int32 CellY) const;

	/** Applies MON7 metadata to a monster initialized from a LevelAsset placement. */
	void ApplyMonsterPlacementMetadata(AGridMonsterActor* Monster) const;

	/** Resolves the strict MON13.2 runtime contract without creating an Actor. */
	bool ResolveMonsterSpawn(const FGridLevelObjectData& ObjectData, UGridMonsterDefinitionAsset*& OutDefinition, TSubclassOf<AGridMonsterActor>& OutActorClass,
		FString& OutError) const;

	/** Builds the authoritative cell-centered transform from InitialFacing. */
	bool GetMonsterSpawnTransform(const FGridLevelObjectData& ObjectData, FTransform& OutTransform) const;

	AGridMonsterActor* FindSpawnedMonsterActor(const FGuid& SpawnId) const;

	int32 GetSpawnedMonsterActorCount() const;
	int32 GetMonsterSpawnFailureCount() const
	{
		return RuntimeMonsterSpawnFailureCount;
	}

	/** Executes a MON13.3 lifecycle command for one persistent MonsterSpawn. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Spawn")
	bool ExecuteMonsterSpawnCommand(FGuid SpawnId, EGridObjectCommand Command);

	/** Starts or resumes the persistent MON13.4 encounter owned by this anchor. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Encounter")
	bool StartMonsterEncounter(FGuid AnchorSpawnId);

	UFUNCTION(BlueprintPure, Category = "Monster|Encounter")
	bool IsMonsterEncounterCompleted(FName EncounterGroupId) const;

	UFUNCTION(BlueprintPure, Category = "Monster|Encounter")
	int32 GetMonsterEncounterActiveWave(FName EncounterGroupId) const;

	/** Called only after a MonsterSpawn has committed a genuine death. */
	void NotifyMonsterEncounterDeath(FGuid SpawnId);

	/** Atomically moves a spawned monster to a validated intra-level pose. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Spawn")
	bool TeleportSpawnedMonster(FGuid SpawnId, int32 TargetCellX, int32 TargetCellY, EGridEdge TargetFacing);

	UFUNCTION(BlueprintCallable, Category = "Monster|Persistence")
	void SetMonsterRuntimeLevelActive(AGridMonsterActor* Monster, bool bActive);

	bool CanPartyPickupItemActor(const AGridItemActor* ItemActor, const AGrimrockPartyPawn* PartyPawn) const;
	bool CanPartyPickupItemEntry(const FGridSpawnedItemRuntimeEntry& Entry, const AGrimrockPartyPawn* PartyPawn, bool bLogRejection = true) const;

	// Allows runtime objects such as Receptacles to trigger their outgoing links.
	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	bool ExecuteLinksFromRuntimeObject(FGuid SourceObjectId, EGridObjectEvent SourceEvent);

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	void HandlePartyCellChanged(int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY);

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	void NotifyPawnEnteredCell(int32 CellX, int32 CellY);

	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	void NotifyPawnExitedCell(int32 CellX, int32 CellY);

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Runtime")
	bool TravelToDungeonLevel(FName TargetLevelId, int32 TargetCellX, int32 TargetCellY, EGridEdge TargetFacing, AGrimrockPartyPawn* PartyPawn);

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Runtime")
	bool CaptureCurrentLevelRuntimeState();

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Runtime")
	bool ApplyCurrentLevelRuntimeState();

	FGridLevelRuntimeState* GetOrCreateRuntimeStateForCurrentLevel();
	const FGridLevelRuntimeState* FindRuntimeStateForCurrentLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Runtime")
	bool TryExecuteTransitionAtCell(int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn, bool bTriggeredByUseAction);

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Runtime")
	bool FindTransitionAtCell(int32 CellX, int32 CellY, bool bTriggeredByUseAction, FGridObjectTransitionParams& OutTransition) const;

	void RebuildRuntimeObjects();
	void AddRuntimeObjectActor(const FGridLevelObjectData& ObjectData);
	void AddPlacedItemActor(const FGridLevelObjectData& ObjectData);
	bool IsRuntimeSpawnableObject(const FGridLevelObjectData& ObjectData) const;

	template <typename T> T* FindRuntimeObjectActor(const FGuid& ObjectId) const
	{
		if (!ObjectId.IsValid())
		{
			return nullptr;
		}
		if (const TObjectPtr<AGridRuntimeObjectActor>* ActorPtr = SpawnedRuntimeObjectActors.Find(ObjectId))
		{
			return Cast<T>(ActorPtr->Get());
		}
		return nullptr;
	}

	UStaticMesh* GetObjectMesh(const FGridLevelObjectData& ObjectData) const;
	UMaterialInterface* GetObjectMaterial(const FGridLevelObjectData& ObjectData) const;
	bool GetObjectPlacementTransform(const FGridLevelObjectData& ObjectData, FTransform& OutTransform) const;
	const UGridObjectArchetypeAsset* FindObjectArchetype(FName ArchetypeId) const;
	UGridItemDefinitionAsset* ResolveRuntimeItemDefinition(FName ItemDefinitionId) const;
	AGridItemActor* SpawnItemActorForDefinition(UGridItemDefinitionAsset* ItemDefinition, FName ItemDefinitionId, AActor* OwnerActor,
		USceneComponent* AttachParent, TSubclassOf<AGridItemActor> PreferredItemActorClass = nullptr) const;

protected:
	FVector CellToWorld(int32 X, int32 Y, float ZOffset = 0.f) const;

	void AddFloor(int32 X, int32 Y, float CellSize);
	void AddCeiling(int32 X, int32 Y, float CellSize);
	void AddEdgeInstance(UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize);
	bool ShouldSuppressStandardWallForEdge(int32 X, int32 Y, EGridEdge Edge) const;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class UGridMonsterEncounterComponent;

	void GetEdgeTransform(int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const;
	bool TryGetOppositeEdge(int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge) const;
	bool TryResolveDoorEdge(int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge, bool& bOutResolvedOpposite) const;

	TSubclassOf<AGridRuntimeObjectActor> GetObjectRuntimeActorClass(const FGridLevelObjectData& ObjectData) const;

	bool GetWallMountedObjectTransform(const FGridLevelObjectData& ObjectData, float ZOffset, float WallInset, float LocalOffsetAlongWall,
		float LocalOffsetVertical, FTransform& OutTransform) const;
	bool GetFloorEdgeObjectTransform(const FGridLevelObjectData& ObjectData, float ZOffset, float EdgeInset, FTransform& OutTransform) const;
	bool GetCenteredObjectTransform(const FGridLevelObjectData& ObjectData, float ZOffset, FTransform& OutTransform) const;

	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<AGridRuntimeObjectActor>> SpawnedRuntimeObjectActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGridItemActor>> SpawnedItemActors;

	UPROPERTY(Transient)
	TArray<FGridSpawnedItemRuntimeEntry> SpawnedItemEntries;

	/** MON13.2 Actors owned by LevelAsset MonsterSpawn placements. */
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<AGridMonsterActor>> SpawnedMonsterActors;

	int32 RuntimeObjectRebuildGeneration = 0;
	int32 RuntimeMonsterSpawnFailureCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<UReadableMessageWidget> ActiveReadableMessageWidget;

	FTimerHandle ReadableMessageTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UReadableMessageWidget> ActiveInteractionFeedbackWidget;

	FTimerHandle InteractionFeedbackTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UReadableMessageWidget> ActiveCombatFeedbackWidget;

	FTimerHandle CombatFeedbackTimerHandle;

	static bool IsSafeRuntimeRenderTransform(const FTransform& Transform);
	void LogUnsafeInstanceTransform(
		const TCHAR* FunctionName, const UInstancedStaticMeshComponent* Component, int32 X, int32 Y, EGridEdge Edge, const FTransform& Transform) const;
	void LogUnsafeObjectTransform(
		const TCHAR* FunctionName, const FGridLevelObjectData& ObjectData, const UStaticMesh* StaticMesh, const FTransform& Transform) const;
	void LogUnsafeItemTransform(const TCHAR* FunctionName, FName ArchetypeId, const AActor* OwnerActor, const USceneComponent* AttachParent,
		const UStaticMesh* StaticMesh, const FTransform& Transform) const;

	void RegisterRuntimeObjectActor(const FGuid& ObjectId, AGridRuntimeObjectActor* Actor);
	void ClearRuntimeObjectActors();
	AGridMonsterActor* AddMonsterSpawnActor(const FGridLevelObjectData& ObjectData, const FGridRuntimeMonsterState* RestoreState = nullptr);
	bool DespawnMonsterSpawnActor(const FGridLevelObjectData& ObjectData, bool bRememberState, bool bEmitEvent);
	bool StoreMonsterPlacementState(const FGridLevelObjectData& ObjectData, AGridMonsterActor* Monster, bool bIsSpawned);
	void ClearSpawnedMonsterActors();
	void AbortActiveCombatAndMonsterActions();
	void ApplyInitialMonsterStateForCurrentLevel();

	template <typename TActor>
	TActor* SpawnRuntimeObjectActor(const FGridLevelObjectData& ObjectData, UStaticMesh*& OutMesh, UMaterialInterface*& OutMaterial, FTransform& OutTransform)
	{
		static_assert(TIsDerivedFrom<TActor, AGridRuntimeObjectActor>::IsDerived, "TActor must derive from AGridRuntimeObjectActor");
		OutMesh = nullptr;
		OutMaterial = nullptr;
		OutTransform = FTransform::Identity;

		if (!LevelAsset)
		{
			return nullptr;
		}
		OutMesh = GetObjectMesh(ObjectData);
		OutMaterial = GetObjectMaterial(ObjectData);
		TSubclassOf<AGridRuntimeObjectActor> ActorClass = GetObjectRuntimeActorClass(ObjectData);

		if (!ActorClass || !OutMesh)
		{
			return nullptr;
		}
		UWorld* World = GetWorld();
		if (!World)
		{
			return nullptr;
		}
		if (!GetObjectPlacementTransform(ObjectData, OutTransform))
		{
			return nullptr;
		}
		if (!IsSafeRuntimeRenderTransform(OutTransform))
		{
			LogUnsafeObjectTransform(TEXT("SpawnRuntimeObjectActor"), ObjectData, OutMesh, OutTransform);
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		TActor* Actor = World->SpawnActor<TActor>(ActorClass, OutTransform.GetLocation(), OutTransform.GetRotation().Rotator(), Params);
		if (!Actor)
		{
			return nullptr;
		}
		RegisterRuntimeObjectActor(ObjectData.ObjectId, Actor);
		return Actor;
	}
};
