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

/** Sparse per-instance decision for the optional door-chain mechanism. */
UENUM(BlueprintType)
enum class EGridDoorChainMode : uint8
{
	Inherit UMETA(DisplayName = "Inherit Definition"),
	Enabled UMETA(DisplayName = "Enabled"),
	Disabled UMETA(DisplayName = "Disabled")
};

/** Sparse generic override for one authored MovingPart slot of a placed world object. */
USTRUCT(BlueprintType)
struct FGridWorldObjectMovingPartInstanceOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Moving Part", meta = (ClampMin = "0", ClampMax = "1"))
	int32 PartIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Moving Part")
	bool bOverrideLocalTransform = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Moving Part",
		meta = (EditCondition = "bOverrideLocalTransform", EditConditionHides))
	FTransform LocalTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Moving Part")
	bool bOverrideMotionAmount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Moving Part",
		meta = (EditCondition = "bOverrideMotionAmount", EditConditionHides))
	float MotionAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Moving Part")
	bool bOverrideMotionDuration = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Moving Part",
		meta = (EditCondition = "bOverrideMotionDuration", EditConditionHides, ClampMin = "0.0"))
	float MotionDuration = 0.0f;
};

/** Instance-owned acceptance/capacity rules for a receptacle puzzle. Presentation remains Definition-owned. */
USTRUCT(BlueprintType)
struct FGridReceptacleInstanceRuleValues
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Receptacle")
	bool bAcceptAnyItem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Receptacle")
	TArray<FGridReceptacleAcceptedItemConfig> AcceptedItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Receptacle", meta = (ClampMin = "1"))
	int32 MaxContainedItems = 1;
};

/** Instance-owned accepted-key set for a wall-lock puzzle. Lock presentation/messages remain Definition-owned. */
USTRUCT(BlueprintType)
struct FGridLockAcceptedKeyInstanceRuleValues
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Lock|Accepted Keys")
	TArray<TObjectPtr<UGridItemDefinitionAsset>> AcceptedKeyItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Lock|Accepted Keys")
	TArray<FName> AcceptedKeyIds;
};

/**
 * GEUI09 sparse puzzle-rule overrides.
 *
 * The Definition remains authoritative unless the corresponding bOverride flag is true.
 * This is intentionally not a full FGridObjectBehaviorParams copy.
 */
USTRUCT(BlueprintType)
struct FGridWorldObjectInteractionOverrides
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Button")
	bool bOverrideButtonHoldTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Button",
		meta = (EditCondition = "bOverrideButtonHoldTime", EditConditionHides, ClampMin = "0.0"))
	float ButtonHoldTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Pressure Plate")
	bool bOverridePressurePlateWeight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Pressure Plate",
		meta = (EditCondition = "bOverridePressurePlateWeight", EditConditionHides))
	FGridPressurePlateWeightParams PressurePlateWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Receptacle")
	bool bOverrideReceptacleRules = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Receptacle",
		meta = (EditCondition = "bOverrideReceptacleRules", EditConditionHides))
	FGridReceptacleInstanceRuleValues ReceptacleRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Lock")
	bool bOverrideAcceptedKeys = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Lock",
		meta = (EditCondition = "bOverrideAcceptedKeys", EditConditionHides))
	FGridLockAcceptedKeyInstanceRuleValues AcceptedKeys;
};

/** Minimal per-instance state/configuration for one placed world object. */
USTRUCT(BlueprintType)
struct FGridWorldObjectInstanceConfig
{
	GENERATED_BODY()

	/** Puzzle-local initial state. Closed is the normal authored default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Door|Initial State")
	bool bDoorInitiallyOpen = false;

	/** Puzzle-local initial state. A teleporter may require a mechanism to enable it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Teleporter|Initial State")
	bool bTeleporterInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Teleporter")
	FGridTeleporterBehaviorParams Teleporter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Transition")
	FGridObjectTransitionParams Transition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Pit")
	FGridPitBehaviorParams Pit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Receptacle")
	TArray<FGridReceptacleInitialItemConfig> ReceptacleInitialContent;

	/**
	 * Sparse gameplay/puzzle overrides exposed by the Grid Editor Selected Object inspector.
	 * Permanent presentation and shared defaults remain on the Definition.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Interaction")
	FGridWorldObjectInteractionOverrides InteractionOverrides;

	/** Sparse visual exceptions. Shared geometry/motion remains authored on the Definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Moving Parts")
	TArray<FGridWorldObjectMovingPartInstanceOverride> MovingPartOverrides;

	/**
	 * Sparse door-chain enablement override.
	 * Inherit preserves the Definition value; Enabled/Disabled force only the chain presence.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Door|Chain")
	EGridDoorChainMode DoorChainMode = EGridDoorChainMode::Inherit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Door|Chain")
	bool bOverrideChainPullDuration = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Door|Chain",
		meta = (EditCondition = "bOverrideChainPullDuration", EditConditionHides, ClampMin = "0.01"))
	float ChainPullDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance|Lock")
	bool bStartsUnlocked = false;
};

/** Persistent placement of a reusable world-object definition. Presence is implied by placement. */
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName LogicId = NAME_None;

	// LUA-UX03 source-compatibility tombstone; no longer reflected/serialized or authored.
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

/** A collectible physically present in the level. Presence is implied by placement. */
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

	/** Spawn-only state: whether this generator creates its monster when the level starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (DisplayName = "Spawn at Start"))
	bool bInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName LogicId = NAME_None;

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

	/** Spawn-only state: whether this generator creates its item when the level starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (DisplayName = "Spawn at Start"))
	bool bInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName LogicId = NAME_None;

	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FString Notes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authoring")
	FName PaletteEntryId = NAME_None;
};

/** Data-only logical/narrative target. Presence is implied by placement. */
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic")
	FGridLogicNodeParams Logic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Story Companion")
	TObjectPtr<URPGStoryCompanionAsset> StoryCompanionDefinition = nullptr;

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
