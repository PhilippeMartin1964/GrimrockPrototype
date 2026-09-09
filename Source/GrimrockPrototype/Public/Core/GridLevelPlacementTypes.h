#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "GridLevelPlacementTypes.generated.h"

class URPGStoryCompanionAsset;

/** WORLDOBJ-MIG07 target placement buckets. */
UENUM(BlueprintType)
enum class EGridLevelPlacementBucket : uint8
{
	None,
	WorldObject,
	LooseItem,
	MonsterSpawn,
	ItemSpawn,
	LogicObject
};

/** Minimal per-instance state/configuration for one placed world object. */
USTRUCT(BlueprintType)
struct FGridWorldObjectInstanceConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Teleporter")
	FGridTeleporterBehaviorParams Teleporter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Transition")
	FGridObjectTransitionParams Transition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Pit")
	FGridPitBehaviorParams Pit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Receptacle")
	TArray<FGridReceptacleInitialItemConfig> ReceptacleInitialContent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Lock")
	bool bStartsUnlocked = false;
};

/** Persistent placement of a reusable world-object definition. */
USTRUCT(BlueprintType)
struct FGridWorldObjectInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid InstanceId;

	/** Stable definition reference used until MIG10 final naming/AssetManager cleanup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName WorldObjectDefinitionId = NAME_None;

	/** Temporary type discriminator while the definition registry is still id-based. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	EGridLevelObjectType Type = EGridLevelObjectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	EGridEdge WallSide = EGridEdge::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	bool bHasLocalTransformOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (EditCondition = "bHasLocalTransformOverride", EditConditionHides))
	FTransform LocalTransformOverride = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial State")
	bool bInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial State")
	bool bInitiallyActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName LogicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FString Notes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName PaletteEntryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable", meta = (MultiLine = "true"))
	FText ReadableTextOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	FGridWorldObjectInstanceConfig InstanceConfig;
};

/** A collectible physically present in the level. It is not a spawn generator. */
USTRUCT(BlueprintType)
struct FGridLooseItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UGridItemDefinitionAsset> ItemDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	EGridEdge SurfaceSide = EGridEdge::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	float LocalYaw = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial State")
	bool bInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName LogicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	TObjectPtr<UGridReadableContentAsset> ReadableContentAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	FName ReadableContentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	FText ReadTitleOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable", meta = (MultiLine = "true"))
	FText ReadTextOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FString Notes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName PaletteEntryId = NAME_None;
};

/** Persistent monster generator/placement. SpawnId remains the stable logical id. */
USTRUCT(BlueprintType)
struct FGridMonsterSpawnInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid SpawnId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	TObjectPtr<UGridMonsterDefinitionAsset> MonsterDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	EGridEdge Facing = EGridEdge::North;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial State")
	EGridMonsterState InitialMonsterState = EGridMonsterState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	EGridMonsterPatrolMode PatrolMode = EGridMonsterPatrolMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TArray<FGridMonsterPatrolWaypoint> PatrolWaypoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	FName EncounterGroupId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (ClampMin = "0"))
	int32 EncounterWaveIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial State")
	bool bInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName LogicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FString Notes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName PaletteEntryId = NAME_None;
};

/** Item generator. Distinct from a collectible already present in the level. */
USTRUCT(BlueprintType)
struct FGridItemSpawnInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid SpawnId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UGridItemDefinitionAsset> ItemDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial State")
	bool bInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName LogicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FString Notes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName PaletteEntryId = NAME_None;
};

/** Data-only logical/narrative target. No runtime Actor is required by this structure. */
USTRUCT(BlueprintType)
struct FGridLogicObjectInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName LogicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	EGridLevelObjectType Type = EGridLevelObjectType::Logic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 CellY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial State")
	bool bInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Initial State")
	bool bInitiallyActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic")
	FGridLogicNodeParams Logic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Story Companion")
	TObjectPtr<URPGStoryCompanionAsset> StoryCompanionDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FString Notes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName PaletteEntryId = NAME_None;
};

/** Scalar classification of the native placement collections; no data conversion. */
namespace GridLevelPlacement
{
	inline EGridLevelPlacementBucket GetBucket(EGridLevelObjectType Type)
	{
		switch (Type)
		{
			case EGridLevelObjectType::Item:
				return EGridLevelPlacementBucket::LooseItem;
			case EGridLevelObjectType::MonsterSpawn:
				return EGridLevelPlacementBucket::MonsterSpawn;
			case EGridLevelObjectType::ItemSpawn:
				return EGridLevelPlacementBucket::ItemSpawn;
			case EGridLevelObjectType::Logic:
			case EGridLevelObjectType::StoryCompanion:
			case EGridLevelObjectType::CustomRecruiter:
				return EGridLevelPlacementBucket::LogicObject;
			case EGridLevelObjectType::None:
				return EGridLevelPlacementBucket::None;
			default:
				return EGridLevelPlacementBucket::WorldObject;
		}
	}

}
