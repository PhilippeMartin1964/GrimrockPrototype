}
}

if (CoordinateGridPlane)
{
	CoordinateGridPlane->SetHiddenInGame(true, true);
	CoordinateGridPlane->SetVisibility(false, true);
}
}

#if WITH_EDITOR
void AGridLevelEditorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AGridLevelEditorActor, CoordinateGridPlaneMesh) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AGridLevelEditorActor, CoordinateGridMaterial) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AGridLevelEditorActor, bShowCoordinateGrid) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AGridLevelEditorActor, CoordinateGridZOffset))
	{
		UpdateCoordinateGridPlane();
		return;
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AGridLevelEditorActor, bShowCoordinateLabels) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AGridLevelEditorActor, CoordinateLabelWorldSize))
	{
		UpdateCoordinateHoverLabel();
		return;
	}
}
#endif

bool AGridLevelEditorActor::HasValidLevelAsset() const
{
	return LevelAsset != nullptr;
}

bool AGridLevelEditorActor::IsValidSelectedCell() const
{
	return LevelAsset && LevelAsset->IsValidCoord(SelectedCellX, SelectedCellY);
}

bool AGridLevelEditorActor::RequiresEdge(EGridLevelObjectType ObjectType) const
{
	switch (ObjectType)
	{
		case EGridLevelObjectType::Door:
		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Lever:
		case EGridLevelObjectType::Receptacle:
			return true;
		default:
			return false;
	}
}

bool AGridLevelEditorActor::IsEdgePlacedObject(const FGridLevelObjectData& ObjectData) const
{
	return IsEdgePlacedObject(ObjectData.Type, ObjectData.ArchetypeId);
}

bool AGridLevelEditorActor::IsEdgePlacedObject(EGridLevelObjectType ObjectType, FName ArchetypeId) const
{
	if (ObjectType == EGridLevelObjectType::Item && ArchetypeId == FName(TEXT("Item_Torch")))
	{
		return true;
	}

	if (const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById(ArchetypeId))
	{
		return Archetype->IsEdgePlaced() || Archetype->IsWallPlaced();
	}

	return RequiresEdge(ObjectType);
}

bool AGridLevelEditorActor::IsCellCenteredObject(EGridLevelObjectType ObjectType) const
{
	switch (ObjectType)
	{
		case EGridLevelObjectType::PressurePlate:
		case EGridLevelObjectType::MonsterSpawn:
		case EGridLevelObjectType::ItemSpawn:
		case EGridLevelObjectType::Item:
		case EGridLevelObjectType::Light:
		case EGridLevelObjectType::Teleporter:
		case EGridLevelObjectType::Trigger:
		case EGridLevelObjectType::Decoration:
			return true;
		default:
			return false;
	}
}

const UGridObjectArchetypeAsset* AGridLevelEditorActor::FindObjectArchetypeById(FName ArchetypeId) const
{
	if (ArchetypeId.IsNone() || !ObjectPalette)
	{
		return nullptr;
	}

	for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
	{
		if (Entry.DefaultArchetype && Entry.DefaultArchetype->ArchetypeId == ArchetypeId)
		{
			return Entry.DefaultArchetype;
		}
	}

	return nullptr;
}

bool AGridLevelEditorActor::SetSelectedObjectOrientation(EGridEdge Orientation)
{
	if (Orientation == EGridEdge::None || !LevelAsset || !LastSelectedObjectId.IsValid())
	{
		return false;
	}

	FGridLevelObjectData* SelectedObject = FindSelectedObjectMutable();
	if (!SelectedObject)
	{
		return false;
	}

	const bool bUsesEdge = IsEdgePlacedObject(*SelectedObject);
	if (bUsesEdge)
	{
		const FGuid SelectedObjectId = SelectedObject->ObjectId;
		const EGridLevelObjectType SelectedObjectType = SelectedObject->Type;
		const bool bDestinationOccupied = LevelAsset->Objects.ContainsByPredicate(
			[SelectedObjectId, SelectedObjectType, SelectedObject, Orientation](const FGridLevelObjectData& Obj)
			{
				return Obj.ObjectId != SelectedObjectId && Obj.CellX == SelectedObject->CellX && Obj.CellY == SelectedObject->CellY &&
					Obj.Type == SelectedObjectType && Obj.Edge == Orientation;
			});

		if (bDestinationOccupied)
		{
			UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: cannot orient selected object, destination edge is occupied."));
			return false;
		}
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	if (bUsesEdge)
	{
		SelectedObject->Edge = Orientation;
		SelectedEdge = Orientation;
	}
	else if (SelectedObject->Type == EGridLevelObjectType::MonsterSpawn)
	{
		SelectedObject->InitialFacing = Orientation;
		SelectedObject->LocalYaw = GetYawForOrientation(Orientation);
	}
	else
	{
		SelectedObject->LocalYaw = GetYawForOrientation(Orientation);
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

FGridLevelCellData* AGridLevelEditorActor::GetSelectedCellMutable()
{
	if (!IsValidSelectedCell())
	{
		return nullptr;
	}

	return &LevelAsset->GetCellMutable(SelectedCellX, SelectedCellY);
}

EGridWallType* AGridLevelEditorActor::GetSelectedWallMutable(FGridLevelCellData& CellData)
{
	switch (SelectedEdge)
	{
		case EGridEdge::North:
			return &CellData.NorthWall;
		case EGridEdge::East:
			return &CellData.EastWall;
		case EGridEdge::South:
			return &CellData.SouthWall;
		case EGridEdge::West:
			return &CellData.WestWall;
		default:
			return nullptr;
	}
}

void AGridLevelEditorActor::ResolvePreviewRuntimeActor()
{
	if (!PreviewRuntimeActor)
	{
		PreviewRuntimeActor = Cast<AGridLevelRuntimeActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
	}
}

FString AGridLevelEditorActor::GetEditorRuntimeAssetConsistencyDiagnostics() const
{
	const UWorld* World = GetWorld();
	const UGridLevelAsset* PreviewLevelAsset = PreviewRuntimeActor ? PreviewRuntimeActor->LevelAsset.Get() : nullptr;

	FString Result;
	Result += TEXT("GridLevelEditorActor Asset Consistency\n");
	Result += FString::Printf(TEXT("EditorActor: %s\n"), *GetName());
	Result += FString::Printf(TEXT("World: %s\n"), World ? *World->GetMapName() : TEXT("None"));
	Result += FString::Printf(TEXT("DungeonAsset: %s\n"), DungeonAsset ? *DungeonAsset->GetPathName() : TEXT("None"));
	Result += FString::Printf(TEXT("CurrentDungeonLevelId: %s\n"), *CurrentDungeonLevelId.ToString());
	Result += FString::Printf(TEXT("Editor LevelAsset: %s\n"), LevelAsset ? *LevelAsset->GetPathName() : TEXT("None"));
	Result += FString::Printf(TEXT("Editor Asset Stats: %s\n"), *GetLevelAssetStatsText(LevelAsset));
	Result += FString::Printf(TEXT("Editor Start: %s\n"), *GetLevelStartText(LevelAsset));
