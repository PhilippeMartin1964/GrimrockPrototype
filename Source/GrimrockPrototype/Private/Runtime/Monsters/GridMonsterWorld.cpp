#include "GridMonsterWorld.h"

#include "EngineUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"

void GridMonsterWorld::CollectWorldMonsters(const UWorld* World, TArray<AGridMonsterActor*>& OutMonsters)
{
	OutMonsters.Reset();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AGridMonsterActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (IsValid(*It))
		{
			OutMonsters.Add(*It);
		}
	}

	OutMonsters.Sort(
		[](const AGridMonsterActor& Left, const AGridMonsterActor& Right)
		{
			const FGuid LeftId = Left.ResolvePersistenceId();
			const FGuid RightId = Right.ResolvePersistenceId();
			if (LeftId.IsValid() != RightId.IsValid())
			{
				return LeftId.IsValid();
			}
			if (LeftId != RightId)
			{
				return LeftId.ToString(EGuidFormats::Digits) < RightId.ToString(EGuidFormats::Digits);
			}
			return Left.GetPathName() < Right.GetPathName();
		});
}

AGridLevelRuntimeActor* GridMonsterWorld::FindRuntimeActor(const AGridMonsterActor* Monster)
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
