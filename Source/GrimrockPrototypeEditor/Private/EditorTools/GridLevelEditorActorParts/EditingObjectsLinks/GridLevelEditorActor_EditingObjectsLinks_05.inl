SelectedPaletteEntryId = Entry->EntryId;
PaintObjectType = Entry->DefaultArchetype->SupportedType;
ObjectArchetypeId = Entry->DefaultArchetype->ArchetypeId;
SelectedArchetypeId = Entry->DefaultArchetype->ArchetypeId;
bObjectInitiallyEnabled = Entry->DefaultArchetype->bDefaultInitiallyEnabled;
bObjectInitiallyActive = Entry->DefaultArchetype->bDefaultInitiallyActive;
ObjectTag = Entry->DefaultArchetype->DefaultTag;
ObjectBehavior = Entry->DefaultArchetype->DefaultBehavior;

return true;
}

void AGridLevelEditorActor::ApplySelectedPaletteEntry()
{
	ApplyPaletteEntry(SelectedPaletteEntryId);
}

bool AGridLevelEditorActor::ApplyEditedSelectedObject()
{
	if (!HasValidLevelAsset() || !LastSelectedObjectId.IsValid())
	{
		return false;
	}

	const TArray<FGridLevelObjectData> CompatibilityObjects = LevelAsset->BuildCompatibilityObjectProjectionFromTyped();
	const FGridLevelObjectData* StoredObject = CompatibilityObjects.FindByPredicate(
		[this](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == LastSelectedObjectId;
		});
	if (!StoredObject)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *StoredObject;
	EditedObject.Type = PaintObjectType;
	EditedObject.Edge = IsEdgePlacedObject(PaintObjectType, ObjectArchetypeId) ? SelectedEdge : EGridEdge::None;
	EditedObject.ArchetypeId = ObjectArchetypeId;
	EditedObject.PaletteEntryId = SelectedPaletteEntryId;
	EditedObject.bInitiallyEnabled = bObjectInitiallyEnabled;
	EditedObject.bInitiallyActive = bObjectInitiallyActive;
	EditedObject.Tag = ObjectTag;
	EditedObject.Notes = ObjectNotes;
	EditedObject.Behavior = ObjectBehavior;

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

bool AGridLevelEditorActor::RemoveLinkByIndexForSelectedObject(int32 LinkIndex)
{
	if (!HasValidLevelAsset() || !LastSelectedObjectId.IsValid())
	{
		return false;
	}

	int32 CurrentIndex = 0;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	for (int32 Index = 0; Index < LevelAsset->Links.Num(); ++Index)
	{
		const FGridObjectLink& Link = LevelAsset->Links[Index];

		if (Link.SourceObjectId != LastSelectedObjectId && Link.TargetObjectId != LastSelectedObjectId)
		{
			continue;
		}

		if (CurrentIndex == LinkIndex)
		{
			LevelAsset->Links.RemoveAt(Index);

#if WITH_EDITOR
			LevelAsset->MarkPackageDirty();
#endif

			RebuildPreview();
			return true;
		}

		++CurrentIndex;
	}
	return false;
}

bool AGridLevelEditorActor::RemoveAllLinksForSelectedObject()
{
	if (!HasValidLevelAsset() || !LastSelectedObjectId.IsValid())
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	const int32 RemovedCount = LevelAsset->Links.RemoveAll(
		[this](const FGridObjectLink& Link)
		{
			return Link.SourceObjectId == LastSelectedObjectId || Link.TargetObjectId == LastSelectedObjectId;
		});

	if (RemovedCount <= 0)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::CreateLink(FGuid SourceObjectId, FGuid TargetObjectId, EGridObjectEvent SourceEvent, EGridObjectCommand Command)
{
	FGridObjectLink Link;
	Link.SourceObjectId = SourceObjectId;
	Link.TargetObjectId = TargetObjectId;
	Link.SourceEvent = SourceEvent;
	Link.Command = Command;
	Link.Condition = EGridObjectCondition::None;

	return GridEditorLinkService::CreateLink(*this, Link);
}

void AGridLevelEditorActor::ClearSelectedObjectState()
{
	LastSelectedObjectId.Invalidate();

	PaintObjectType = EGridLevelObjectType::None;
	ObjectArchetypeId = NAME_None;
	SelectedArchetypeId = NAME_None;
	SelectedPaletteEntryId = NAME_None;

	bObjectInitiallyEnabled = true;
	bObjectInitiallyActive = false;

	ObjectTag = NAME_None;
	ObjectNotes.Empty();
	ObjectBehavior = FGridObjectBehaviorParams();
	ResolvePreviewRuntimeActor();

	if (PreviewRuntimeActor)
	{
		PreviewRuntimeActor->SetEditorSelectedObject(FGuid());
	}
}

bool AGridLevelEditorActor::RemoveExactLink(FGuid SourceObjectId, FGuid TargetObjectId, EGridObjectEvent SourceEvent, EGridObjectCommand Command)
{
	FGridObjectLink Link;
	Link.SourceObjectId = SourceObjectId;
	Link.TargetObjectId = TargetObjectId;
	Link.SourceEvent = SourceEvent;
	Link.Command = Command;
	Link.Condition = EGridObjectCondition::None;

	return GridEditorLinkService::RemoveExactLink(*this, Link);
}
