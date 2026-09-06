#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "GridDungeonRuntimeState.generated.h"

class UGridReadableContentAsset;
class UGridItemDefinitionAsset;
USTRUCT(BlueprintType)
struct FGridRuntimeDoorState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid ObjectId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsOpen = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bBlocksMovement = true;
};

USTRUCT(BlueprintType)
struct FGridRuntimeInteractiveState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid ObjectId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsActivated = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsPressed = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsOn = false;
};

USTRUCT(BlueprintType)
struct FGridRuntimeObjectPresenceState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid ObjectId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bRemovedFromInitialPlacement = false;
};

USTRUCT(BlueprintType)
struct FGridRuntimeItemState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid ObjectId;

	/** Canonical runtime/save identity for an item. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FName ItemDefinitionId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 Quantity = 1;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 CellX = INDEX_NONE;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 CellY = INDEX_NONE;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	EGridEdge Edge = EGridEdge::None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FTransform Transform = FTransform::Identity;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsSimulatingPhysics = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsContainedInReceptacle = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid ReceptacleObjectId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bLightsEnabled = true;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TObjectPtr<UGridReadableContentAsset> ReadableContentAsset = nullptr;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FName ReadableContentId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FText ReadTitleOverride;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FText ReadTextOverride;
};

USTRUCT(BlueprintType)
struct FGridRuntimePitState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid ObjectId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsOpen = true;
};

USTRUCT(BlueprintType)
struct FGridPendingWorldItemState
{
	GENERATED_BODY()

	/** Normal world-item state that will be materialized when the destination level becomes active. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGridRuntimeItemState ItemState;

	/** Keeps the definition resolvable even when the destination level does not otherwise reference this item type. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TObjectPtr<UGridItemDefinitionAsset> ItemDefinitionAsset = nullptr;
};

USTRUCT(BlueprintType)
struct FGridRuntimeReceptacleState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid ObjectId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bCanRemoveItem = true;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FGridRuntimeItemState> ContainedItems;
};

USTRUCT(BlueprintType)
struct FGridRuntimeMonsterState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid PersistenceId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid SpawnObjectId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FName MonsterDefinitionId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FName DungeonLevelId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 CellX = INDEX_NONE;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 CellY = INDEX_NONE;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	EGridEdge Facing = EGridEdge::North;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	EGridMonsterState MonsterState = EGridMonsterState::Idle;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 CurrentHealth = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 CurrentPhysicalArmor = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 CurrentMagicalArmor = 0;

	/** Stable MON16.7 status snapshots; DefinitionAsset remains transient. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FGridStatusEffectSaveState> StatusEffects;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bMonsterEnabled = true;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FName EncounterGroupId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bHasLastKnownPartyCell = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FIntPoint LastKnownPartyCell = FIntPoint::ZeroValue;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsDead = false;
};

/** Persistent MON13.3 presence and last known state for one MonsterSpawn. */
USTRUCT(BlueprintType)
struct FGridRuntimeMonsterPlacementState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid SpawnId;

	/** False keeps the placement absent until a Spawn command succeeds. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsSpawned = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bHasMonsterState = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGridRuntimeMonsterState MonsterState;
};

/** Persistent MON13.4 progress for one encounter group. */
USTRUCT(BlueprintType)
struct FGridRuntimeMonsterEncounterState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FName EncounterGroupId = NAME_None;

	/** MonsterSpawn used as the source of encounter lifecycle links. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FGuid AnchorSpawnId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 ActiveWaveIndex = INDEX_NONE;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bStarted = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bCompleted = false;

	/** Only committed deaths advance waves; Despawn never adds an id here. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TSet<FGuid> DefeatedSpawnIds;
};

USTRUCT(BlueprintType)
struct FGridLevelRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FName LevelId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridRuntimeDoorState> Doors;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridRuntimeInteractiveState> InteractiveObjects;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridRuntimeObjectPresenceState> ObjectPresence;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridRuntimeItemState> Items;

	/** PIT03 authoritative open/closed state for controlled pits that diverged from their authored initial state. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridRuntimePitState> Pits;

	/** PIT02 items that already left an upper level but whose destination level is not currently active. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridPendingWorldItemState> PendingInboundItems;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridRuntimeReceptacleState> Receptacles;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bHasBeenVisited = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridRuntimeMonsterState> Monsters;

	/** MON13.3 lifecycle state keyed by persistent MonsterSpawn ObjectId. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FGuid, FGridRuntimeMonsterPlacementState> MonsterPlacements;

	/** MON13.4 encounter progress keyed by EncounterGroupId. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FName, FGridRuntimeMonsterEncounterState> MonsterEncounters;

	/**
     * MON19.2.2: distinguishes a canonical runtime variable snapshot from a
     * legacy save whose variables still need level-default initialization.
     */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bLevelVariablesInitialized = false;

	/** MON19.2.2 Bool values keyed by stable VariableId. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FName, bool> BoolVariables;

	/** MON19.2.2 Int32 values keyed by stable VariableId. */
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FName, int32> IntVariables;
};

USTRUCT(BlueprintType)
struct FGridDungeonRuntimeState
{
	GENERATED_BODY()

	// Runtime state can be embedded in a versioned SaveGame while the actor keeps its owning property transient.
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TMap<FName, FGridLevelRuntimeState> LevelStates;
};