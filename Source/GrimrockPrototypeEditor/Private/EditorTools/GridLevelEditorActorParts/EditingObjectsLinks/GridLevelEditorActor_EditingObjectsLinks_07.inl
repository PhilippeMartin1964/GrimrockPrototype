#include "Core/GridObjectInstanceBehavior.h"

bool AGridLevelEditorActor::FocusSelectedObject()
{
	if (!LastSelectedObjectId.IsValid())
	{
		return false;
	}

	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj)
	{
		return false;
	}

	SelectedCellX = Obj->CellX;
	SelectedCellY = Obj->CellY;
	SelectedEdge = Obj->Edge;

#if WITH_EDITOR
	if (GEditor)
	{
		FVector WorldLocation = FVector::ZeroVector;
		if (TryGetObjectWorldLocation(*Obj, WorldLocation))
		{
			const float FocusExtent = LevelAsset ? FMath::Max(50.f, LevelAsset->CellSize * 0.25f) : 50.f;
			GEditor->MoveViewportCamerasToBox(FBox(WorldLocation - FVector(FocusExtent), WorldLocation + FVector(FocusExtent)), false);
		}
	}
#endif

	return true;
}

bool AGridLevelEditorActor::ApplyBehaviorToSelectedObject(const FGridObjectBehaviorParams& NewBehavior)
{
	if (!HasValidLevelAsset() || !LastSelectedObjectId.IsValid())
	{
		return false;
	}

	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.Behavior = GridObjectInstanceBehavior::BuildSparseOverrides(NewBehavior);

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}
	ObjectBehavior = NewBehavior;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::ResetSelectedObjectBehaviorFromArchetype()
{
	if (!HasValidLevelAsset() || !LastSelectedObjectId.IsValid())
	{
		return false;
	}

	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj)
	{
		return false;
	}

	const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById(Obj->ArchetypeId);
	if (!Archetype)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	// Reset means "use the definition again", not "clone the definition again".
	EditedObject.Behavior = GridObjectInstanceBehavior::BuildSparseOverrides(Archetype->DefaultBehavior);

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}
	ObjectBehavior = Archetype->DefaultBehavior;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectArchetypeId(FName NewArchetypeId)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.ArchetypeId = NewArchetypeId;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}
	ObjectArchetypeId = NewArchetypeId;
	SelectedArchetypeId = NewArchetypeId;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionAsset(UGridItemDefinitionAsset* NewItemDefinitionAsset)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.ItemDefinitionAsset = NewItemDefinitionAsset;
	EditedObject.ItemDefinitionId = NAME_None;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionId(FName NewItemDefinitionId)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.ItemDefinitionId = NewItemDefinitionId;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SyncSelectedItemDefinitionIdFromAsset()
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}

	UGridItemDefinitionAsset* DefinitionAsset = Obj->ItemDefinitionAsset;
	if (!DefinitionAsset)
	{
		if (const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById(Obj->ArchetypeId))
		{
			DefinitionAsset = Archetype->DefaultBehavior.Item.ItemDefinitionAsset;
		}
	}

	if (!DefinitionAsset)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	// TD07 current-schema repair: promote the direct asset reference and clear
	// the redundant authoring id instead of recreating Asset+Id dual authority.
	EditedObject.ItemDefinitionAsset = DefinitionAsset;
	EditedObject.ItemDefinitionId = NAME_None;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectMonsterDefinitionAsset(UGridMonsterDefinitionAsset* NewMonsterDefinitionAsset)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.MonsterDefinitionAsset = NewMonsterDefinitionAsset;
	if (NewMonsterDefinitionAsset)
	{
		EditedObject.MonsterDefinitionId = NewMonsterDefinitionAsset->MonsterId;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}