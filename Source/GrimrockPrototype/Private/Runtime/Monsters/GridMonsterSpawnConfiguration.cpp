#include "Runtime/Monsters/GridMonsterActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	AGridLevelRuntimeActor* FindSpawnRuntimeActor(const AGridMonsterActor* Monster)
	{
		if (!IsValid(Monster))
		{
			return nullptr;
		}

		if (AGridLevelRuntimeActor* OwnerRuntime = Cast<AGridLevelRuntimeActor>(Monster->GetOwner()))
		{
			return OwnerRuntime;
		}

		UWorld* World = Monster->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}
}

void AGridMonsterActor::ApplySpawnPlacementConfiguration()
{
	MonsterState = EGridMonsterState::Idle;
	PatrolMode = EGridMonsterPatrolMode::None;
	PatrolWaypoints.Reset();

	AGridLevelRuntimeActor* RuntimeActor = FindSpawnRuntimeActor(this);
	const FGridLevelObjectData* SpawnData =
		IsValid(RuntimeActor) && RuntimeActor->LevelAsset && SpawnObjectId.IsValid() ? RuntimeActor->LevelAsset->FindMonsterSpawnById(SpawnObjectId) : nullptr;
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

	EncounterGroupId = SpawnData->EncounterGroupId;
	PatrolMode = SpawnData->PatrolMode;
	PatrolWaypoints = SpawnData->PatrolWaypoints;
}
