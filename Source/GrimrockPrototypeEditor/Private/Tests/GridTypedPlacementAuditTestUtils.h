#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace GridTypedPlacementAuditTestUtils
{
	/** Visit definition references in native placements; never construct a placement projection. */
	template <typename TAuditReference>
	void AuditDefinitionReferences(const UGridLevelAsset& Level, TAuditReference&& Audit)
	{
		for (const FGridWorldObjectInstance& Instance : Level.WorldObjectInstances)
		{
			for (int32 Index = 0; Index < Instance.InstanceConfig.ReceptacleInitialContent.Num(); ++Index)
			{
				const FName ItemId = Instance.InstanceConfig.ReceptacleInitialContent[Index].ItemId;
				// Native canonical initial content is audited in X.48/X.58.
				// Do not report the intentional absence of a retired pointer as TD07 debt.
				Audit(FString::Printf(TEXT("WorldObjectInstances[%s].ReceptacleInitialContent[%d]"), *Instance.InstanceId.ToString(), Index),
					nullptr, ItemId, NAME_None, TEXT("Item"));
			}
		}
		for (const FGridLooseItemInstance& Instance : Level.LooseItemInstances)
		{
			const FString Context = FString::Printf(TEXT("LooseItemInstances[%s]"), *Instance.InstanceId.ToString());
			Audit(Context + TEXT(".ItemId"), nullptr, Instance.ItemId, NAME_None, TEXT("Item"));
			const UGridReadableContentAsset* Readable = Instance.ReadableContentAsset;
			Audit(Context + TEXT(".ReadableContent"), Readable, Readable ? Readable->ReadableContentId : NAME_None, Instance.ReadableContentId,
				TEXT("ReadableContent"));
		}
		for (const FGridMonsterSpawnInstance& Spawn : Level.MonsterSpawns)
		{
			const UGridMonsterDefinitionAsset* Monster = Spawn.MonsterDefinition;
			Audit(FString::Printf(TEXT("MonsterSpawns[%s].MonsterDefinition"), *Spawn.SpawnId.ToString()), Monster,
				Monster ? Monster->MonsterId : NAME_None, NAME_None, TEXT("Monster"));
		}
		for (const FGridItemSpawnInstance& Spawn : Level.ItemSpawns)
		{
			Audit(FString::Printf(TEXT("ItemSpawns[%s].ItemId"), *Spawn.SpawnId.ToString()), nullptr,
				Spawn.ItemId, NAME_None, TEXT("Item"));
		}
	}
}

#endif
