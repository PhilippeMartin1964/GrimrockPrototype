#include "Runtime/GridActivationComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/Monsters/GridAutomaticPerceptionEngagementSubsystem.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridActivation, Log, All);

namespace
{
	FString GridObjectEventToString(EGridObjectEvent EventType)
	{
		if (const UEnum* Enum = StaticEnum<EGridObjectEvent>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(EventType));
		}
		return FString::Printf(TEXT("%d"), static_cast<int32>(EventType));
	}

	bool IsReadableGenericObject(EGridLevelObjectType Type)
	{
		return Type == EGridLevelObjectType::Decoration || Type == EGridLevelObjectType::Light;
	}
}

UGridActivationComponent::UGridActivationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridActivationComponent::Initialize(AGridLevelRuntimeActor* InRuntime)
{
	RuntimeActor = InRuntime;

	FString LuaError;
	if (!ReloadLuaRuntime(&LuaError) && !LuaError.IsEmpty())
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid Lua runtime initialization failed: %s"), *LuaError);
	}
}

void UGridActivationComponent::ResetRuntimeState()
{
	ActiveObjectIds.Reset();
	IndexedObjectIds.Reset();
	LinkIndexesBySource.Reset();
	InteractableObjectIdByEdge.Reset();
	PressurePlateIdsByCell.Reset();
	TriggerIdsByCell.Reset();
	DeclinedStoryCompanionOfferKeys.Reset();
	DispatchingSourceObjectIds.Reset();
	RuntimeDispatchDepth = 0;
	RuntimeActionBudgetRemaining = 0;
	bExecutingLuaCallback = false;
	LuaVm.Reset();
}

void UGridActivationComponent::SetActiveObjectIds(const TSet<FGuid>& InActiveObjectIds)
{
	ActiveObjectIds = InActiveObjectIds;
}

bool UGridActivationComponent::TryInteractAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn)
{
	const FGuid ObjectId = FindInteractableObjectOnEdge(FromCellX, FromCellY, Edge);
	return ObjectId.IsValid() && ActivateObject(ObjectId, PartyPawn);
}

AGridReceptacleActor* UGridActivationComponent::FindReceptacleAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge) const
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return nullptr;
	}

	const FGuid ObjectId = FindInteractableObjectOnEdge(FromCellX, FromCellY, Edge);
	const FGridWorldObjectInstance* ObjectData = RuntimeActor->LevelAsset->FindWorldObjectInstanceById(ObjectId);
	if (!ObjectData || ObjectData->Type != EGridLevelObjectType::Receptacle)
	{
		return nullptr;
	}

	return RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor>(ObjectId);
}

void UGridActivationComponent::HandlePartyCellChanged(int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
	if (OldCellX == NewCellX && OldCellY == NewCellY)
	{
		RefreshPressurePlatesAtCell(NewCellX, NewCellY);
		NotifyPawnEnteredCell(NewCellX, NewCellY);
		GridAutomaticPerceptionEngagement::Request(RuntimeActor, TEXT("PartyCellStable"));
		return;
	}

	RefreshPressurePlatesAtCell(OldCellX, OldCellY);
	RefreshPressurePlatesAtCell(NewCellX, NewCellY);
	NotifyPawnExitedCell(OldCellX, OldCellY);
	NotifyPawnEnteredCell(NewCellX, NewCellY);
	GridAutomaticPerceptionEngagement::Request(RuntimeActor, TEXT("PartyTranslationCompleted"));
}

void UGridActivationComponent::NotifyPawnEnteredCell(int32 CellX, int32 CellY)
{
	ProcessTriggersAtCell(CellX, CellY, true);
}

void UGridActivationComponent::NotifyPawnExitedCell(int32 CellX, int32 CellY)
{
	ProcessTriggersAtCell(CellX, CellY, false);
}

bool UGridActivationComponent::ContainsIndexedObject(FGuid ObjectId) const
{
	return ObjectId.IsValid() && IndexedObjectIds.Contains(ObjectId);
}

FGuid UGridActivationComponent::FindInteractableObjectOnEdge(int32 X, int32 Y, EGridEdge Edge) const
{
	const FGuid* ObjectId = InteractableObjectIdByEdge.Find(FGridEdgeKey(X, Y, Edge));
	return ObjectId ? *ObjectId : FGuid();
}

