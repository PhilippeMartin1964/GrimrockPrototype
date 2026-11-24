#pragma once

#include "CoreMinimal.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;
class UWorld;

namespace GridMonsterWorld
{
	/** Valid actors in this world, ordered by persistence GUID and then object path. */
	void CollectWorldMonsters(const UWorld* World, TArray<AGridMonsterActor*>& OutMonsters);

	/** Prefer the owner, then the first runtime actor in the monster's world. */
	AGridLevelRuntimeActor* FindRuntimeActor(const AGridMonsterActor* Monster);
}
