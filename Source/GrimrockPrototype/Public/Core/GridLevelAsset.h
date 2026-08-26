#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridLuaScriptTypes.h"
#include "GridLevelVariableTypes.h"
#include "GridTypes.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TArray<FGridLevelObjectData> Objects;

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
     * Validates the persistent MON13.1 MonsterSpawn contract only.
     * Runtime actor creation and occupancy registration belong to later MON13 milestones.
     */
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Monsters|Validation")
	bool ValidateMonsterSpawns(UPARAM(ref) TArray<FString>& OutErrors) const;

	const FGridLevelObjectData* FindMonsterSpawnById(const FGuid& SpawnId) const;
};
