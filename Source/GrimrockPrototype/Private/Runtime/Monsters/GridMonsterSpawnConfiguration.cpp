#include "Runtime/Monsters/GridMonsterActor.h"
#include "GridMonsterWorld.h"

#include "Core/GridLevelAsset.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"

void AGridMonsterActor::ApplySpawnPlacementConfiguration()
{
	MonsterState = EGridMonsterState::Idle;
	PatrolMode = EGridMonsterPatrolMode::None;
	PatrolWaypoints.Reset();

	AGridLevelRuntimeActor* RuntimeActor = GridMonsterWorld::FindRuntimeActor(this);
	const FGridMonsterSpawnInstance* SpawnData =
		IsValid(RuntimeActor) && RuntimeActor->LevelAsset && SpawnObjectId.IsValid()
			? RuntimeActor->LevelAsset->MonsterSpawns.FindByPredicate(
				[SpawnObjectId = SpawnObjectId](const FGridMonsterSpawnInstance& Spawn)
				{
					return Spawn.SpawnId == SpawnObjectId;
				})
			: nullptr;
	if (!SpawnData)
	{
		return;
	}

	// Fresh placement state is deliberately limited to present exploration
	// states. Save/Continue restoration may later replace it with any valid
	// runtime monster state.
	if (SpawnData->InitialMonsterState == EGridMonsterState::Dormant)
	{
		MonsterState = EGridMonsterState::Dormant;
	}

	RuntimeActor->ApplyMonsterPlacementMetadata(this);
}