bool UGridActivationComponent::ActivateObject(FGuid ObjectId, AGrimrockPartyPawn* PartyPawn)
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return false;
	}

	const FGridWorldObjectInstance* ObjectData = RuntimeActor->LevelAsset->FindWorldObjectInstanceById(ObjectId);
	if (!ObjectData)
	{
		return false;
	}

	switch (ObjectData->Type)
	{
		case EGridLevelObjectType::Button:
		{
			if (AGridButtonActor* ButtonActor = RuntimeActor->FindRuntimeObjectActor<AGridButtonActor>(ObjectId))
			{
				ButtonActor->TriggerPress();
			}
			return ExecuteLinksFromObjectForEvent(ObjectId, EGridObjectEvent::Activated);
		}

		case EGridLevelObjectType::Lever:
		{
			const bool bWasActive = ActiveObjectIds.Contains(ObjectId);
			const bool bNewActive = !bWasActive;
			if (bNewActive)
			{
				ActiveObjectIds.Add(ObjectId);
			}
			else
			{
				ActiveObjectIds.Remove(ObjectId);
			}
			if (AGridLeverActor* LeverActor = RuntimeActor->FindRuntimeObjectActor<AGridLeverActor>(ObjectId))
			{
				LeverActor->SetLeverState(bNewActive);
			}
			const EGridObjectEvent LeverEvent = bNewActive ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
			return ExecuteLinksFromObjectForEvent(ObjectId, LeverEvent);
		}

		case EGridLevelObjectType::Receptacle:
		{
			if (AGridWallLockActor* WallLockActor = RuntimeActor->FindRuntimeObjectActor<AGridWallLockActor>(ObjectId))
			{
				return WallLockActor->TryInteractWithParty(PartyPawn);
			}
			return ActivateReceptacle(*ObjectData, PartyPawn);
		}

		case EGridLevelObjectType::Decoration:
		case EGridLevelObjectType::Light:
			return ActivateReadableObject(*ObjectData);

		default:
			return false;
	}
}

bool UGridActivationComponent::RefreshPressurePlatesAtCell(int32 X, int32 Y)
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return false;
	}

	TArray<FGuid> PlateIds;
	PressurePlateIdsByCell.MultiFind(FIntPoint(X, Y), PlateIds);
	bool bAnyStateChanged = false;

	for (const FGuid& PlateId : PlateIds)
	{
		const FGridWorldObjectInstance* PlateData = RuntimeActor->LevelAsset->FindWorldObjectInstanceById(PlateId);
		if (!PlateData || PlateData->Type != EGridLevelObjectType::PressurePlate)
		{
			continue;
		}

		const UGridObjectArchetypeAsset* PlateArchetype = RuntimeActor->FindObjectArchetype(PlateData->WorldObjectDefinitionId);
		const FGridObjectBehaviorParams EffectiveBehavior = GridObjectInstanceBehavior::Resolve(FGridRuntimeWorldObjectData(*PlateData), PlateArchetype);
		const FGridPressurePlateWeightParams& WeightParams = EffectiveBehavior.PressurePlateWeight;
		const float CurrentItemWeight = RuntimeActor->GetWorldItemWeightAtCell(X, Y, WeightParams.bCountEdgeItems);
		const bool bPartyActivates = WeightParams.bActivateWhenPartyPresent && RuntimeActor->IsPartyOnCell(X, Y);
		const bool bWeightActivates = WeightParams.bUseItemWeight && CurrentItemWeight >= FMath::Max(0.0f, WeightParams.RequiredItemWeight);
		const bool bShouldBePressed = bPartyActivates || bWeightActivates;
		const bool bWasPressed = ActiveObjectIds.Contains(PlateId);

		if (AGridPressurePlateActor* PlateActor = RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor>(PlateId))
		{
			PlateActor->SetWeightState(CurrentItemWeight, WeightParams.RequiredItemWeight, WeightParams.bUseItemWeight, WeightParams.bActivateWhenPartyPresent);
		}

		if (bWasPressed == bShouldBePressed)
		{
			continue;
		}

		if (bShouldBePressed)
		{
			ActiveObjectIds.Add(PlateId);
		}
		else
		{
			ActiveObjectIds.Remove(PlateId);
		}

		if (AGridPressurePlateActor* PlateActor = RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor>(PlateId))
		{
			PlateActor->SetPressed(bShouldBePressed);
		}

		const EGridObjectEvent StateEvent = bShouldBePressed ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
		UE_LOG(LogGridActivation, Log, TEXT("GridPressurePlate StateChanged Id=%s Cell=(%d,%d) Party=%s ItemWeight=%.2f RequiredWeight=%.2f Pressed=%s"),
			*PlateId.ToString(), X, Y, bPartyActivates ? TEXT("true") : TEXT("false"), CurrentItemWeight, WeightParams.RequiredItemWeight,
			bShouldBePressed ? TEXT("true") : TEXT("false"));
		ExecuteLinksFromObjectForEvent(PlateId, StateEvent);
		bAnyStateChanged = true;
	}

	return bAnyStateChanged;
}

