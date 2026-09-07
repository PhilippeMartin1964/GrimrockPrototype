#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridLuaScriptTypes.h"
#include "GridLevelVariableTypes.h"
#include "GridTypes.h"
#include "GridLevelPlacementTypes.h"
#include "GridLevelPlacementCompatibility.h"
#include "GridLevelAsset.generated.h"

class UGridQuestDefinitionAsset;

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridLevelAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

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

	/**
	 * WORLDOBJ-MIG09-E1 transient compatibility cache only.
	 * It is never serialized and is rebuilt from the five typed placement arrays.
	 * MIG09-E2 removes this cache together with FGridLevelObjectData.
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Gameplay|Legacy")
	TArray<FGridLevelObjectData> Objects;

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
	FGuid AddObject(const FGridLevelObjectData& NewObject);
	bool RemoveObjectById(const FGuid& ObjectId);
	void RemoveLinksForObject(const FGuid& ObjectId);
	void EnsureObjectIds();

	/**
	 * Dormant compatibility writer. No production/editor caller remains after MIG09-D2.
	 * MIG09-E2 removes it together with the transient DTO.
	 */
	bool CommitCompatibilityObjectEdit(const FGuid& ObjectId);

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

	/** Historical MIG08 conversion entry point kept only for migration characterization tests until E2. */
	void RebuildTypedPlacementProjectionFromLegacy()
	{
		WorldObjectInstances.Reset();
		LooseItemInstances.Reset();
		MonsterSpawns.Reset();
		ItemSpawns.Reset();
		LogicObjects.Reset();

		for (const FGridLevelObjectData& Object : Objects)
		{
			switch (GridLevelPlacementConversion::GetBucket(Object.Type))
			{
				case EGridLevelPlacementBucket::WorldObject:
					WorldObjectInstances.Add(GridLevelPlacementConversion::ToWorldObject(Object));
					break;
				case EGridLevelPlacementBucket::LooseItem:
					LooseItemInstances.Add(GridLevelPlacementConversion::ToLooseItem(Object));
					break;
				case EGridLevelPlacementBucket::MonsterSpawn:
					MonsterSpawns.Add(GridLevelPlacementConversion::ToMonsterSpawn(Object));
					break;
				case EGridLevelPlacementBucket::ItemSpawn:
					ItemSpawns.Add(GridLevelPlacementConversion::ToItemSpawn(Object));
					break;
				case EGridLevelPlacementBucket::LogicObject:
					LogicObjects.Add(GridLevelPlacementConversion::ToLogicObject(Object));
					break;
				case EGridLevelPlacementBucket::None:
				default:
					break;
			}
		}
	}

	/** Historical MIG08 test helper; typed storage is otherwise always authoritative in E1. */
	void EnableTypedPlacementStorageFromLegacy()
	{
		RebuildTypedPlacementProjectionFromLegacy();
		RefreshLegacyObjectMirrorFromTyped();
	}

	/**
	 * WORLDOBJ-MIG09-E2B transitional DTO projection built directly from typed authority.
	 * Runtime consumers may use this value projection while E2C removes FGridLevelObjectData itself;
	 * unlike Objects/GetObjectCompatibilityView(), it never reads or mutates the legacy cache.
	 */
	TArray<FGridLevelObjectData> BuildCompatibilityObjectProjectionFromTyped() const
	{
		TArray<FGridLevelObjectData> Projection;
		Projection.Reserve(GetTypedPlacementCount());
		for (const FGridWorldObjectInstance& Instance : WorldObjectInstances)
		{
			Projection.Add(GridLevelPlacementCompatibility::ToLegacyWorldObject(Instance));
		}
		for (const FGridLooseItemInstance& Instance : LooseItemInstances)
		{
			Projection.Add(GridLevelPlacementCompatibility::ToLegacyLooseItem(Instance));
		}
		for (const FGridMonsterSpawnInstance& Spawn : MonsterSpawns)
		{
			Projection.Add(GridLevelPlacementCompatibility::ToLegacyMonsterSpawn(Spawn));
		}
		for (const FGridItemSpawnInstance& Spawn : ItemSpawns)
		{
			Projection.Add(GridLevelPlacementCompatibility::ToLegacyItemSpawn(Spawn));
		}
		for (const FGridLogicObjectInstance& Instance : LogicObjects)
		{
			Projection.Add(GridLevelPlacementCompatibility::ToLegacyLogicObject(Instance));
		}
		return Projection;
	}

	/** Rebuilds the non-persistent E1 compatibility cache from typed source of truth. */
	void RefreshLegacyObjectMirrorFromTyped()
	{
		Objects = BuildCompatibilityObjectProjectionFromTyped();
	}

	/** Transitional E1 read view. E2 removes it with FGridLevelObjectData. */
	const TArray<FGridLevelObjectData>& GetObjectCompatibilityView() const
	{
		const_cast<UGridLevelAsset*>(this)->RefreshLegacyObjectMirrorFromTyped();
		return Objects;
	}

	/** Validates the persistent MON13.1 MonsterSpawn contract only. */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Monsters|Validation")
	bool ValidateMonsterSpawns(UPARAM(ref) TArray<FString>& OutErrors) const;

	const FGridLevelObjectData* FindMonsterSpawnById(const FGuid& SpawnId) const;
};
