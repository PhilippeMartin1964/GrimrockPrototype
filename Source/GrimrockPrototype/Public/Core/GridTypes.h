#pragma once

#include "CoreMinimal.h"
#include "GridObjectBehavior.h"
#include "GridLogicTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "GridTypes.generated.h"

class UGridReadableContentAsset;
class URPGStoryCompanionAsset;

UENUM(BlueprintType)
enum class EGridCellType : uint8
{
	Empty UMETA(DisplayName = "Empty"),
	Floor UMETA(DisplayName = "Floor"),
	Pit UMETA(DisplayName = "Pit"),
	StairsUp UMETA(DisplayName = "Stairs Up"),
	StairsDown UMETA(DisplayName = "Stairs Down"),
	Teleporter UMETA(DisplayName = "Teleporter")
};

UENUM(BlueprintType)
enum class EGridWallType : uint8
{
	None UMETA(DisplayName = "None"),
	Solid UMETA(DisplayName = "Solid")
};

UENUM(BlueprintType)
enum class EGridLevelObjectType : uint8
{
	None UMETA(DisplayName = "None"),
	Door UMETA(DisplayName = "Door"),
	Button UMETA(DisplayName = "Button"),
	PressurePlate UMETA(DisplayName = "Pressure Plate"),
	Lever UMETA(DisplayName = "Lever"),
	Decoration UMETA(DisplayName = "Decoration"),
	MonsterSpawn UMETA(DisplayName = "Monster Spawn"),
	ItemSpawn UMETA(DisplayName = "Item Spawn"),
	Light UMETA(DisplayName = "Light"),
	Teleporter UMETA(DisplayName = "Teleporter"),
	Trigger UMETA(DisplayName = "Trigger"),
	Receptacle UMETA(DisplayName = "Receptacle"),
	Item UMETA(DisplayName = "Item"),
	Logic UMETA(DisplayName = "Logic"),
	StoryCompanion UMETA(DisplayName = "Story Companion"),
	CustomRecruiter UMETA(DisplayName = "Custom Recruiter")
};

UENUM(BlueprintType)
enum class EGridObjectPlacementKind : uint8
{
	Center UMETA(DisplayName = "Center"),
	Edge UMETA(DisplayName = "Edge"),
	Floor UMETA(DisplayName = "Floor"),
	Wall UMETA(DisplayName = "Wall"),
	Ceiling UMETA(DisplayName = "Ceiling")
};

UENUM(BlueprintType)
enum class EGridObjectCategory : uint8
{
	Mechanism UMETA(DisplayName = "Mechanism"),
	Decoration UMETA(DisplayName = "Decoration"),
	Prop UMETA(DisplayName = "Prop"),
	Receptacle UMETA(DisplayName = "Receptacle"),
	Light UMETA(DisplayName = "Light"),
	Readable UMETA(DisplayName = "Readable"),
	Spawn UMETA(DisplayName = "Spawn"),
	Teleporter UMETA(DisplayName = "Teleporter"),
	Item UMETA(DisplayName = "Item")
};

UENUM(BlueprintType)
enum class EGridObjectCommand : uint8
{
	Toggle UMETA(DisplayName = "Toggle"),
	Open UMETA(DisplayName = "Open"),
	Close UMETA(DisplayName = "Close"),
	Activate UMETA(DisplayName = "Activate"),
	Deactivate UMETA(DisplayName = "Deactivate"),
	Enable UMETA(DisplayName = "Enable"),
	Disable UMETA(DisplayName = "Disable"),
	Lock UMETA(DisplayName = "Lock"),
	Unlock UMETA(DisplayName = "Unlock"),
	Spawn UMETA(DisplayName = "Spawn"),
	Despawn UMETA(DisplayName = "Despawn"),
	Teleport UMETA(DisplayName = "Teleport"),
	ShowMessage UMETA(DisplayName = "Show Message"),
	ReceptacleConsumeItem UMETA(DisplayName = "Receptacle Consume Item"),
	ReceptacleConsumeAllItems UMETA(DisplayName = "Receptacle Consume All Items"),
	ReceptacleEnableRemoval = 17 UMETA(DisplayName = "Receptacle Enable Removal"),
	ReceptacleDisableRemoval = 18 UMETA(DisplayName = "Receptacle Disable Removal"),
	StartEncounter = 19 UMETA(DisplayName = "Start Encounter"),
	LogicExecute = 20 UMETA(DisplayName = "Logic Execute"),
	LogicReset = 21 UMETA(DisplayName = "Logic Reset"),
	LuaCallback = 22 UMETA(DisplayName = "Lua Callback"),
	OfferRecruitment = 23 UMETA(DisplayName = "Offer Recruitment"),
	OpenCustomRecruit = 24 UMETA(DisplayName = "Open Custom Recruit")
};