bool UGridActivationComponent::RefreshAllPressurePlates()
{
	TArray<FIntPoint> PlateCells;
	PressurePlateIdsByCell.GetKeys(PlateCells);
	TSet<FIntPoint> UniquePlateCells;
	UniquePlateCells.Append(PlateCells);

	bool bAnyStateChanged = false;
	for (const FIntPoint& PlateCell : UniquePlateCells)
	{
		bAnyStateChanged |= RefreshPressurePlatesAtCell(PlateCell.X, PlateCell.Y);
	}
	return bAnyStateChanged;
}

void UGridActivationComponent::RegisterInitialObjectState(const FGridWorldObjectInstance& Instance)
{
	if (Instance.bInitiallyActive && Instance.InstanceId.IsValid())
	{
		ActiveObjectIds.Add(Instance.InstanceId);
	}
}


void UGridActivationComponent::RebuildIndexes()
{
	IndexedObjectIds.Reset();
	LinkIndexesBySource.Reset();
	InteractableObjectIdByEdge.Reset();
	PressurePlateIdsByCell.Reset();
	TriggerIdsByCell.Reset();

	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return;
	}

	RegisterCurrentLevelQuestDefinitions();
	const UGridLevelAsset& Level = *RuntimeActor->LevelAsset;

	for (const FGridWorldObjectInstance& ObjectData : Level.WorldObjectInstances)
	{
		if (!ObjectData.InstanceId.IsValid())
		{
			continue;
		}
		IndexedObjectIds.Add(ObjectData.InstanceId);

		if (ObjectData.Type == EGridLevelObjectType::Button || ObjectData.Type == EGridLevelObjectType::Lever ||
			ObjectData.Type == EGridLevelObjectType::Receptacle || IsReadableGenericObject(ObjectData.Type))
		{
			if (ObjectData.WallSide != EGridEdge::None)
			{
				InteractableObjectIdByEdge.Add(FGridEdgeKey(ObjectData.CellX, ObjectData.CellY, ObjectData.WallSide), ObjectData.InstanceId);
			}
		}
		else if (ObjectData.Type == EGridLevelObjectType::PressurePlate)
		{
			PressurePlateIdsByCell.Add(FIntPoint(ObjectData.CellX, ObjectData.CellY), ObjectData.InstanceId);
		}
		else if (ObjectData.Type == EGridLevelObjectType::Trigger)
		{
			TriggerIdsByCell.Add(FIntPoint(ObjectData.CellX, ObjectData.CellY), ObjectData.InstanceId);
		}
	}

	for (const FGridLooseItemInstance& Instance : Level.LooseItemInstances)
	{
		if (Instance.InstanceId.IsValid())
		{
			IndexedObjectIds.Add(Instance.InstanceId);
		}
	}
	for (const FGridMonsterSpawnInstance& Spawn : Level.MonsterSpawns)
	{
		if (Spawn.SpawnId.IsValid())
		{
			IndexedObjectIds.Add(Spawn.SpawnId);
		}
	}
	for (const FGridItemSpawnInstance& Spawn : Level.ItemSpawns)
	{
		if (Spawn.SpawnId.IsValid())
		{
			IndexedObjectIds.Add(Spawn.SpawnId);
		}
	}
	for (const FGridLogicObjectInstance& Instance : Level.LogicObjects)
	{
		if (Instance.InstanceId.IsValid())
		{
			IndexedObjectIds.Add(Instance.InstanceId);
		}
	}

	for (int32 Index = 0; Index < Level.Links.Num(); ++Index)
	{
		if (Level.Links[Index].SourceObjectId.IsValid())
		{
			LinkIndexesBySource.Add(Level.Links[Index].SourceObjectId, Index);
		}
	}
}

