#include "Runtime/GridLevelRuntimeActor.h"

#include "Runtime/Map/GridMapExplorationState.h"
#include "Runtime/Map/GridMapRevealService.h"

int32 AGridLevelRuntimeActor::RevealMapAroundCell(int32 CellX, int32 CellY)
{
	if (!LevelAsset || LevelAsset->Width != FGridMapExplorationState::GridSize || LevelAsset->Height != FGridMapExplorationState::GridSize)
	{
		return 0;
	}

	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		return 0;
	}

	return FGridMapRevealService::RevealAroundCell(*LevelAsset, DoorSystemComponent, FIntPoint(CellX, CellY), RuntimeState->MapExploration);
}

bool AGridLevelRuntimeActor::TryDiscoverMapSecretDoor(FGuid ObjectId, bool& bOutNewlyDiscovered)
{
	bOutNewlyDiscovered = false;
	if (!LevelAsset || !DoorSystemComponent || !ObjectId.IsValid())
	{
		return false;
	}

	const FGridWorldObjectInstance* Instance = LevelAsset->FindWorldObjectInstanceById(ObjectId);
	if (!Instance || Instance->Type != EGridLevelObjectType::Door ||
		!DoorSystemComponent->IsSecretDoorOnEdge(Instance->CellX, Instance->CellY, Instance->WallSide))
	{
		return false;
	}

	FGridLevelRuntimeState* RuntimeState = GetOrCreateRuntimeStateForCurrentLevel();
	return RuntimeState && RuntimeState->MapExploration.TryMarkSecretDiscovered(ObjectId, bOutNewlyDiscovered);
}
