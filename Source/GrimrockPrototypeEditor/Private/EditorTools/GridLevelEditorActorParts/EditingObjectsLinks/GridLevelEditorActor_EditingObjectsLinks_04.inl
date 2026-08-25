{
	if (Obj.ObjectId == ObjectId)
	{
		return &Obj;
	}
}
return nullptr;
}

FGridLevelObjectData* AGridLevelEditorActor::FindSelectedObjectMutable()
{
	if (!HasValidLevelAsset() || !LastSelectedObjectId.IsValid())
	{
		return nullptr;
	}

	return LevelAsset->Objects.FindByPredicate(
		[this](const FGridLevelObjectData& Obj)
		{
			return Obj.ObjectId == LastSelectedObjectId;
		});
}

bool AGridLevelEditorActor::TryGetObjectWorldLocation(const FGridLevelObjectData& ObjectData, FVector& OutWorldLocation) const
{
	if (!HasValidLevelAsset())
	{
		return false;
	}
	const float CellSize = LevelAsset->CellSize;
	FVector GridWorldOrigin = FVector::ZeroVector;
	if (PreviewRuntimeActor)
	{
		GridWorldOrigin = PreviewRuntimeActor->GetActorLocation() + PreviewRuntimeActor->GridOrigin;
	}

	const FVector CellCenter =
		GridWorldOrigin + FVector((ObjectData.CellX * CellSize) + (CellSize * 0.5f), (ObjectData.CellY * CellSize) + (CellSize * 0.5f), 12.f);

	if (IsEdgePlacedObject(ObjectData))
	{
		switch (ObjectData.Edge)
		{
			case EGridEdge::North:
				OutWorldLocation = CellCenter + FVector(0.f, CellSize * 0.5f, 0.f);
				return true;

			case EGridEdge::East:
				OutWorldLocation = CellCenter + FVector(CellSize * 0.5f, 0.f, 0.f);
				return true;

			case EGridEdge::South:
				OutWorldLocation = CellCenter + FVector(0.f, -CellSize * 0.5f, 0.f);
				return true;

			case EGridEdge::West:
				OutWorldLocation = CellCenter + FVector(-CellSize * 0.5f, 0.f, 0.f);
				return true;

			default:
				return false;
		}
	}

	OutWorldLocation = CellCenter;
	return true;
}

bool AGridLevelEditorActor::TryGetSelectedObjectWorldLocation(FVector& OutWorldLocation) const
{
	const FGridLevelObjectData* Obj = FindObjectAtSelection();
	return Obj ? TryGetObjectWorldLocation(*Obj, OutWorldLocation) : false;
}

bool AGridLevelEditorActor::TryGetPendingLinkSourceLocation(FVector& OutWorldLocation) const
{
	if (!bHasPendingLinkSource || !PendingLinkSourceObjectId.IsValid())
	{
		return false;
	}

	const FGridLevelObjectData* Obj = FindObjectById(PendingLinkSourceObjectId);
	return Obj ? TryGetObjectWorldLocation(*Obj, OutWorldLocation) : false;
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

	const FGridLevelObjectData* SelectedObject = FindObjectAtSelection();
	if (!SelectedObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: no object at selection for link mode."));
		return false;
	}

	if (!bHasPendingLinkSource)
	{
		PendingLinkSourceObjectId = SelectedObject->ObjectId;
		bHasPendingLinkSource = true;
		LastSelectedObjectId = SelectedObject->ObjectId;

		UE_LOG(LogTemp, Log, TEXT("GridLevelEditorActor: link source set to %s"), *SelectedObject->ObjectId.ToString());

		return true;
	}

	if (PendingLinkSourceObjectId == SelectedObject->ObjectId)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: source and target are identical."));
		return false;
	}

	FGridObjectLink NewLink;
	NewLink.SourceObjectId = PendingLinkSourceObjectId;
	NewLink.TargetObjectId = SelectedObject->ObjectId;
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
		UE_LOG(LogTemp, Log, TEXT("GridLevelEditorActor: link created %s -> %s"), *PendingLinkSourceObjectId.ToString(), *SelectedObject->ObjectId.ToString());
	}

	LastSelectedObjectId = SelectedObject->ObjectId;
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

	const FGridLevelObjectData* SelectedObject = FindObjectAtSelection();
	if (!SelectedObject)
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	const int32 RemovedCount = LevelAsset->Links.RemoveAll(
		[&](const FGridObjectLink& Link)
		{
			return Link.SourceObjectId == SelectedObject->ObjectId || Link.TargetObjectId == SelectedObject->ObjectId;
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
	if (!Entry || !Entry->DefaultArchetype)
	{
		return false;
	}
