#include "Core/GridObjectInstanceBehavior.h"

bool AGridLevelEditorActor::FocusSelectedObject()
{
	if (!LevelAsset || !LevelAsset->TryGetTypedPlacementLocation(LastSelectedObjectId, SelectedCellX, SelectedCellY, SelectedEdge)) return false;
#if WITH_EDITOR
	if (GEditor)
	{
		FVector WorldLocation;
		if (TryGetObjectWorldLocationById(LastSelectedObjectId, WorldLocation))
		{
			const float FocusExtent = FMath::Max(50.f, LevelAsset->CellSize * 0.25f);
			GEditor->MoveViewportCamerasToBox(FBox(WorldLocation - FVector(FocusExtent), WorldLocation + FVector(FocusExtent)), false);
		}
	}
#endif
	return true;
}

bool AGridLevelEditorActor::ApplyBehaviorToSelectedObject(const FGridObjectBehaviorParams& NewBehavior)
{
	FGridWorldObjectInstance* WorldObjectInstance = LevelAsset ? LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId) : nullptr;
	if (!WorldObjectInstance) return false;
	LevelAsset->Modify();
	WorldObjectInstance->InstanceConfig.Teleporter = NewBehavior.Teleporter;
	WorldObjectInstance->InstanceConfig.Transition = NewBehavior.Transition;
	WorldObjectInstance->InstanceConfig.Pit = NewBehavior.Pit;
	WorldObjectInstance->InstanceConfig.ReceptacleInitialContent = NewBehavior.Receptacle.InitialContent;
	WorldObjectInstance->InstanceConfig.bStartsUnlocked = NewBehavior.Lock.bStartsUnlocked;
	ObjectBehavior = GridObjectInstanceBehavior::Resolve(*WorldObjectInstance, FindWorldObjectDefinitionById(WorldObjectInstance->WorldObjectDefinitionId));
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::ResetSelectedObjectBehaviorFromDefinition()
{
	const FGridWorldObjectInstance* WorldObjectInstance = LevelAsset ? LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId) : nullptr;
	const UGridWorldObjectDefinitionAsset* Definition = WorldObjectInstance ? FindWorldObjectDefinitionById(WorldObjectInstance->WorldObjectDefinitionId) : nullptr;
	return Definition && ApplyBehaviorToSelectedObject(Definition->DefaultBehavior);
}

bool AGridLevelEditorActor::SetSelectedWorldObjectDefinitionId(FName NewWorldObjectDefinitionId)
{
	FGridWorldObjectInstance* WorldObjectInstance = LevelAsset ? LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId) : nullptr;
	if (!WorldObjectInstance) return false;
	LevelAsset->Modify();
	WorldObjectInstance->WorldObjectDefinitionId = NewWorldObjectDefinitionId;
	WorldObjectDefinitionId = NewWorldObjectDefinitionId;
	SelectedWorldObjectDefinitionId = NewWorldObjectDefinitionId;
	ObjectBehavior = GridObjectInstanceBehavior::Resolve(*WorldObjectInstance, FindWorldObjectDefinitionById(NewWorldObjectDefinitionId));
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionAsset(UGridItemDefinitionAsset* NewItemDefinitionAsset)
{
	if (!LevelAsset) return false;
	if (FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		LooseItemInstance->ItemDefinition = NewItemDefinitionAsset;
	}
	else if (FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		ItemSpawn->ItemDefinition = NewItemDefinitionAsset;
	}
	else return false;
	ObjectBehavior.Item.ItemDefinitionAsset = NewItemDefinitionAsset;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionId(FName NewItemDefinitionId)
{
	// Compatibility authoring command: resolve a definition without storing a second identity.
	if (!LevelAsset) return false;
	if (const FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId))
	{
		if (LooseItemInstance->ItemDefinition && LooseItemInstance->ItemDefinition->ItemDefinitionId == NewItemDefinitionId) return true;
	}
	if (ObjectPalette)
	{
		for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
		{
			if (Entry.DefaultItemDefinition && Entry.DefaultItemDefinition->ItemDefinitionId == NewItemDefinitionId)
				return SetSelectedObjectItemDefinitionAsset(Entry.DefaultItemDefinition);
		}
	}
	return false;
}

bool AGridLevelEditorActor::SyncSelectedItemDefinitionIdFromAsset()
{
	const FGridLooseItemInstance* LooseItemInstance = LevelAsset ? LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId) : nullptr;
	return LooseItemInstance && LooseItemInstance->ItemDefinition;
}

bool AGridLevelEditorActor::SetSelectedObjectMonsterDefinitionAsset(UGridMonsterDefinitionAsset* NewMonsterDefinitionAsset)
{
	FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset ? LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId) : nullptr;
	if (!MonsterSpawn) return false;
	LevelAsset->Modify();
	MonsterSpawn->MonsterDefinition = NewMonsterDefinitionAsset;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}