UENUM(BlueprintType)
enum class EGridObjectCondition : uint8
{
	None UMETA(DisplayName = "None"),
	ReceptacleIsEmpty UMETA(DisplayName = "Receptacle Is Empty"),
	ReceptacleHasAnyItem UMETA(DisplayName = "Receptacle Has Any Item"),
	ReceptacleContainsItemDefinition UMETA(DisplayName = "Receptacle Contains Item Definition"),
	ReceptacleContainsItemTag UMETA(DisplayName = "Receptacle Contains Item Tag"),
	ReceptacleContainsItemType UMETA(DisplayName = "Receptacle Contains Item Type"),
	ReceptacleItemCountAtLeast UMETA(DisplayName = "Receptacle Item Count At Least"),
	ReceptacleWeightAtLeast UMETA(DisplayName = "Receptacle Weight At Least"),
	LevelVariableBoolEquals UMETA(DisplayName = "Level Variable Bool Equals"),
	LevelVariableIntCompare UMETA(DisplayName = "Level Variable Int Compare")
};

UENUM(BlueprintType)
enum class EGridObjectEvent : uint8
{
	Activated UMETA(DisplayName = "Activated"),
	Deactivated UMETA(DisplayName = "Deactivated"),
	ItemInserted UMETA(DisplayName = "Item Inserted"),
	ItemRemoved UMETA(DisplayName = "Item Removed"),
	ItemChanged UMETA(DisplayName = "Item Changed"),
	Used UMETA(DisplayName = "Used"),
	Entered UMETA(DisplayName = "Entered"),
	Exited UMETA(DisplayName = "Exited"),
	Opened UMETA(DisplayName = "Opened"),
	Closed UMETA(DisplayName = "Closed"),
	Enabled UMETA(DisplayName = "Enabled"),
	Disabled UMETA(DisplayName = "Disabled"),
	MonsterDied UMETA(DisplayName = "Monster Died"),
	MonsterSpawned UMETA(DisplayName = "Monster Spawned"),
	MonsterDespawned UMETA(DisplayName = "Monster Despawned"),
	MonsterTeleported UMETA(DisplayName = "Monster Teleported"),
	EncounterWaveStarted UMETA(DisplayName = "Encounter Wave Started"),
	EncounterCompleted UMETA(DisplayName = "Encounter Completed")
};

USTRUCT(BlueprintType)
struct FGridLevelCellData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridCellType CellType = EGridCellType::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridWallType NorthWall = EGridWallType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridWallType EastWall = EGridWallType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridWallType SouthWall = EGridWallType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridWallType WestWall = EGridWallType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasCeiling = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBlocksOccupancy = false;
};

