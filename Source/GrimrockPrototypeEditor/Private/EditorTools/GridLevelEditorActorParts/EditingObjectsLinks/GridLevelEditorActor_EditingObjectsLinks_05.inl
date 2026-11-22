#include "EditorTools/GridEditorItemLevelPolicy.h"

SelectedPaletteEntryId = Entry->EntryId;
PaintObjectType = Entry->DefaultWorldObjectDefinition->SupportedType;
WorldObjectDefinitionId = Entry->DefaultWorldObjectDefinition->DefinitionId;
SelectedWorldObjectDefinitionId = Entry->DefaultWorldObjectDefinition->DefinitionId;
// Ordinary placed objects always exist. This editor scratch flag is used only by spawn placements.
bObjectInitiallyEnabled = true;
bObjectInitiallyActive = false;
ObjectBehavior = Entry->DefaultWorldObjectDefinition->DefaultBehavior;

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

	const FGridObjectPaletteEntry* CanonicalEntry = ObjectPalette
		? ObjectPalette->FindEntryById(SelectedPaletteEntryId) : nullptr;
	const FGridLooseItemInstance* ExistingLoose = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId);
	const bool bReplaceLooseIdentity = ExistingLoose &&
		SelectedPaletteEntryId != ExistingLoose->PaletteEntryId;
	if (ExistingLoose)
	{
		if ((bReplaceLooseIdentity && !CanonicalEntry) ||
			(CanonicalEntry && (!CanonicalEntry->IsDirectItemEntry() || !CanonicalEntry->IsValidEntry())))
			return false;
		const FName TargetId = bReplaceLooseIdentity ? CanonicalEntry->ItemId : ExistingLoose->ItemId;
		const int32 TargetLevel = TargetId == ExistingLoose->ItemId
			? ExistingLoose->ItemLevel : CanonicalEntry->DefaultItemLevel;
		const UGridItemSubsystem* Registry = ResolveEditorItemRegistry();
		FGridResolvedItem Resolved;
		if (!FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry, TargetId, TargetLevel) ||
			!Registry->ResolveItem(TargetId, TargetLevel, Resolved) ||
			ExistingLoose->Quantity < 1 ||
			(TargetId != ExistingLoose->ItemId &&
			 ExistingLoose->Quantity > FMath::Max(1, Resolved.MaxStack)))
			return false;
	}
	if (!EditGridPlacementAuthoring(LevelAsset, LastSelectedObjectId, [this](auto& Placement)
		{
			Placement.PaletteEntryId = SelectedPaletteEntryId;
			Placement.Notes = ObjectNotes;
		})) return false;
	if (FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId))
	{
		WorldObjectInstance->Type = PaintObjectType;
		WorldObjectInstance->WorldObjectDefinitionId = WorldObjectDefinitionId;
		WorldObjectInstance->WallSide = IsEdgePlacedObject(PaintObjectType, WorldObjectDefinitionId) ? SelectedEdge : EGridEdge::None;
		WorldObjectInstance->InstanceConfig.Relocation = ObjectBehavior.Relocation;
		WorldObjectInstance->InstanceConfig.Pit = ObjectBehavior.Pit;
		WorldObjectInstance->InstanceConfig.ReceptacleInitialContent = ObjectBehavior.Receptacle.InitialContent;
		WorldObjectInstance->InstanceConfig.bStartsUnlocked = ObjectBehavior.Lock.bStartsUnlocked;
	}
	else if (FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId))
	{
		if (bReplaceLooseIdentity && LooseItemInstance->ItemId != CanonicalEntry->ItemId)
		{
			LooseItemInstance->ItemId = CanonicalEntry->ItemId;
			LooseItemInstance->ItemLevel = CanonicalEntry->DefaultItemLevel;
		}
		LooseItemInstance->ReadableContentAsset = ObjectBehavior.Item.DefaultReadableContentAsset;
		LooseItemInstance->ReadableContentId = ObjectBehavior.Item.DefaultReadableContentId;
		LooseItemInstance->ReadTitleOverride = ObjectBehavior.Item.DefaultReadTitleOverride;
		LooseItemInstance->ReadTextOverride = ObjectBehavior.Item.DefaultReadTextOverride;
	}
	else if (FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId))
	{
		MonsterSpawn->bSpawnAtStart = bObjectInitiallyEnabled;
	}
	else if (FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(LastSelectedObjectId))
	{
		// The selected-object ItemId command is the identity authority.
		// Applying metadata must not overwrite the generator's physical identity.
		ItemSpawn->bSpawnAtStart = bObjectInitiallyEnabled;
	}
	else if (FGridLogicObjectInstance* LogicInstance = LevelAsset->FindLogicObjectInstanceById(LastSelectedObjectId))
	{
		LogicInstance->Type = PaintObjectType;
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
	WorldObjectDefinitionId = NAME_None;
	SelectedWorldObjectDefinitionId = NAME_None;
	SelectedPaletteEntryId = NAME_None;

	bObjectInitiallyEnabled = true;
	bObjectInitiallyActive = false;

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
