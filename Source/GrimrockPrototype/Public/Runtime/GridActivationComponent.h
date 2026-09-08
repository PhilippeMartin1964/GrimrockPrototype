#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GridTypes.h"
#include "Core/GridLevelPlacementTypes.h"
#include "GridLuaVm.h"
#include "GridActivationComponent.generated.h"

class AGridLevelRuntimeActor;
class AGrimrockPartyPawn;
class AGridReceptacleActor;
class URPGStoryCompanionAsset;
struct FGridPartyInventoryState;

UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridActivationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridActivationComponent();

	void Initialize(AGridLevelRuntimeActor* InRuntime);
	void ResetRuntimeState();

	bool TryInteractAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn);
	AGridReceptacleActor* FindReceptacleAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge) const;
	void HandlePartyCellChanged(int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY);
	void NotifyPawnEnteredCell(int32 CellX, int32 CellY);
	void NotifyPawnExitedCell(int32 CellX, int32 CellY);
	bool ExecuteLinksFromObjectForEvent(FGuid SourceObjectId, EGridObjectEvent SourceEvent);
	bool RefreshPressurePlatesAtCell(int32 X, int32 Y);
	bool RefreshAllPressurePlates();

	/** Transitional caller adapter. RebuildIndexes no longer stores this DTO. */
	void RegisterInitialObjectState(const FGridLevelObjectData& ObjectData);

	void RebuildIndexes();

	/** Reloads the MON19 Lua VM from the current LevelAsset scripts. */
	bool ReloadLuaRuntime(FString* OutError = nullptr);

	const TSet<FGuid>& GetActiveObjectIds() const
	{
		return ActiveObjectIds;
	}
	void SetActiveObjectIds(const TSet<FGuid>& InActiveObjectIds);

	FString GetDebugSummary() const;
	void LogDebugSummary() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

	UPROPERTY(Transient)
	TSet<FGuid> ActiveObjectIds;

	/** MON20.4.6 session-local memory of declined automatic recruitment offers. */
	TSet<FString> DeclinedStoryCompanionOfferKeys;
	TSet<FGuid> DispatchingSourceObjectIds;

	/** One Lua VM per active runtime component/level. Not serialized. */
	FGridLuaVm LuaVm;
	bool bExecutingLuaCallback = false;
	int32 RuntimeDispatchDepth = 0;
	int32 RuntimeActionBudgetRemaining = 0;

private:
	static constexpr int32 MaxRuntimeActionBudget = 128;

	bool ContainsIndexedObject(FGuid ObjectId) const;
	FGuid FindInteractableObjectOnEdge(int32 X, int32 Y, EGridEdge Edge) const;
	bool ActivateObject(FGuid ObjectId, AGrimrockPartyPawn* PartyPawn);

	bool ExecuteLinksFromObjectForEventInternal(FGuid SourceObjectId, EGridObjectEvent SourceEvent);
	bool ApplyLinkCommand(const FGridObjectLink& LinkData);
	bool ApplyQuestLinkCommand(const FGridObjectLink& LinkData);
	void RegisterCurrentLevelQuestDefinitions();
	bool ExecuteLuaCallbackLink(const FGridObjectLink& LinkData);
	bool ExecuteLuaIssuedCommand(FGuid SourceObjectId, const FString& TargetObjectId, const FString& CommandName, FString& OutError);
	bool ConsumeRuntimeActionBudget(const TCHAR* ActionLabel);
	bool EvaluateGridObjectLinkCondition(const FGridObjectLink& LinkData, AActor* SourceActor, AActor* TargetActor) const;
	bool ApplyDoorLinkCommand(const FGridWorldObjectInstance& TargetObject, EGridObjectCommand Command);
	bool ApplyPitLinkCommand(const FGridWorldObjectInstance& TargetObject, EGridObjectCommand Command);
	bool ApplyReceptacleLinkCommand(const FGridWorldObjectInstance& TargetObject, EGridObjectCommand Command);
	bool ApplyStatefulLinkCommand(const FGridWorldObjectInstance& TargetObject, EGridObjectCommand Command);
	bool SetTargetActiveState(const FGridWorldObjectInstance& TargetObject, bool bActive);
	bool IsTargetActive(FGuid ObjectId) const;
	void LogLinkResult(const FGridObjectLink& LinkData, EGridObjectCommand ResolvedCommand, bool bSuccess, const TCHAR* FailureReason) const;

	static FString BuildStoryCompanionOfferKey(FGuid SourceObjectId, FGuid CharacterId);
	bool IsStoryCompanionOfferDeclined(FGuid SourceObjectId, FGuid CharacterId) const;
	void RememberStoryCompanionOfferDeclined(FGuid SourceObjectId, FGuid CharacterId);
	static bool IsStoryCompanionAlreadyActive(const FGridPartyInventoryState& PartyState, const URPGStoryCompanionAsset& CompanionDefinition);

	bool ProcessTriggersAtCell(int32 X, int32 Y, bool bEntering);
	bool ProcessTriggerEvent(const FGridWorldObjectInstance& TriggerData, bool bEntering);

private:
	/** WORLDOBJ-MIG09-E2C-FINAL-A: indexes contain identity only; typed placement arrays remain authoritative. */
	TSet<FGuid> IndexedObjectIds;
	TMultiMap<FGuid, int32> LinkIndexesBySource;
	TMap<FGridEdgeKey, FGuid> InteractableObjectIdByEdge;
	TMultiMap<FIntPoint, FGuid> PressurePlateIdsByCell;
	TMultiMap<FIntPoint, FGuid> TriggerIdsByCell;

	int32 GetIndexedObjectCount() const
	{
		return IndexedObjectIds.Num();
	}
	int32 GetIndexedLinkCount() const
	{
		return LinkIndexesBySource.Num();
	}
	int32 GetActiveObjectCount() const
	{
		return ActiveObjectIds.Num();
	}
	bool ActivateReadableObject(const FGridWorldObjectInstance& ObjectData);
	bool ActivateReceptacle(const FGridWorldObjectInstance& ObjectData, AGrimrockPartyPawn* PartyPawn);

	friend class FGridMON2046RecruitmentOfferDeclineSourceScopeTest;
	friend class FGridMON2046RecruitmentAlreadyActiveSuppressionTest;
};