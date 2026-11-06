#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridVfxProfileTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "TimerManager.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "Runtime/GridPlacementTransformResolver.h"
#include "GridLevelRuntimeActor.generated.h"

class AGridEditorPreviewObjectActor;
class UMaterialInstanceDynamic;
class UGridItemDefinitionAsset;
class AGridRuntimeObjectActor;
class AGridItemActor;
class AGridThrownItemActor;
class AGridPitTrapdoorActor;
class AGridReceptacleActor;
class AGridWallLockActor;
class AGridMonsterActor;
class UGridMonsterDefinitionAsset;
class UGridActivationComponent;
class UGridMonsterEncounterComponent;
class UGridDoorSystemComponent;
class UGridEditorPreviewComponent;
class UGridPlayerAttackPresentationComponent;
class UDecalComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UNiagaraComponent;
class UGridSurfaceVfxProfileAsset;
class UGridCatalystVfxProfileAsset;
class UReadableMessageWidget;
class UUserWidget;
struct FGridResolvedCombatModifiers;

/** Visual variant; authoritative RPG03.6 surface outcomes stay unchanged. */
enum class EGridSurfaceCatalystVfxKind : uint8 { Fizzle, OilIgnition, PoisonBurst, IceMelt };

/** Visual metadata only; all authoritative C6 outcomes are committed separately. */
struct FGridCatalystVisualCell
{
	FVector Center = FVector::ZeroVector;
	EGridSurfaceCatalystVfxKind Kind = EGridSurfaceCatalystVfxKind::Fizzle;
	float WaveDelay = 0.f;
	int32 FirstSpark = 0;
	int32 SparkCount = 0;
};

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
	TObjectPtr<UGridItemDefinitionAsset> ItemDefinitionAsset = nullptr;

	UPROPERTY(Transient)
	FName ItemDefinitionId = NAME_None;

	UPROPERTY(Transient)
	int32 Quantity = 1;

	/** Fixed item level of this physical world instance (0 for Legacy items). */
	UPROPERTY(Transient)
	int32 ItemLevel = 0;
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
	bool bIsExecutingRelocation = false;

	UPROPERTY(Transient)
	FGridDungeonRuntimeState DungeonRuntimeState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> FloorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> WallMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> CeilingMesh;

	/** Matériau de décalque projeté sur la vraie surface du sol à Z=0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Surfaces|Visuel")
	TSoftObjectPtr<UMaterialInterface> OilSurfaceMaterial;

	/** Décalques transitoires : aucune géométrie flottante ni sauvegarde visuelle. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDecalComponent>> ActiveOilDecals;

	/** Brève projection animée au point d'impact, indépendante des flaques durables. */
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> ActiveOilSplashDecal;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ActiveOilSplashMaterial;

	/** ARMEMENT01.17.1.5C: real 3D droplets and satellite decal spatters. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ActiveOilSplashDroplets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDecalComponent>> ActiveOilSplashSpatters;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveOilSplashSpatterMaterials;


	/** ARMEMENT01.17.2C: presentation-only components, never serialized. */
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> ActiveCatalystPulse;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ActiveCatalystPulseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> ActiveCatalystLight;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ActiveCatalystSparks;

	/** 17.3C: one moving bottle, with independent transient components per affected cell. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ActiveCatalystProjectile;

	/** Moving ember trail on the same 30Hz timer as the physical bottle. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ActiveCatalystTrailSparks;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDecalComponent>> ActiveCatalystCellPulses;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveCatalystCellPulseMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> ActiveCatalystCellLights;

	/** One shared fire profile; never read visual presentation from the level DataAsset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|VFX|Fire")
	TSoftObjectPtr<UGridSurfaceVfxProfileAsset> FireVfxProfile;

	/** One independent, reusable presentation profile for catalyst effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|VFX|Catalyst")
	TSoftObjectPtr<UGridCatalystVfxProfileAsset> CatalystVfxProfile;

	/** Preview-only settings from the assigned profile (no gameplay authority). */
	FGridFireVfxSettings ResolveFireVfxSettings() const;
	FGridCatalystVfxSettings ResolveCatalystVfxSettings() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> ActiveFireNiagaraComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> ActiveFireSurfaceLights;

	/** Wet floor after a canonical Ice->Water reaction; presentation only. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDecalComponent>> ActiveWaterSurfaceDecals;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveWaterSurfaceMaterials;

	FTimerHandle CatalystVisualTimer;
	float CatalystVisualElapsedSeconds = 0.f;
	float CatalystImpactElapsedStart = 0.f;
	FVector CatalystVisualCenter = FVector::ZeroVector;
	float CatalystVisualCellSize = 200.f;
	EGridSurfaceCatalystVfxKind CatalystVisualKind = EGridSurfaceCatalystVfxKind::Fizzle;
	int32 CatalystPresentationCount = 0;
	int32 CatalystImpactCount = 0;
	bool bCatalystVisualsDeferred = false;
	bool bCatalystImpactStarted = false;
	FVector CatalystFlightStart = FVector::ZeroVector;
	FVector CatalystFlightEnd = FVector::ZeroVector;
	TArray<FGridCatalystVisualCell> ActiveCatalystVisualCells;

	FTimerHandle OilSplashTimer;
	float OilSplashElapsedSeconds = 0.f;
	int32 OilSplashPresentationCount = 0;
	FVector OilSplashImpactCenter = FVector::ZeroVector;
	float OilSplashImpactCellSize = 200.f;

	/** The splash is presentation only; does not modify RPG03.6 surfaces. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Surface|Oil|Splash",
		meta = (ClampMin = "0.4", ClampMax = "2.0"))
	float OilSplashDurationSeconds = 0.78f;

	/** Temps de gameplay équivalent à un round, réglable avant la partie. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Surface|Oil",
		meta = (ClampMin = "0.1", UIMin = "1.0", UIMax = "30.0"))
	float OilExplorationSecondsPerRound = 6.f;

	/** ARMEMENT01.17.3B — configurable, specific to catalysed Fire (other RPG03.6 reactions stay untouched). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Surface|Catalyst|Balance", meta = (ClampMin = "0", ClampMax = "1000"))
	int32 CatalystFireDamagePerRound = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Surface|Catalyst|Balance", meta = (ClampMin = "0", ClampMax = "1000"))
	int32 CatalystPoisonBurstDamage = 6;

	FTimerHandle OilExplorationTimer;
	double LastOilExplorationUpdateTime = 0.0;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Object Definitions")
	TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> WorldObjectDefinitions;

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

	bool BuildInitialRuntimeState();

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

	/** A linked, still-closed floor trap hides its PIT03, not the normal floor. */
	bool IsPitConcealedByFloorTrap(const FGridWorldObjectInstance& Pit) const;
	void RefreshConcealedPitCellFloor(const FGridWorldObjectInstance& Pit);

	UFUNCTION(BlueprintCallable, Category = "Runtime|Rendering")
	bool ShouldHideCellCeiling(int32 CellX, int32 CellY) const;

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
	 * Canonical horizontal hand reach for free world pickup, cursor placement and inventory/hotbar targeting.
	 * The serialized property name is retained for Blueprint compatibility.
	 * A visible target at or below this distance is manipulated by hand; farther cursor targets use throwing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime|Interaction", meta = (ClampMin = "0.0"))
	float WorldItemPickupReach = 200.0f;

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
		const FVector& StartWorldLocation, const FVector& LaunchVelocity, int32 SourceCellX, int32 SourceCellY,
		bool bRangedAmmunitionProjectile = false, AActor* LaunchSourceActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Throw")
	bool TryResolveWorldCellFromImpactPoint(const FVector& WorldPoint, int32& OutCellX, int32& OutCellY, FVector& OutLocalOffset) const;

	/**
	 * Spawn a recovering projectile pickup at its actual airborne mesh pose.
	 * Chaos then owns its fall; never snap it to cell center or reset its yaw.
	 */
	bool TryDropItemInstanceAtWorldTransform(const FGridItemInstance& ItemInstance,
		UGridItemDefinitionAsset* ItemDefinition, const FTransform& WorldMeshTransform);

	/** Reconcile physics-driven mesh locations with pickup grid registration. */
	void RefreshWorldItemPhysicsCells();

	UFUNCTION(BlueprintCallable, Category = "Inventory|World")
	float GetWorldItemWeightAtCell(int32 CellX, int32 CellY, bool bIncludeEdgeItems = false) const;

	UFUNCTION(BlueprintPure, Category = "Runtime|Interaction")
	bool IsPartyOnCell(int32 CellX, int32 CellY) const;

	/** Applies static authored metadata (encounter + patrol route) from the current LevelAsset placement. */
	void ApplyMonsterPlacementMetadata(AGridMonsterActor* Monster) const;

	/** Typed MON13.2 runtime contract. */
	bool ResolveMonsterSpawn(const FGridMonsterSpawnInstance& SpawnData, UGridMonsterDefinitionAsset*& OutDefinition,
		TSubclassOf<AGridMonsterActor>& OutActorClass, FString& OutError) const;

	/** Typed authoritative cell-centered transform. */
	bool GetMonsterSpawnTransform(const FGridMonsterSpawnInstance& SpawnData, FTransform& OutTransform) const;

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

	/** Generic out-of-combat sabotage target query. The caller owns the Skill Check. */
	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction|Sabotage")
	bool GetRuntimeObjectSabotageDifficulty(FGuid ObjectId, int32& OutDifficulty) const;

	/** Emits only the generic Sabotaged event after a caller has resolved a successful check. */
	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction|Sabotage")
	bool ExecuteRuntimeObjectSabotage(FGuid ObjectId);

    /** Authored Trigger trap state; never confuses combat-cell traps with dungeon triggers. */
    bool GetTriggerTrapDisarmDifficulty(FGuid TrapObjectId, int32& OutDifficulty) const;

    /** TRAP01.9: discovered-trap hint using this placement's effective DC.
     * Hidden/unknown/neutralized traps never disclose the number.
     */
    FText GetDungeonTrapDisarmHint(FGuid TrapObjectId) const;

    /** TRAP01.9.1: readonly, RNG-free estimate using the current party.
     * Available for a known, disarmable trap when an actual specialist with
     * their own tools exists. No position or combat-state promises.
     */
    bool TryGetDungeonTrapDisarmOdds(
        const AGrimrockPartyPawn* PartyPawn, FGuid TrapObjectId,
        struct FGridDungeonTrapDisarmOdds& OutOdds) const;
    bool IsTriggerTrapDisarmed(FGuid TrapObjectId) const;
    /** Returns only already-known trap instance IDs (TRAP01.2 will author detection). */
    bool IsTriggerTrapDetected(FGuid TrapObjectId) const;

    /** TRAP01.1 authoritative state query for one authored dungeon trap. */
    EGridDungeonTrapState GetDungeonTrapState(FGuid TrapObjectId) const;

    /** Explicitly records the first successful perception discovery.
     * Proximity alone is NEVER a discovery. TRAP01.2 will call this.
     * Does not reveal a physical mesh or consume the trap.
     */
    bool TryRecordDungeonTrapDetection(FGuid TrapObjectId);

    /** TRAP01.2 one event-driven scan, with caller-owned deterministic RNG.
     * Looks only on cardinal sight lines, up to 4 cells, honoring walls/doors.
     * Returns number of genuinely new trap discoveries.
     */
    int32 EvaluatePassiveDungeonTrapPerception(AGrimrockPartyPawn* PartyPawn,
        FRandomStream& RandomStream);

    /** TRAP-SEARCH01.1: an explicit, separately persisted one-roll-per-distance
     * search of nearby authored traps. Never called from movement or Tick;
     * caller owns the RNG and the next milestone owns input/UI arbitration.
     * Returns discoveries, not number of candidates or attempted checks.
     */
    int32 EvaluateActiveDungeonTrapSearch(AGrimrockPartyPawn* PartyPawn,
        FRandomStream& RandomStream);

    /** Data-owned trap reveal; creates optional runtime visual only for
     * detected/triggered/disarmed states and reapplies it after Save/Continue. */
    void RefreshDungeonTrapRevealVisual(FGuid TrapObjectId);
    void RestoreDungeonTrapRevealVisuals();

    /** TRAP01.3 will call when the actual damage/activation commits.
     * TRAP01.2 publishes the reveal and position, not damage. */
    bool TryRecordDungeonTrapTriggered(FGuid TrapObjectId, AGrimrockPartyPawn* PartyPawn);

    /** TRAP01.5.1: clickable revealed trap, using carried tools and the
     * party-best Dexterity specialist. Returns true when interaction was
     * handled, including an informative refusal (spent/blocked/no tools).
     * Hidden traps never expose an interactable actor or an event.
     */
    bool TryInteractWithRevealedDungeonTrap(
        AGrimrockPartyPawn* PartyPawn, FGuid TrapObjectId);


    /** TRAP01.3: commit the OnEnterCell hazards authored in one cell.
     * One shared damage roll per hazard, then individual character defenses.
     * The caller supplies the RNG for deterministic UE automation. This call
     * requires the group to actually stand in the target cell and never runs
     * for a stable-cell initialization (caller enforces movement boundary).
     * Returns the number of hazard instances that fired.
     */
    int32 TriggerDungeonTrapsOnPartyEnterCell(
        AGrimrockPartyPawn* PartyPawn, int32 CellX, int32 CellY,
        FRandomStream& RandomStream);




    /** Selects exactly one disarmable dungeon Trigger in the party cell or the
     * traversable cell directly in front. Ambiguous targets fail closed. */
    bool FindDisarmableTriggerTrapForParty(const AGrimrockPartyPawn* PartyPawn,
        FGuid& OutTrapObjectId) const;


    /** TRAP01.4: CharacterIndex is the inventory tool's SOURCE owner, not
     * necessarily the performer. Select the strongest eligible living member
     * with their own Tool.Disarm and trained Skill_Traps; use Dexterity.
     * OutDisarmerCharacterIndex receives the real performer after a resolved
     * check, even on skill failure. The tool is never consumed.
     * Returns true for a resolved roll; rejects without RNG/state mutation.
     */
    bool TryDisarmTriggerTrap(AGrimrockPartyPawn* PartyPawn, FGuid TrapObjectId,
        int32 CharacterIndex, const class URPGSkillAsset* SkillDefinition,
        FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult,
        int32* OutDisarmerCharacterIndex = nullptr,
        bool* OutTriggeredOnFailure = nullptr);


	UFUNCTION(BlueprintCallable, Category = "Runtime|Interaction")
	void HandlePartyCellChanged(int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY);

	/** MON21.6.3 reveals the current map cell and topology-visible cardinal neighbours. */
	int32 RevealMapAroundCell(int32 CellX, int32 CellY);

	/** MON21.6.4 records durable-in-session knowledge of one actual secret door object. */
	bool TryDiscoverMapSecretDoor(FGuid ObjectId, bool& bOutNewlyDiscovered);
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

	/** RPG03.6 persistent cell-surface authority stored in the current level runtime state. */
	bool ApplyCombatSurfaceAtCell(int32 CellX, int32 CellY, const FGridCombatSurfaceEffectProfile& Profile,
		const FGuid& SourceCombatantId, FName SourceActionId, const FGridResolvedCombatModifiers& SourceModifiers);

	bool InteractCombatSurfaceAtCell(int32 CellX, int32 CellY, EGridCombatSurfaceInteraction Interaction,
		const FGridResolvedCombatModifiers& SourceModifiers, FGridCombatSurfaceReactionResult& OutReaction);

	bool ConvertCombatSurfaceAtCell(int32 CellX, int32 CellY, const FGridCombatSurfaceConversionProfile& Profile,
		const FGuid& SourceCombatantId, FName SourceActionId, const FGridResolvedCombatModifiers& SourceModifiers);

	const FGridCombatSurfaceState* FindCombatSurfaceAtCell(int32 CellX, int32 CellY) const;

	/** Additional movement budget charged when entering the current surface cell. */
	int32 GetCombatSurfaceTraversalCostModifierAtCell(int32 CellX, int32 CellY) const;

	void GetCurrentCombatSurfaceSnapshot(TMap<FIntPoint, FGridCombatSurfaceState>& OutSurfaces) const;

	int32 AdvanceCombatSurfaceRound(TArray<FIntPoint>* OutExpiredCells = nullptr);

	/** Atomic, prevalidated multi-cell catalyst commit; no partial mutation or visual resets. */
	bool CommitCatalystAreaPlan(
		const TArray<struct FGridCatalystCellSnapshot>& Snapshot,
		const TArray<struct FGridCatalystCellOutcome>& Outcomes,
		const FGuid& SourceCombatantId, FName SourceActionId);

	/** Applies the catalyst's authored periodic Fire overlay after a combat C6 reaction. */
	bool ConfigureCatalystFireAtCell(int32 CellX, int32 CellY,
		const FGuid& SourceCombatantId, FName SourceActionId);

	/** Advances all temporary RPG03.6 surfaces outside combat (legacy name kept for compatibility). */
	int32 AdvanceExplorationOilTime(float ElapsedSeconds, TArray<FIntPoint>* OutExpiredCells = nullptr);
	void RefreshExplorationOilTimer();
	void UpdateExplorationOilTimer();
	void ClearExplorationOilTimer();

	/** Projette l'huile sur la géométrie réelle du sol (Z=0), sans plan superposé. */
	void RefreshCombatOilSurfaceVisuals();
	void ClearCombatOilSurfaceVisuals();
	/** Présentation facultative après résolution d'une vraie action de flasque. */
	void PlayCombatOilSplashAtCell(int32 CellX, int32 CellY);
	void UpdateCombatOilSplash();
	void ClearCombatOilSplash();

	static EGridSurfaceCatalystVfxKind ResolveCatalystVfxKind(
		EGridCombatSurfaceType InputType, const FGridCombatSurfaceReactionResult& Reaction);
	void PlaySurfaceCatalystReactionAtCell(int32 CellX, int32 CellY,
		EGridCombatSurfaceType InputType, const FGridCombatSurfaceReactionResult& Reaction);

	/** Defers only visual refresh during the single logical gameplay transaction. */
	void BeginCatalystSurfaceVisualBatch();
	void CancelCatalystSurfaceVisualBatch();
	/** One bottle/flight, then independent Oil/Poison/Ice/Fizzle effects in the area. */
	void PlaySurfaceCatalystArea(FIntPoint LaunchCell, FIntPoint ImpactCell,
		const TArray<struct FGridCatalystCellOutcome>& Outcomes);
	void BeginCatalystImpactVfx();
	void UpdateCatalystReactionVfx();
	void ClearCatalystReactionVfx();
	void RefreshFireSurfaceVisuals();
	void ClearFireSurfaceVisuals();
	void RefreshWaterSurfaceVisuals();
	void ClearWaterSurfaceVisuals();
	int32 GetActiveFireNiagaraCount() const { return ActiveFireNiagaraComponents.Num(); }
	int32 GetActiveWaterSurfaceDecalCount() const { return ActiveWaterSurfaceDecals.Num(); }
	int32 GetCatalystPresentationCount() const { return CatalystPresentationCount; }
	int32 GetCatalystImpactCount() const { return CatalystImpactCount; }
	int32 GetActiveCatalystProjectileCount() const { return IsValid(ActiveCatalystProjectile.Get()) ? 1 : 0; }
	int32 GetActiveCatalystTrailCount() const { return ActiveCatalystTrailSparks.Num(); }
	int32 GetActiveCatalystCellPulseCount() const { return ActiveCatalystCellPulses.Num(); }
	int32 GetActiveCatalystSparkCount() const { return ActiveCatalystSparks.Num(); }
	EGridSurfaceCatalystVfxKind GetCatalystVisualKind() const { return CatalystVisualKind; }

	int32 GetOilSplashPresentationCount() const { return OilSplashPresentationCount; }
	int32 GetActiveOilSplashCount() const { return ActiveOilSplashDecal.Get() ? 1 : 0; }
	int32 GetActiveOilSplashDropletCount() const { return ActiveOilSplashDroplets.Num(); }
	int32 GetActiveOilSplashSpatterCount() const { return ActiveOilSplashSpatters.Num(); }
	const UStaticMeshComponent* GetOilSplashDroplet(int32 Index) const
	{
		return ActiveOilSplashDroplets.IsValidIndex(Index) ? ActiveOilSplashDroplets[Index].Get() : nullptr;
	}
	static float GetOilSplashSpreadScale(float NormalizedProgress);
	int32 GetActiveOilDecalCount() const { return ActiveOilDecals.Num(); }
	const UDecalComponent* GetActiveOilDecal(int32 Index) const
	{
		return ActiveOilDecals.IsValidIndex(Index) ? ActiveOilDecals[Index].Get() : nullptr;
	}

	/** RPG03.9 generic temporary combat trap authority. */
	bool ApplyCombatTrapAtCell(int32 CellX, int32 CellY, const FGridCombatTrapEffectProfile& Profile,
		const FGuid& SourceCombatantId, FName SourceActionId, int32 ResolvedRawDamage);
	const FGridCombatTrapState* FindCombatTrapAtCell(int32 CellX, int32 CellY) const;
	bool ConsumeCombatTrapAtCell(int32 CellX, int32 CellY, FGridCombatTrapState& OutState);
	int32 AdvanceCombatTrapRound(TArray<FIntPoint>* OutExpiredCells = nullptr);

	/** Smoke blocks only cells strictly between endpoints; an occupant in smoke instead receives the authored ranged Evasion bonus. */
	bool DoesCombatSmokeBlockLine(const FIntPoint& FromCell, const FIntPoint& ToCell) const;
	bool IsCombatSmokeAtCell(int32 CellX, int32 CellY) const;

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Runtime")
	bool TryExecuteRelocationAtCell(int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn);

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Runtime")
	bool FindRelocationAtCell(int32 CellX, int32 CellY, FGridRelocationBehaviorParams& OutRelocation) const;

	/** PIT01: resolves an enabled static open pit without using the generic relocation path. */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|Pit")
	bool FindOpenPitAtCell(int32 CellX, int32 CellY, FGridRelocationBehaviorParams& OutRelocation) const;

	/** PIT01: validates the destination and starts the party-owned fall presentation. */
	UFUNCTION(BlueprintCallable, Category = "Dungeon|Pit")
	bool TryBeginPitFallAtCell(int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn);

	/** PIT03 runtime authority. Open=true means the cell behaves as a fall-through pit. */
	UFUNCTION(BlueprintPure, Category = "Dungeon|Pit")
	bool IsPitOpen(FGuid PitObjectId) const;

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Pit")
	bool SetPitOpen(FGuid PitObjectId, bool bOpen, bool bEmitEvent = true);

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Pit")
	bool TogglePit(FGuid PitObjectId, bool bEmitEvent = true);

	/** PIT02: routes a normal world item through an open pit instead of leaving it on the source level. */
	bool TryRouteWorldItemThroughOpenPit(
		const FGridItemInstance& ItemInstance, UGridItemDefinitionAsset* ItemDefinitionAsset, int32 CellX, int32 CellY, const FVector& LocalOffset);

	/** PIT02: materializes queued arrivals for the current dungeon level. Returns the number successfully spawned. */
	int32 ApplyPendingInboundItemsForCurrentLevel();

	/** PIT03: routes already placed center World Items when a closed trapdoor opens underneath them. */
	int32 DropWorldItemsThroughOpenPitAtCell(int32 CellX, int32 CellY);

	void RebuildRuntimeObjects();
	void AddRuntimeObjectActor(const FGridWorldObjectInstance& ObjectData);
	void AddPlacedItemActor(const FGridLooseItemInstance& ObjectData);
	bool IsRuntimeSpawnableObject(const FGridWorldObjectInstance& ObjectData) const;

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

	const UGridWorldObjectDefinitionAsset* FindWorldObjectDefinition(FName WorldObjectDefinitionId) const;
	UGridItemDefinitionAsset* ResolveRuntimeItemDefinition(FName ItemDefinitionId) const;
	AGridItemActor* SpawnItemActorForDefinition(UGridItemDefinitionAsset* ItemDefinition, FName ItemDefinitionId, AActor* OwnerActor,
		USceneComponent* AttachParent) const;

