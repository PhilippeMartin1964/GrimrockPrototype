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

	if (!LevelAsset->ContainsTypedPlacementId(LastSelectedObjectId) ||
		GridLevelPlacement::GetBucket(LevelAsset->GetTypedPlacementType(LastSelectedObjectId)) != GridLevelPlacement::GetBucket(PaintObjectType))
	{
		return false;
	}

	if (!EditGridPlacementAuthoring(LevelAsset, LastSelectedObjectId, [this](auto& Placement)
		{
			Placement.PaletteEntryId = SelectedPaletteEntryId;
			Placement.bInitiallyEnabled = bObjectInitiallyEnabled;
			Placement.Tag = ObjectTag;
			Placement.Notes = ObjectNotes;
		})) return false;
	if (FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId))
	{
		WorldObjectInstance->Type = PaintObjectType;
		WorldObjectInstance->WorldObjectDefinitionId = ObjectArchetypeId;
		WorldObjectInstance->WallSide = IsEdgePlacedObject(PaintObjectType, ObjectArchetypeId) ? SelectedEdge : EGridEdge::None;
		WorldObjectInstance->bInitiallyActive = bObjectInitiallyActive;
		WorldObjectInstance->InstanceConfig.Teleporter = ObjectBehavior.Teleporter;
		WorldObjectInstance->InstanceConfig.Transition = ObjectBehavior.Transition;
		WorldObjectInstance->InstanceConfig.Pit = ObjectBehavior.Pit;
		WorldObjectInstance->InstanceConfig.ReceptacleInitialContent = ObjectBehavior.Receptacle.InitialContent;
		WorldObjectInstance->InstanceConfig.bStartsUnlocked = ObjectBehavior.Lock.bStartsUnlocked;
	}
	else if (FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId))
	{
		LooseItemInstance->ItemDefinition = ObjectBehavior.Item.ItemDefinitionAsset;
		LooseItemInstance->ReadableContentAsset = ObjectBehavior.Item.DefaultReadableContentAsset;
		LooseItemInstance->ReadableContentId = ObjectBehavior.Item.DefaultReadableContentId;
		LooseItemInstance->ReadTitleOverride = ObjectBehavior.Item.DefaultReadTitleOverride;
		LooseItemInstance->ReadTextOverride = ObjectBehavior.Item.DefaultReadTextOverride;
	}
	else if (FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(LastSelectedObjectId))
	{
		ItemSpawn->ItemDefinition = ObjectBehavior.Item.ItemDefinitionAsset;
	}
	else if (FGridLogicObjectInstance* LogicInstance = LevelAsset->FindLogicObjectInstanceById(LastSelectedObjectId))
	{
		LogicInstance->Type = PaintObjectType;
		LogicInstance->bInitiallyActive = bObjectInitiallyActive;
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