bool UGridActivationComponent::ProcessTriggersAtCell(int32 X, int32 Y, bool bEntering)
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return false;
	}

	TArray<FGuid> TriggerIds;
	TriggerIdsByCell.MultiFind(FIntPoint(X, Y), TriggerIds);
	bool bAnyTriggered = false;
	for (const FGuid& TriggerId : TriggerIds)
	{
		if (const FGridWorldObjectInstance* TriggerData = RuntimeActor->LevelAsset->FindWorldObjectInstanceById(TriggerId))
		{
			bAnyTriggered |= ProcessTriggerEvent(*TriggerData, bEntering);
		}
	}
	return bAnyTriggered;
}

bool UGridActivationComponent::ProcessTriggerEvent(const FGridWorldObjectInstance& TriggerData, bool bEntering)
{
	if (TriggerData.Type != EGridLevelObjectType::Trigger || !TriggerData.InstanceId.IsValid())
	{
		return false;
	}

	const TCHAR* EventLabel = bEntering ? TEXT("Enter") : TEXT("Exit");
	const EGridObjectEvent SourceEvent = bEntering ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
	UE_LOG(LogGridActivation, Log, TEXT("Grid trigger detected: Id=%s Cell=(%d,%d) Event=%s SourceEvent=%s"), *TriggerData.InstanceId.ToString(),
		TriggerData.CellX, TriggerData.CellY, EventLabel, *GridObjectEventToString(SourceEvent));
	return ExecuteLinksFromObjectForEvent(TriggerData.InstanceId, SourceEvent);
}

bool UGridActivationComponent::ActivateReadableObject(const FGridWorldObjectInstance& ObjectData)
{
	if (!RuntimeActor)
	{
		return false;
	}
	AGridGenericObjectActor* GenericActor = RuntimeActor->FindRuntimeObjectActor<AGridGenericObjectActor>(ObjectData.InstanceId);
	if (!GenericActor || !GenericActor->HasReadableText())
	{
		return false;
	}
	if (GenericActor->bRuntimeReadableOnlyOnce && GenericActor->bRuntimeHasBeenRead)
	{
		return true;
	}
	if (GEngine)
	{
		const FText ReadableText = GenericActor->GetReadableText();
		UE_LOG(LogGridActivation, Log, TEXT("Readable object %s: %s"), *ObjectData.InstanceId.ToString(), *ReadableText.ToString());
		RuntimeActor->ShowReadableMessage(ReadableText);
		GenericActor->MarkAsRead();
		return true;
	}
	GenericActor->MarkAsRead();
	return true;
}

bool UGridActivationComponent::ActivateReceptacle(const FGridWorldObjectInstance& ObjectData, AGrimrockPartyPawn* PartyPawn)
{
	if (!RuntimeActor || !PartyPawn)
	{
		return false;
	}
	AGridReceptacleActor* ReceptacleActor = RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor>(ObjectData.InstanceId);
	if (!ReceptacleActor)
	{
		return false;
	}
	const int32 PreviousItemCount = ReceptacleActor->GetContainedItemCount();
	if (!ReceptacleActor->TryInteractWithParty(PartyPawn))
	{
		return false;
	}
	const int32 CurrentItemCount = ReceptacleActor->GetContainedItemCount();
	if (CurrentItemCount > 0)
	{
		ActiveObjectIds.Add(ObjectData.InstanceId);
	}
	else
	{
		ActiveObjectIds.Remove(ObjectData.InstanceId);
	}
	return CurrentItemCount != PreviousItemCount;
}

FString UGridActivationComponent::GetDebugSummary() const
{
	return FString::Printf(TEXT("Activation | Objects=%d Links=%d Interactables=%d Plates=%d Triggers=%d Active=%d LuaScripts=%d"), IndexedObjectIds.Num(),
		LinkIndexesBySource.Num(), InteractableObjectIdByEdge.Num(), PressurePlateIdsByCell.Num(), TriggerIdsByCell.Num(), ActiveObjectIds.Num(),
		LuaVm.GetLoadedScriptCount());
}

void UGridActivationComponent::LogDebugSummary() const
{
	UE_LOG(LogGridActivation, Log, TEXT("%s"), *GetDebugSummary());
}
