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
				const UGridItemDefinitionAsset* Item = Instance.InstanceConfig.ReceptacleInitialContent[Index].ItemDefinition;
				Audit(FString::Printf(TEXT("WorldObjectInstances[%s].ReceptacleInitialContent[%d]"), *Instance.InstanceId.ToString(), Index),
					Item, Item ? Item->ItemDefinitionId : NAME_None, NAME_None, TEXT("Item"));
			}
		}
		for (const FGridLooseItemInstance& Instance : Level.LooseItemInstances)
		{
			const FString Context = FString::Printf(TEXT("LooseItemInstances[%s]"), *Instance.InstanceId.ToString());
			const UGridItemDefinitionAsset* Item = Instance.ItemDefinition;
			Audit(Context + TEXT(".ItemDefinition"), Item, Item ? Item->ItemDefinitionId : NAME_None, NAME_None, TEXT("Item"));
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
			const UGridItemDefinitionAsset* Item = Spawn.ItemDefinition;
			Audit(FString::Printf(TEXT("ItemSpawns[%s].ItemDefinition"), *Spawn.SpawnId.ToString()), Item,
				Item ? Item->ItemDefinitionId : NAME_None, NAME_None, TEXT("Item"));
		}
	}
}

#endif
