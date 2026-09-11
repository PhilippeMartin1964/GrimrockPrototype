bool AGridLevelEditorActor::SetSelectedObjectMonsterDefinitionId(FName NewMonsterDefinitionId)
{
	const FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset ? LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId) : nullptr;
	if (!MonsterSpawn) return false;
	if (MonsterSpawn->MonsterDefinition && MonsterSpawn->MonsterDefinition->MonsterId == NewMonsterDefinitionId) return true;
	if (ObjectPalette)
	{
		for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
		{
			if (Entry.DefaultMonsterDefinition && Entry.DefaultMonsterDefinition->MonsterId == NewMonsterDefinitionId)
				return SetSelectedObjectMonsterDefinitionAsset(Entry.DefaultMonsterDefinition);
		}
	}
	return false;
}

bool AGridLevelEditorActor::SyncSelectedMonsterDefinitionIdFromAsset()
{
	const FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset ? LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId) : nullptr;
	return MonsterSpawn && MonsterSpawn->MonsterDefinition;
}

bool AGridLevelEditorActor::SetSelectedObjectEncounterGroupId(FName NewEncounterGroupId)
{
	FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset ? LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId) : nullptr;
	if (!MonsterSpawn) return false;
	LevelAsset->Modify();
	MonsterSpawn->EncounterGroupId = NewEncounterGroupId;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectEncounterWaveIndex(int32 NewEncounterWaveIndex)
{
	FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset ? LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId) : nullptr;
	if (!MonsterSpawn) return false;
	LevelAsset->Modify();
	MonsterSpawn->EncounterWaveIndex = FMath::Max(0, NewEncounterWaveIndex);
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitialMonsterState(EGridMonsterState NewInitialMonsterState)
{
	FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset ? LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId) : nullptr;
	if (!MonsterSpawn || (NewInitialMonsterState != EGridMonsterState::Idle && NewInitialMonsterState != EGridMonsterState::Dormant)) return false;
	if (MonsterSpawn->InitialMonsterState == NewInitialMonsterState) return true;
	LevelAsset->Modify();
	MonsterSpawn->InitialMonsterState = NewInitialMonsterState;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableContentAsset(UGridReadableContentAsset* NewReadableContentAsset)
{
	FGridLooseItemInstance* LooseItemInstance = LevelAsset ? LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId) : nullptr;
	if (!LooseItemInstance) return false;
	LevelAsset->Modify();
	LooseItemInstance->ReadableContentAsset = NewReadableContentAsset;
	if (NewReadableContentAsset && LooseItemInstance->ReadableContentId.IsNone()) LooseItemInstance->ReadableContentId = NewReadableContentAsset->ReadableContentId;
	ObjectBehavior.Item.DefaultReadableContentAsset = LooseItemInstance->ReadableContentAsset;
	ObjectBehavior.Item.DefaultReadableContentId = LooseItemInstance->ReadableContentId;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableContentId(FName NewReadableContentId)
{
	FGridLooseItemInstance* LooseItemInstance = LevelAsset ? LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId) : nullptr;
	if (!LooseItemInstance) return false;
	LevelAsset->Modify();
	LooseItemInstance->ReadableContentId = NewReadableContentId;
	ObjectBehavior.Item.DefaultReadableContentId = NewReadableContentId;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadTitleOverride(const FText& NewReadTitleOverride)
{
	FGridLooseItemInstance* LooseItemInstance = LevelAsset ? LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId) : nullptr;
	if (!LooseItemInstance) return false;
	LevelAsset->Modify();
	LooseItemInstance->ReadTitleOverride = NewReadTitleOverride;
	ObjectBehavior.Item.DefaultReadTitleOverride = NewReadTitleOverride;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadTextOverride(const FText& NewReadTextOverride)
{
	FGridLooseItemInstance* LooseItemInstance = LevelAsset ? LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId) : nullptr;
	if (!LooseItemInstance) return false;
	LevelAsset->Modify();
	LooseItemInstance->ReadTextOverride = NewReadTextOverride;
	ObjectBehavior.Item.DefaultReadTextOverride = NewReadTextOverride;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectTag(FName NewTag)
{
	(void)NewTag;
	return false;
}

bool AGridLevelEditorActor::SetSelectedObjectNotes(const FString& NewNotes)
{
	if (!EditGridPlacementAuthoring(LevelAsset, LastSelectedObjectId, [&NewNotes](auto& Placement) { Placement.Notes = NewNotes; })) return false;
	ObjectNotes = NewNotes;
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableText(const FText& NewReadableText)
{
	FGridWorldObjectInstance* WorldObjectInstance = LevelAsset ? LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId) : nullptr;
	if (!WorldObjectInstance) return false;
	Modify();
	LevelAsset->Modify();
	WorldObjectInstance->ReadableTextOverride = NewReadableText;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}
