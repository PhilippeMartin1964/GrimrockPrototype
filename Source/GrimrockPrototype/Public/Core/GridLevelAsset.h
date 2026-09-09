#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridLuaScriptTypes.h"
#include "GridLevelVariableTypes.h"
#include "GridTypes.h"
#include "GridLevelPlacementTypes.h"
#include "GridLevelAsset.generated.h"

class UGridQuestDefinitionAsset;

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
