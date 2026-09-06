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
	 * WORLDOBJ-MIG07 legacy bridge. Runtime/editor consumers still read this array
	 * during MIG07-A/B. MIG08 migrates real assets to the typed collections below;
	 * MIG09 removes this monolithic storage once every consumer has moved.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Legacy")
	TArray<FGridLevelObjectData> Objects;

	/** WORLDOBJ-MIG07 target: reusable world-object placements only. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements|MIG07")
	TArray<FGridWorldObjectInstance> WorldObjectInstances;

	/** WORLDOBJ-MIG07 target: collectibles physically present in the level. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements|MIG07")
	TArray<FGridLooseItemInstance> LooseItemInstances;

	/** WORLDOBJ-MIG07 target: monster spawn placements/generators. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements|MIG07")
	TArray<FGridMonsterSpawnInstance> MonsterSpawns;

	/** WORLDOBJ-MIG07 target: item generators, distinct from loose items. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements|MIG07")
	TArray<FGridItemSpawnInstance> ItemSpawns;

	/** WORLDOBJ-MIG07 target: data-only logic/narrative objects. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements|MIG07")
	TArray<FGridLogicObjectInstance> LogicObjects;

	/**
	 * WORLDOBJ-MIG07-B authority marker. False means the historical Objects array
	 * is still the persistent source. True means the five typed collections above
	 * are authoritative and Objects is only a compatibility mirror for consumers
	 * that have not yet been physically purged.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay|Placements|MIG07")
	bool bTypedPlacementStorageAuthoritative = false;

	/**
	 * WORLDOBJ-MIG06 migration marker.
	 *
	 * Object ids in this set store only sparse instance-owned values in
	 * FGridLevelObjectData::Behavior. An id absent from the set uses the historical
	 * pre-MIG06 full Behavior snapshot until the real assets are migrated in MIG08.
	 */
	UPROPERTY()
	TSet<FGuid> SparseBehaviorOverrideObjectIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TArray<FGridObjectLink> Links;

	/** Definitions referenced by this level. Runtime quest state remains campaign-owned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Quests")
	TArray<TObjectPtr<UGridQuestDefinitionAsset>> QuestDefinitions;

	/**
     * MON19.2.2 logical variables. VariableId is unique across Bool and Int32
     * definitions; runtime values live in FGridLevelRuntimeState, never here.
     */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Logic|Variables")
	TArray<FGridLevelVariableDefinition> LevelVariables;

	/**
     * MON19.3.1 source-only Lua scripts for this level. One future active-level
     * VM loads all enabled ScriptIds into isolated environments.
     */
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
	 * WORLDOBJ-MIG07-C write-through bridge used by the existing Grid Editor.
	 * The editor may still stage one object through FGridLevelObjectData, but once
	 * typed storage is authoritative this method merges that edit back into the
	 * correct typed collection without destroying typed-only fields.
	 */
	bool CommitCompatibilityObjectEdit(const FGuid& ObjectId);

	bool UsesSparseBehaviorOverrides(const FGuid& ObjectId) const
	{
		if (!ObjectId.IsValid())
		{
			return false;
		}
		if (bTypedPlacementStorageAuthoritative)
		{
			return WorldObjectInstances.ContainsByPredicate(
				[&ObjectId](const FGridWorldObjectInstance& Instance)
				{
					return Instance.InstanceId == ObjectId;
				});
		}
		return SparseBehaviorOverrideObjectIds.Contains(ObjectId);
	}

	void SetSparseBehaviorOverrides(const FGuid& ObjectId, bool bUsesSparseOverrides)
	{
		if (!ObjectId.IsValid())
		{
			return;
		}

		if (bTypedPlacementStorageAuthoritative)
		{
			const bool bIsTypedWorldObject = WorldObjectInstances.ContainsByPredicate(
				[&ObjectId](const FGridWorldObjectInstance& Instance)
				{
					return Instance.InstanceId == ObjectId;
				});
			if (bUsesSparseOverrides && bIsTypedWorldObject)
			{
				SparseBehaviorOverrideObjectIds.Add(ObjectId);
			}
			else
			{
				SparseBehaviorOverrideObjectIds.Remove(ObjectId);
			}
			return;
		}

		if (bUsesSparseOverrides)
		{
			SparseBehaviorOverrideObjectIds.Add(ObjectId);
		}
		else
		{
			SparseBehaviorOverrideObjectIds.Remove(ObjectId);
		}
	}

	/** Number of placements already represented by the MIG07 typed schema. */
	int32 GetTypedPlacementCount() const
	{
		return WorldObjectInstances.Num() + LooseItemInstances.Num() + MonsterSpawns.Num() + ItemSpawns.Num() + LogicObjects.Num();
	}

	/**
	 * MIG07 migration helper used by tests and, later, MIG08 asset conversion.
	 * It projects the current legacy monolith into the target typed collections.
	 * Runtime authority does not switch until EnableTypedPlacementStorageFromLegacy().
	 */
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

	/**
	 * Explicit MIG07-B cut-over helper. MIG08 will invoke the equivalent operation
	 * while converting real assets; it is deliberately never implicit for legacy assets.
	 */
	void EnableTypedPlacementStorageFromLegacy()
	{
		RebuildTypedPlacementProjectionFromLegacy();
		bTypedPlacementStorageAuthoritative = true;
		RefreshLegacyObjectMirrorFromTyped();
	}

	/**
	 * Rebuilds the historical monolithic array from the typed source of truth.
	 * This is a compatibility view, not a second authoring authority.
	 */
	void RefreshLegacyObjectMirrorFromTyped()
	{
		if (!bTypedPlacementStorageAuthoritative)
		{
			return;
		}

		Objects.Reset(GetTypedPlacementCount());
		SparseBehaviorOverrideObjectIds.Reset();

		for (const FGridWorldObjectInstance& Instance : WorldObjectInstances)
		{
			Objects.Add(GridLevelPlacementCompatibility::ToLegacyWorldObject(Instance));
			if (Instance.InstanceId.IsValid())
			{
				SparseBehaviorOverrideObjectIds.Add(Instance.InstanceId);
			}
		}
		for (const FGridLooseItemInstance& Instance : LooseItemInstances)
		{
			Objects.Add(GridLevelPlacementCompatibility::ToLegacyLooseItem(Instance));
		}
		for (const FGridMonsterSpawnInstance& Spawn : MonsterSpawns)
		{
			Objects.Add(GridLevelPlacementCompatibility::ToLegacyMonsterSpawn(Spawn));
		}
		for (const FGridItemSpawnInstance& Spawn : ItemSpawns)
		{
			Objects.Add(GridLevelPlacementCompatibility::ToLegacyItemSpawn(Spawn));
		}
		for (const FGridLogicObjectInstance& Instance : LogicObjects)
		{
			Objects.Add(GridLevelPlacementCompatibility::ToLegacyLogicObject(Instance));
		}
	}

	/**
	 * Transitional view for runtime/editor code still typed against FGridLevelObjectData.
	 * In typed mode, the view is rebuilt from the typed collections before exposure.
	 */
	const TArray<FGridLevelObjectData>& GetObjectCompatibilityView() const
	{
		if (bTypedPlacementStorageAuthoritative)
		{
			const_cast<UGridLevelAsset*>(this)->RefreshLegacyObjectMirrorFromTyped();
		}
		return Objects;
	}

	/**
     * Validates the persistent MON13.1 MonsterSpawn contract only.
     * Runtime actor creation and occupancy registration belong to later MON13 milestones.
     */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Monsters|Validation")
	bool ValidateMonsterSpawns(UPARAM(ref) TArray<FString>& OutErrors) const;

	const FGridLevelObjectData* FindMonsterSpawnById(const FGuid& SpawnId) const;
};
