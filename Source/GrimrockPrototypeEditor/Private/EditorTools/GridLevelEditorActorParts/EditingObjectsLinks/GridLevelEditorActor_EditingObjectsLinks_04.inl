bool AGridLevelEditorActor::TryGetSelectedObjectWorldLocation(FVector& OutWorldLocation) const
{
	return LastSelectedObjectId.IsValid() && TryGetObjectWorldLocationById(LastSelectedObjectId, OutWorldLocation);
}

bool AGridLevelEditorActor::TryGetPendingLinkSourceLocation(FVector& OutWorldLocation) const
{
	if (!bHasPendingLinkSource || !PendingLinkSourceObjectId.IsValid())
	{
		return false;
	}

	return TryGetObjectWorldLocationById(PendingLinkSourceObjectId, OutWorldLocation);
}

bool AGridLevelEditorActor::HasPendingLinkSource() const
{
	return bHasPendingLinkSource && PendingLinkSourceObjectId.IsValid();
}
void AGridLevelEditorActor::ClearPendingLinkSource()
{
	bHasPendingLinkSource = false;
	PendingLinkSourceObjectId.Invalidate();
}

bool AGridLevelEditorActor::BeginOrCompleteLinkAtSelection()
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		return false;
	}

	const FGuid SelectedObjectId = FindObjectIdAtSelection();
	if (!SelectedObjectId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: no object at selection for link mode."));
		return false;
	}

	if (!bHasPendingLinkSource)
	{
		PendingLinkSourceObjectId = SelectedObjectId;
		bHasPendingLinkSource = true;
		LastSelectedObjectId = SelectedObjectId;

		UE_LOG(LogTemp, Log, TEXT("GridLevelEditorActor: link source set to %s"), *SelectedObjectId.ToString());

		return true;
	}

	if (PendingLinkSourceObjectId == SelectedObjectId)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: source and target are identical."));
		return false;
	}

	FGridObjectLink NewLink;
	NewLink.SourceObjectId = PendingLinkSourceObjectId;
	NewLink.TargetObjectId = SelectedObjectId;
	NewLink.SourceEvent = LinkSourceEvent;
	NewLink.Command = LinkCommand;
	NewLink.Condition = EGridObjectCondition::None;

	const bool bAlreadyExists = GridEditorLinkService::ContainsExactLink(LevelAsset->Links, NewLink);
	if (!bAlreadyExists && !GridEditorLinkService::CreateLink(*this, NewLink))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: link creation rejected by the connector policy."));
		return false;
	}

	if (!bAlreadyExists)
	{
		UE_LOG(LogTemp, Log, TEXT("GridLevelEditorActor: link created %s -> %s"), *PendingLinkSourceObjectId.ToString(), *SelectedObjectId.ToString());
	}

	LastSelectedObjectId = SelectedObjectId;
	ClearPendingLinkSource();
	if (bAlreadyExists)
	{
		RebuildPreview();
	}
	return true;
}

bool AGridLevelEditorActor::RemoveLinksAtSelection()
{
	if (!HasValidLevelAsset())
	{
		return false;
	}

	const FGuid SelectedObjectId = FindObjectIdAtSelection();
	if (!SelectedObjectId.IsValid())
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	const int32 RemovedCount = LevelAsset->Links.RemoveAll(
		[&](const FGridObjectLink& Link)
		{
			return Link.SourceObjectId == SelectedObjectId || Link.TargetObjectId == SelectedObjectId;
		});

	if (RemovedCount > 0)
	{
#if WITH_EDITOR
		LevelAsset->MarkPackageDirty();
#endif
		RebuildPreview();
		return true;
	}
	return false;
}

bool AGridLevelEditorActor::ApplyPaletteEntry(FName EntryId)
{
	if (!ObjectPalette)
	{
		return false;
	}

	const FGridObjectPaletteEntry* Entry = ObjectPalette->FindEntryById(EntryId);
	if (!Entry)
	{
		return false;
	}

	// WORLDOBJ-MIG05: collectibles are palette-addressable directly through their
	// ItemDefinition. No companion UGridObjectArchetypeAsset is required.
	if (Entry->DefaultItemDefinition)
	{
		SelectedPaletteEntryId = Entry->EntryId;
		PaintObjectType = EGridLevelObjectType::Item;
		ObjectArchetypeId = NAME_None;
		SelectedArchetypeId = NAME_None;
		bObjectInitiallyEnabled = true;
		bObjectInitiallyActive = false;
		ObjectTag = NAME_None;
		ObjectBehavior = FGridObjectBehaviorParams();
		ObjectBehavior.Item.ItemDefinitionAsset = Entry->DefaultItemDefinition;
		return true;
	}

	if (!Entry->DefaultArchetype)
	{
		return false;
	}
