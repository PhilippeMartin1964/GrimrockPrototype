#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridLuaScriptTypes.h"
#include "GridLevelVariableTypes.h"
#include "GridTypes.h"
#include "GridLevelPlacementTypes.h"
#include "GridLevelAsset.generated.h"

class UGridQuestDefinitionAsset;
class UNiagaraSystem;
class UGridSurfaceVfxProfileAsset;

/**
 * ARMEMENT01.17.4B: presentation-only settings authored IN the one UGridLevelAsset.
 * When bOverrideRuntimeVisuals is false, existing runtime actor values are authoritative.
 * Gameplay surfaces, duration, damage and saving are never derived from this struct.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridLevelSurfaceVfxSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX")
	bool bOverrideRuntimeVisuals = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire")
	bool bShowGroundFire = true;

	/** Choose any compatible Niagara system, e.g. licensed FireFX Ground Fire. Empty = legacy torch fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire")
	TSoftObjectPtr<UNiagaraSystem> GroundFireSystem;

	/** Artist-system world scaling, not a replacement for a Niagara spawn-area parameter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="0.01", ClampMax="5.0"))
	float GroundFireScale = 1.0f;

	/** Fraction of CellSize: 0.90 is a 180cm footprint on a 200cm cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="0.1", ClampMax="1.0"))
	float GroundFireCoverage = 0.90f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="5.0", ClampMax="300.0"))
	float FlameHeightCm = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="0.0", ClampMax="4.0"))
	float FlameDensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire")
	bool bEnableSmoke = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="0.0", ClampMax="4.0"))
	float SmokeDensity = .5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire")
	FLinearColor FireTint = FLinearColor(1.f, .43f, .08f);

	/** Cap the presentation cost on the 32x32 grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="0", ClampMax="128"))
	int32 MaxFireCells = 48;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="0", ClampMax="32"))
	int32 MaxFireLights = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="0.0", ClampMax="50000.0"))
	float FireLightIntensity = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire", meta=(ClampMin="0.1", ClampMax="5.0"))
	float FireLightRadiusCells = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Fire")
	FLinearColor FireLightColor = FLinearColor(1.f, .3f, .06f);

	/**
	 * Optional Niagara USER parameters. Match actual exposed names from the imported
	 * system's Parameters panel (including "User." prefix). None leaves that setting alone.
	 * SpawnArea takes Vector3 full-size cm (X,Y,Z=height); other sizes are floats.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Niagara Binding")
	FName SpawnAreaParameter = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Niagara Binding")
	FName FlameHeightParameter = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Niagara Binding")
	FName FlameDensityParameter = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Niagara Binding")
	FName SmokeDensityParameter = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Niagara Binding")
	FName FireTintParameter = NAME_None;

	/** Catalyst's existing ground pulse is retained, merely authored per level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Catalyst", meta=(ClampMin="0.1", ClampMax="1.5"))
	float CatalystFlightDurationSeconds = .42f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Catalyst", meta=(ClampMin="0.3", ClampMax="2.0"))
	float CatalystPulseDurationSeconds = .95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface VFX|Catalyst", meta=(ClampMin="0.0", ClampMax="0.3"))
	float CatalystWaveDelayPerCellSeconds = .085f;

	/** Defensive sanitization for programmatically authored assets and malformed data. */
	FGridLevelSurfaceVfxSettings Sanitized() const;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridLevelAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	// --- Grid size ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 Width = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 Height = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	float CellSize = 200.f;

	// --- Grid data ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TArray<FGridLevelCellData> Cells;

	/** Optional explicit visual profile for this level. Normal levels inherit the shared default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level|Surface VFX")
	TSoftObjectPtr<UGridSurfaceVfxProfileAsset> SurfaceVfxProfile;

	/** ARMEMENT01.17.4B compatibility: old inline settings, no longer preferred for authoring. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level|Surface VFX|Legacy", meta=(ShowOnlyInnerProperties, AdvancedDisplay))
	FGridLevelSurfaceVfxSettings SurfaceVfxSettings;

	// --- Gameplay ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Start")
	int32 StartCellX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Start")
	int32 StartCellY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Start")
	EGridEdge StartFacing = EGridEdge::North;

	UFUNCTION(BlueprintCallable, Category = "Gameplay|Start")
	bool IsStartCellValid() const;

	UFUNCTION(BlueprintCallable, Category = "Gameplay|Start")
	FIntPoint GetStartCell() const;


	/** Persistent reusable world-object placements. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements")
	TArray<FGridWorldObjectInstance> WorldObjectInstances;

	/** Collectibles physically present in the level. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements")
	TArray<FGridLooseItemInstance> LooseItemInstances;

	/** Monster spawn placements/generators. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements")
	TArray<FGridMonsterSpawnInstance> MonsterSpawns;

	/** Item generators, distinct from loose items. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements")
	TArray<FGridItemSpawnInstance> ItemSpawns;

	/** Data-only logic/narrative objects. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements")
	TArray<FGridLogicObjectInstance> LogicObjects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TArray<FGridObjectLink> Links;

	/** Definitions referenced by this level. Runtime quest state remains campaign-owned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Quests")
	TArray<TObjectPtr<UGridQuestDefinitionAsset>> QuestDefinitions;

	/** MON19.2.2 logical variables. Runtime values live in FGridLevelRuntimeState. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Logic|Variables")
	TArray<FGridLevelVariableDefinition> LevelVariables;

	/** MON19.3.1 source-only Lua scripts for this level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Logic|Lua")
	TArray<FGridLuaScriptSource> LuaScripts;

public:
	void EnsureCellCount();
	bool IsValidCoord(int32 X, int32 Y) const;

	/**
	 * Replaces only the instance-owned configuration of one persistent world object.
	 * WorldObjectInstances remains read-only through reflection; this is the narrow
	 * authoring mutation boundary used by editor tooling and deterministic migrations.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Placements")
	bool SetWorldObjectInstanceConfig(const FGuid& ObjectId, const FGridWorldObjectInstanceConfig& NewConfig);
	int32 GetIndex(int32 X, int32 Y) const;
	const FGridLevelCellData& GetCell(int32 X, int32 Y) const;
	FGridLevelCellData& GetCellMutable(int32 X, int32 Y);
	void ClearLevel();
	bool RemoveObjectById(const FGuid& ObjectId);
	void RemoveLinksForObject(const FGuid& ObjectId);
	void EnsureObjectIds();


	/** Sparse behavior is structural for reusable world-object placements. */
	bool UsesSparseBehaviorOverrides(const FGuid& ObjectId) const
	{
		return ObjectId.IsValid() && WorldObjectInstances.ContainsByPredicate(
			[&ObjectId](const FGridWorldObjectInstance& Instance)
			{
				return Instance.InstanceId == ObjectId;
			});
	}

	int32 GetTypedPlacementCount() const
	{
		return WorldObjectInstances.Num() + LooseItemInstances.Num() + MonsterSpawns.Num() + ItemSpawns.Num() + LogicObjects.Num();
	}

	// WORLDOBJ-MIG09-E2C-FINAL-A: native typed lookup surface. Generic runtime/editor
	// consumers use these helpers directly on the five persistent collections.
	FGridWorldObjectInstance* FindWorldObjectInstanceById(const FGuid& ObjectId);
	FGridLooseItemInstance* FindLooseItemInstanceById(const FGuid& ObjectId);
	FGridMonsterSpawnInstance* FindMonsterSpawnInstanceById(const FGuid& ObjectId);
	FGridItemSpawnInstance* FindItemSpawnInstanceById(const FGuid& ObjectId);
	FGridLogicObjectInstance* FindLogicObjectInstanceById(const FGuid& ObjectId);

	/** Placement identities in authoring order, without a compatibility projection. */
	TArray<FGuid> GetTypedPlacementIdsAtCell(int32 CellX, int32 CellY) const;

	const FGridWorldObjectInstance* FindWorldObjectInstanceById(const FGuid& ObjectId) const
	{
		return ObjectId.IsValid() ? WorldObjectInstances.FindByPredicate(
			[&ObjectId](const FGridWorldObjectInstance& Instance)
			{
				return Instance.InstanceId == ObjectId;
			}) : nullptr;
	}

	const FGridLooseItemInstance* FindLooseItemInstanceById(const FGuid& ObjectId) const
	{
		return ObjectId.IsValid() ? LooseItemInstances.FindByPredicate(
			[&ObjectId](const FGridLooseItemInstance& Instance)
			{
				return Instance.InstanceId == ObjectId;
			}) : nullptr;
	}

	const FGridMonsterSpawnInstance* FindMonsterSpawnInstanceById(const FGuid& ObjectId) const
	{
		return ObjectId.IsValid() ? MonsterSpawns.FindByPredicate(
			[&ObjectId](const FGridMonsterSpawnInstance& Spawn)
			{
				return Spawn.SpawnId == ObjectId;
			}) : nullptr;
	}

	const FGridItemSpawnInstance* FindItemSpawnInstanceById(const FGuid& ObjectId) const
	{
		return ObjectId.IsValid() ? ItemSpawns.FindByPredicate(
			[&ObjectId](const FGridItemSpawnInstance& Spawn)
			{
				return Spawn.SpawnId == ObjectId;
			}) : nullptr;
	}

	const FGridLogicObjectInstance* FindLogicObjectInstanceById(const FGuid& ObjectId) const
	{
		return ObjectId.IsValid() ? LogicObjects.FindByPredicate(
			[&ObjectId](const FGridLogicObjectInstance& Instance)
			{
				return Instance.InstanceId == ObjectId;
			}) : nullptr;
	}

	bool ContainsTypedPlacementId(const FGuid& ObjectId) const
	{
		return FindWorldObjectInstanceById(ObjectId) || FindLooseItemInstanceById(ObjectId) || FindMonsterSpawnInstanceById(ObjectId) ||
			FindItemSpawnInstanceById(ObjectId) || FindLogicObjectInstanceById(ObjectId);
	}

	EGridLevelObjectType GetTypedPlacementType(const FGuid& ObjectId) const
	{
		if (const FGridWorldObjectInstance* Instance = FindWorldObjectInstanceById(ObjectId))
		{
			return Instance->Type;
		}
		if (FindLooseItemInstanceById(ObjectId))
		{
			return EGridLevelObjectType::Item;
		}
		if (FindMonsterSpawnInstanceById(ObjectId))
		{
			return EGridLevelObjectType::MonsterSpawn;
		}
		if (FindItemSpawnInstanceById(ObjectId))
		{
			return EGridLevelObjectType::ItemSpawn;
		}
		if (const FGridLogicObjectInstance* Instance = FindLogicObjectInstanceById(ObjectId))
		{
			return Instance->Type;
		}
		return EGridLevelObjectType::None;
	}

	/**
	 * WORLDOBJ-MIG09-E2C typed spatial lookup for editor/runtime diagnostics.
	 * Returns the legacy-equivalent placement edge only as a scalar value:
	 * world objects use WallSide, loose items use SurfaceSide, and cell-centered
	 * monster/item/logic placements report None.
	 */
	bool TryGetTypedPlacementLocation(const FGuid& ObjectId, int32& OutCellX, int32& OutCellY, EGridEdge& OutEdge) const
	{
		OutCellX = INDEX_NONE;
		OutCellY = INDEX_NONE;
		OutEdge = EGridEdge::None;

		if (const FGridWorldObjectInstance* Instance = FindWorldObjectInstanceById(ObjectId))
		{
			OutCellX = Instance->CellX;
			OutCellY = Instance->CellY;
			OutEdge = Instance->WallSide;
			return true;
		}
		if (const FGridLooseItemInstance* Instance = FindLooseItemInstanceById(ObjectId))
		{
			OutCellX = Instance->CellX;
			OutCellY = Instance->CellY;
			OutEdge = Instance->SurfaceSide;
			return true;
		}
		if (const FGridMonsterSpawnInstance* Spawn = FindMonsterSpawnInstanceById(ObjectId))
		{
			OutCellX = Spawn->CellX;
			OutCellY = Spawn->CellY;
			return true;
		}
		if (const FGridItemSpawnInstance* Spawn = FindItemSpawnInstanceById(ObjectId))
		{
			OutCellX = Spawn->CellX;
			OutCellY = Spawn->CellY;
			return true;
		}
		if (const FGridLogicObjectInstance* Instance = FindLogicObjectInstanceById(ObjectId))
		{
			OutCellX = Instance->CellX;
			OutCellY = Instance->CellY;
			return true;
		}
		return false;
	}

	FName GetTypedPlacementLogicId(const FGuid& ObjectId) const
	{
		if (const FGridWorldObjectInstance* Instance = FindWorldObjectInstanceById(ObjectId))
		{
			return Instance->LogicId;
		}
		if (const FGridLooseItemInstance* Instance = FindLooseItemInstanceById(ObjectId))
		{
			return Instance->LogicId;
		}
		if (const FGridMonsterSpawnInstance* Instance = FindMonsterSpawnInstanceById(ObjectId))
		{
			return Instance->LogicId;
		}
		if (const FGridItemSpawnInstance* Instance = FindItemSpawnInstanceById(ObjectId))
		{
			return Instance->LogicId;
		}
		if (const FGridLogicObjectInstance* Instance = FindLogicObjectInstanceById(ObjectId))
		{
			return Instance->LogicId;
		}
		return NAME_None;
	}

	int32 FindTypedPlacementIdsByLogicId(FName LogicId, TArray<FGuid>& OutObjectIds) const
	{
		OutObjectIds.Reset();
		if (LogicId.IsNone())
		{
			return 0;
		}
		for (const FGridWorldObjectInstance& Instance : WorldObjectInstances)
		{
			if (Instance.LogicId == LogicId && Instance.InstanceId.IsValid())
			{
				OutObjectIds.Add(Instance.InstanceId);
			}
		}
		for (const FGridLooseItemInstance& Instance : LooseItemInstances)
		{
			if (Instance.LogicId == LogicId && Instance.InstanceId.IsValid())
			{
				OutObjectIds.Add(Instance.InstanceId);
			}
		}
		for (const FGridMonsterSpawnInstance& Spawn : MonsterSpawns)
		{
			if (Spawn.LogicId == LogicId && Spawn.SpawnId.IsValid())
			{
				OutObjectIds.Add(Spawn.SpawnId);
			}
		}
		for (const FGridItemSpawnInstance& Spawn : ItemSpawns)
		{
			if (Spawn.LogicId == LogicId && Spawn.SpawnId.IsValid())
			{
				OutObjectIds.Add(Spawn.SpawnId);
			}
		}
		for (const FGridLogicObjectInstance& Instance : LogicObjects)
		{
			if (Instance.LogicId == LogicId && Instance.InstanceId.IsValid())
			{
				OutObjectIds.Add(Instance.InstanceId);
			}
		}
		return OutObjectIds.Num();
	}


	/** WORLDOBJ-MIG09-E2C typed LogicId writer shared by editor authoring services. */
	bool SetTypedPlacementLogicId(const FGuid& ObjectId, FName NewLogicId)
	{
		if (!ObjectId.IsValid())
		{
			return false;
		}
		for (FGridWorldObjectInstance& Instance : WorldObjectInstances)
		{
			if (Instance.InstanceId == ObjectId)
			{
				Instance.LogicId = NewLogicId;
				return true;
			}
		}
		for (FGridLooseItemInstance& Instance : LooseItemInstances)
		{
			if (Instance.InstanceId == ObjectId)
			{
				Instance.LogicId = NewLogicId;
				return true;
			}
		}
		for (FGridMonsterSpawnInstance& Spawn : MonsterSpawns)
		{
			if (Spawn.SpawnId == ObjectId)
			{
				Spawn.LogicId = NewLogicId;
				return true;
			}
		}
		for (FGridItemSpawnInstance& Spawn : ItemSpawns)
		{
			if (Spawn.SpawnId == ObjectId)
			{
				Spawn.LogicId = NewLogicId;
				return true;
			}
		}
		for (FGridLogicObjectInstance& Instance : LogicObjects)
		{
			if (Instance.InstanceId == ObjectId)
			{
				Instance.LogicId = NewLogicId;
				return true;
			}
		}
		return false;
	}


	/** Validates the persistent MON13.1 MonsterSpawn contract only. */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Monsters|Validation")
	bool ValidateMonsterSpawns(UPARAM(ref) TArray<FString>& OutErrors) const;

};