protected:
	FVector CellToWorld(int32 X, int32 Y, float ZOffset = 0.f) const;

	void AddFloor(int32 X, int32 Y, float CellSize);
	void AddCeiling(int32 X, int32 Y, float CellSize);
	void AddEdgeInstance(UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize);
	bool ShouldSuppressStandardWallForEdge(int32 X, int32 Y, EGridEdge Edge) const;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    /** One topology/discovery pipeline for passive and active scanning. */
    int32 EvaluateDungeonTrapPerception(
        AGrimrockPartyPawn* PartyPawn, FRandomStream& RandomStream, bool bActiveSearch);

    /** Shared hazard activation. Only validated entry or disarm-failure callers
     * may invoke this internal path; no direct player-facing GUID activation.
     */
    bool TryActivateDungeonTrap(
        AGrimrockPartyPawn* PartyPawn, FGuid TrapObjectId, FRandomStream& RandomStream);

	friend class UGridMonsterEncounterComponent;

	void GetEdgeTransform(int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const;
	bool TryGetOppositeEdge(int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge) const;
	bool TryResolveDoorEdge(int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge, bool& bOutResolvedOpposite) const;

	TSubclassOf<AGridRuntimeObjectActor> GetObjectRuntimeActorClass(const FGridWorldObjectInstance& ObjectData) const;

	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<AGridRuntimeObjectActor>> SpawnedRuntimeObjectActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGridItemActor>> SpawnedItemActors;

	UPROPERTY(Transient)
	TArray<FGridSpawnedItemRuntimeEntry> SpawnedItemEntries;

	/** MON13.2 Actors owned by LevelAsset MonsterSpawn placements. */
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<AGridMonsterActor>> SpawnedMonsterActors;

	/** PIT03.1 delayed connector emission until a trapdoor reaches its physical endpoint. */
	TMap<FGuid, bool> PendingPitEmitEvents;

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
		const TCHAR* FunctionName, const FGridWorldObjectInstance& ObjectData, const UStaticMesh* StaticMesh, const FTransform& Transform) const;
	void LogUnsafeItemTransform(const TCHAR* FunctionName, FName WorldObjectDefinitionId, const AActor* OwnerActor, const USceneComponent* AttachParent,
		const UStaticMesh* StaticMesh, const FTransform& Transform) const;

	void RegisterRuntimeObjectActor(const FGuid& ObjectId, AGridRuntimeObjectActor* Actor);
	void ClearRuntimeObjectActors();

	/** E2B typed monster runtime core. */
	AGridMonsterActor* AddMonsterSpawnActor(const FGridMonsterSpawnInstance& SpawnData, const FGridRuntimeMonsterState* RestoreState = nullptr);
	bool DespawnMonsterSpawnActor(const FGridMonsterSpawnInstance& SpawnData, bool bRememberState, bool bEmitEvent);
	bool StoreMonsterPlacementState(const FGridMonsterSpawnInstance& SpawnData, AGridMonsterActor* Monster, bool bIsSpawned);

	void ClearSpawnedMonsterActors();
	void AbortActiveCombatAndMonsterActions();
	void ApplyInitialMonsterStateForCurrentLevel();
	bool IsPitOpenForLevel(FName LevelId, const FGridWorldObjectInstance& PitObject) const;
	bool IsEffectivePitObject(const FGridWorldObjectInstance& ObjectData) const;
	bool ResolvePitLandingCell(FName TargetLevelId, int32 PreferredCellX, int32 PreferredCellY, int32& OutCellX, int32& OutCellY) const;
	void HandlePitTrapdoorAnimationFinished(FGuid PitObjectId, bool bWasOpen, bool bIsOpen);
	void FinalizePitGameplayStateChange(FGuid PitObjectId, bool bWasOpen, bool bIsOpen, bool bEmitEvent);

	template <typename TActor>
	TActor* SpawnRuntimeObjectActor(const FGridWorldObjectInstance& ObjectData, UStaticMesh*& OutMesh, FTransform& OutTransform)
	{
		static_assert(TIsDerivedFrom<TActor, AGridRuntimeObjectActor>::IsDerived, "TActor must derive from AGridRuntimeObjectActor");
		OutMesh = nullptr;
		OutTransform = FTransform::Identity;

		if (!LevelAsset)
		{
			return nullptr;
		}
		const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinition(ObjectData.WorldObjectDefinitionId);
		OutMesh = Definition && Definition->StaticPart.IsDefined() ? Definition->StaticPart.Mesh.Get() : nullptr;
		TSubclassOf<AGridRuntimeObjectActor> ActorClass = GetObjectRuntimeActorClass(ObjectData);

		// WORLDOBJ-MIG03: presentation is optional. Actor existence is driven by RuntimeActorClass,
		// while StaticPart/MovingParts (or no visual at all) are initialized after spawning.
		if (!ActorClass)
		{
			return nullptr;
		}
		UWorld* World = GetWorld();
		if (!World)
		{
			return nullptr;
		}
		if (!GridPlacementTransformResolver::ResolveWorldObject(*this, ObjectData, OutTransform))
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
		RegisterRuntimeObjectActor(ObjectData.InstanceId, Actor);
		return Actor;
	}
};