USTRUCT(BlueprintType)
struct FGridLevelObjectData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid ObjectId;

	/** Optional human-authored stable alias used by Lua and editor diagnostics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic")
	FName LogicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridLevelObjectType Type = EGridLevelObjectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CellX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CellY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridEdge Edge = EGridEdge::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LocalYaw = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UGridItemDefinitionAsset> ItemDefinitionAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Reading")
	TObjectPtr<UGridReadableContentAsset> ReadableContentAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Reading")
	FName ReadableContentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Reading")
	FText ReadTitleOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Reading", meta = (MultiLine = "true"))
	FText ReadTextOverride;

	/** MON13 persistent monster definition. ObjectId remains the stable SpawnId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster", meta = (EditCondition = "Type == EGridLevelObjectType::MonsterSpawn", EditConditionHides))
	TObjectPtr<UGridMonsterDefinitionAsset> MonsterDefinitionAsset = nullptr;

	/** Stable lookup id used by persistence and future Asset Manager resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster", meta = (EditCondition = "Type == EGridLevelObjectType::MonsterSpawn", EditConditionHides))
	FName MonsterDefinitionId = NAME_None;

	/** Cardinal gameplay orientation. LocalYaw remains a preview compatibility mirror. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster", meta = (EditCondition = "Type == EGridLevelObjectType::MonsterSpawn", EditConditionHides))
	EGridEdge InitialFacing = EGridEdge::None;

	/** Fresh-spawn exploration state. Presence remains controlled by bInitiallyEnabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster", meta = (EditCondition = "Type == EGridLevelObjectType::MonsterSpawn", EditConditionHides))
	EGridMonsterState InitialMonsterState = EGridMonsterState::Idle;

	/** MON14.2 route data only. MON14.3 will execute these waypoints. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Patrol",
		meta = (EditCondition = "Type == EGridLevelObjectType::MonsterSpawn", EditConditionHides))
	EGridMonsterPatrolMode PatrolMode = EGridMonsterPatrolMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Patrol",
		meta = (EditCondition = "Type == EGridLevelObjectType::MonsterSpawn", EditConditionHides))
	TArray<FGridMonsterPatrolWaypoint> PatrolWaypoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bInitiallyActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Notes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable", meta = (MultiLine = "true"))
	FText OverrideReadableText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PaletteEntryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGridObjectBehaviorParams Behavior;

	/** MON19.2.3 data-only logic primitive. No runtime Actor is required. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logic", meta = (EditCondition = "Type == EGridLevelObjectType::Logic", EditConditionHides))
	FGridLogicNodeParams Logic;

	/** MON20.4 narrative recruitment target. No runtime Actor is required. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Story Companion",
		meta = (EditCondition = "Type == EGridLevelObjectType::StoryCompanion", EditConditionHides))
	TObjectPtr<URPGStoryCompanionAsset> StoryCompanionDefinition = nullptr;

	/** Optional MON7/MON13 encounter group. None preserves independent behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	FName EncounterGroupId = NAME_None;

	/** MON13.4 zero-based wave inside EncounterGroupId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster",
		meta = (EditCondition = "Type == EGridLevelObjectType::MonsterSpawn", EditConditionHides, ClampMin = "0"))
	int32 EncounterWaveIndex = 0;
};

USTRUCT(BlueprintType)
struct FGridObjectLink
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid SourceObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid TargetObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridObjectEvent SourceEvent = EGridObjectEvent::Activated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridObjectCommand Command = EGridObjectCommand::Toggle;

	/** MON19.4 ScriptId used only when Command == LuaCallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	FName LuaScriptId = NAME_None;

	/** MON19.4 callback function used only when Command == LuaCallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
	FName LuaCallbackName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	EGridObjectCondition Condition = EGridObjectCondition::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Variable")
	FName ConditionVariableId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Variable")
	bool ConditionBoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Variable")
	EGridLogicIntComparison ConditionIntComparison = EGridLogicIntComparison::Equal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Variable")
	int32 ConditionIntValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	FName ConditionItemDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	FName ConditionItemTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	EGridItemType ConditionItemType = EGridItemType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = (ClampMin = "0"))
	int32 ConditionCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = (ClampMin = "0.0"))
	float ConditionWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	bool bInvertCondition = false;
};

USTRUCT(BlueprintType)
struct FGridEdgeKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 X = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Y = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	EGridEdge Edge = EGridEdge::None;

	FGridEdgeKey() = default;

	FGridEdgeKey(int32 InX, int32 InY, EGridEdge InEdge)
		: X(InX)
		, Y(InY)
		, Edge(InEdge)
	{
	}

	friend bool operator==(const FGridEdgeKey& A, const FGridEdgeKey& B)
	{
		return A.X == B.X && A.Y == B.Y && A.Edge == B.Edge;
	}
};

FORCEINLINE uint32 GetTypeHash(const FGridEdgeKey& Key)
{
	uint32 Hash = GetTypeHash(Key.X);
	Hash = HashCombine(Hash, GetTypeHash(Key.Y));
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Key.Edge)));
	return Hash;
}
