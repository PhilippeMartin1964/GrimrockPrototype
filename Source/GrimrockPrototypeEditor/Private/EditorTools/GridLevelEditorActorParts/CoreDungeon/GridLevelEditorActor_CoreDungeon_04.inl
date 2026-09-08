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

bool AGridLevelEditorActor::IsEdgePlacedObject(const FGuid& ObjectId) const
{
	if (!LevelAsset) return false;
	if (const FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(ObjectId))
	{
		return IsEdgePlacedObject(WorldObjectInstance->Type, WorldObjectInstance->WorldObjectDefinitionId);
	}
	if (const FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(ObjectId))
	{
		return LooseItemInstance->SurfaceSide != EGridEdge::None;
	}
	return false;
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
		case EGridLevelObjectType::Pit:
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

	int32 CellX, CellY;
	EGridEdge Edge;
	if (!LevelAsset->TryGetTypedPlacementLocation(LastSelectedObjectId, CellX, CellY, Edge))
	{
		return false;
	}

	const EGridLevelObjectType ObjectType = LevelAsset->GetTypedPlacementType(LastSelectedObjectId);
	const bool bUsesEdge = IsEdgePlacedObject(LastSelectedObjectId);
	if (bUsesEdge)
	{
		for (const FGuid& ObjectId : LevelAsset->GetTypedPlacementIdsAtCell(CellX, CellY))
		{
			int32 OtherCellX, OtherCellY;
			EGridEdge OtherEdge;
			if (ObjectId != LastSelectedObjectId && LevelAsset->GetTypedPlacementType(ObjectId) == ObjectType &&
				LevelAsset->TryGetTypedPlacementLocation(ObjectId, OtherCellX, OtherCellY, OtherEdge) && OtherEdge == Orientation)
			{
				UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: cannot orient selected object, destination edge is occupied."));
				return false;
			}
		}
	}

	if (FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		if (bUsesEdge) WorldObjectInstance->WallSide = Orientation;
		else
		{
			if (!WorldObjectInstance->bHasLocalTransformOverride) WorldObjectInstance->LocalTransformOverride = FTransform::Identity;
			FRotator Rotation = WorldObjectInstance->LocalTransformOverride.Rotator();
			Rotation.Yaw = GetYawForOrientation(Orientation);
			WorldObjectInstance->LocalTransformOverride.SetRotation(Rotation.Quaternion());
			WorldObjectInstance->bHasLocalTransformOverride = !WorldObjectInstance->LocalTransformOverride.Equals(FTransform::Identity);
		}
	}
	else if (FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		MonsterSpawn->Facing = Orientation;
	}
	else if (FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		if (bUsesEdge) LooseItemInstance->SurfaceSide = Orientation;
		else LooseItemInstance->LocalYaw = GetYawForOrientation(Orientation);
	}
	else return false;

	if (bUsesEdge)
	{
		SelectedEdge = Orientation;
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
