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
