bool AGridLevelEditorActor::SetSelectedObjectMonsterDefinitionId(FName NewMonsterDefinitionId)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->MonsterDefinitionId = NewMonsterDefinitionId;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SyncSelectedMonsterDefinitionIdFromAsset()
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->MonsterDefinitionAsset)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->MonsterDefinitionId = Obj->MonsterDefinitionAsset->MonsterId;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectEncounterGroupId(FName NewEncounterGroupId)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->EncounterGroupId = NewEncounterGroupId;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectEncounterWaveIndex(int32 NewEncounterWaveIndex)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->EncounterWaveIndex = FMath::Max(0, NewEncounterWaveIndex);

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableContentAsset(UGridReadableContentAsset* NewReadableContentAsset)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	Obj->ReadableContentAsset = NewReadableContentAsset;
	if (NewReadableContentAsset && Obj->ReadableContentId.IsNone())
	{
		Obj->ReadableContentId = NewReadableContentAsset->ReadableContentId;
	}
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableContentId(FName NewReadableContentId)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}
#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	Obj->ReadableContentId = NewReadableContentId;
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadTitleOverride(const FText& NewReadTitleOverride)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}
#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	Obj->ReadTitleOverride = NewReadTitleOverride;
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadTextOverride(const FText& NewReadTextOverride)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj || Obj->Type != EGridLevelObjectType::Item)
	{
		return false;
	}
#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	Obj->ReadTextOverride = NewReadTextOverride;
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectTag(FName NewTag)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->Tag = NewTag;
	ObjectTag = NewTag;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectNotes(const FString& NewNotes)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!Obj)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	Obj->Notes = NewNotes;
	ObjectNotes = NewNotes;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableText(const FText& NewReadableText)
{
	FGridLevelObjectData* SelectedObject = FindSelectedObjectMutable();
	if (!SelectedObject || !LevelAsset)
