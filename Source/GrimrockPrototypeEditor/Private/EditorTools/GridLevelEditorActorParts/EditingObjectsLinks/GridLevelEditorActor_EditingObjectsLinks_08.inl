bool AGridLevelEditorActor::SetSelectedObjectMonsterDefinitionId(FName NewMonsterDefinitionId)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.MonsterDefinitionId = NewMonsterDefinitionId;

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

bool AGridLevelEditorActor::SyncSelectedMonsterDefinitionIdFromAsset()
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->MonsterDefinitionAsset)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.MonsterDefinitionId = Obj->MonsterDefinitionAsset->MonsterId;

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

bool AGridLevelEditorActor::SetSelectedObjectEncounterGroupId(FName NewEncounterGroupId)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.EncounterGroupId = NewEncounterGroupId;

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

bool AGridLevelEditorActor::SetSelectedObjectEncounterWaveIndex(int32 NewEncounterWaveIndex)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.EncounterWaveIndex = FMath::Max(0, NewEncounterWaveIndex);

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

bool AGridLevelEditorActor::SetSelectedObjectInitialMonsterState(EGridMonsterState NewInitialMonsterState)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!LevelAsset || !Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn ||
		(NewInitialMonsterState != EGridMonsterState::Idle && NewInitialMonsterState != EGridMonsterState::Dormant))
	{
		return false;
	}
	if (Obj->InitialMonsterState == NewInitialMonsterState)
	{
		return true;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.InitialMonsterState = NewInitialMonsterState;

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

bool AGridLevelEditorActor::SetSelectedObjectReadableContentAsset(UGridReadableContentAsset* NewReadableContentAsset)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.ReadableContentAsset = NewReadableContentAsset;
	if (NewReadableContentAsset && EditedObject.ReadableContentId.IsNone())
	{
		EditedObject.ReadableContentId = NewReadableContentAsset->ReadableContentId;
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

bool AGridLevelEditorActor::SetSelectedObjectReadableContentId(FName NewReadableContentId)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}
	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.ReadableContentId = NewReadableContentId;
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

bool AGridLevelEditorActor::SetSelectedObjectReadTitleOverride(const FText& NewReadTitleOverride)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}
	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.ReadTitleOverride = NewReadTitleOverride;
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

bool AGridLevelEditorActor::SetSelectedObjectReadTextOverride(const FText& NewReadTextOverride)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}
	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.ReadTextOverride = NewReadTextOverride;
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

bool AGridLevelEditorActor::SetSelectedObjectTag(FName NewTag)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.Tag = NewTag;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}
	ObjectTag = NewTag;
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectNotes(const FString& NewNotes)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.Notes = NewNotes;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}
	ObjectNotes = NewNotes;
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableText(const FText& NewReadableText)
{
	const FGridLevelObjectData* SelectedObject = FindObjectById(LastSelectedObjectId);
	if (!SelectedObject || !LevelAsset)