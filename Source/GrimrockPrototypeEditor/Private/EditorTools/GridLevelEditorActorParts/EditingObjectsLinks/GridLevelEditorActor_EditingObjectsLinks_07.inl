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

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	for (FGridLevelObjectData& Obj : LevelAsset->Objects)
	{
		if (Obj.ObjectId != LastSelectedObjectId)
		{
			continue;
		}

		Obj.Behavior = NewBehavior;
		ObjectBehavior = NewBehavior;

#if WITH_EDITOR
		LevelAsset->MarkPackageDirty();
#endif

		RebuildPreview();
		return true;
	}

	return false;
}

bool AGridLevelEditorActor::ResetSelectedObjectBehaviorFromArchetype()
{
	if (!HasValidLevelAsset() || !LastSelectedObjectId.IsValid())
	{
		return false;
	}

	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj)
	{
		return false;
	}

	const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById(Obj->ArchetypeId);
	if (!Archetype)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->Behavior = Archetype->DefaultBehavior;
	ObjectBehavior = Obj->Behavior;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectArchetypeId(FName NewArchetypeId)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->ArchetypeId = NewArchetypeId;
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
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->ItemDefinitionAsset = NewItemDefinitionAsset;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionId(FName NewItemDefinitionId)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->ItemDefinitionId = NewItemDefinitionId;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SyncSelectedItemDefinitionIdFromAsset()
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::Item || !Obj->ItemDefinitionAsset)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->ItemDefinitionId = Obj->ItemDefinitionAsset->ItemDefinitionId;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectMonsterDefinitionAsset(UGridMonsterDefinitionAsset* NewMonsterDefinitionAsset)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->MonsterDefinitionAsset = NewMonsterDefinitionAsset;
	if (NewMonsterDefinitionAsset)
	{
		Obj->MonsterDefinitionId = NewMonsterDefinitionAsset->MonsterId;
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}
