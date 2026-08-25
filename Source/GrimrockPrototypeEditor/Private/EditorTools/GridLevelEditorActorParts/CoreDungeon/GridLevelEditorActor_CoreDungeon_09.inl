CurrentDungeonLevelId = DungeonAsset->DefaultLevelId;
UE_LOG(LogTemp, Log, TEXT("LoadDefaultDungeonLevel: loading DefaultLevelId %s."), *CurrentDungeonLevelId.ToString());
ApplyCurrentDungeonLevel();
return;
}

for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
{
	if (Entry.bEnabled && !Entry.LevelId.IsNone() && Entry.LevelAsset)
	{
#if WITH_EDITOR
		Modify();
#endif
		CurrentDungeonLevelId = Entry.LevelId;
		UE_LOG(LogTemp, Warning, TEXT("LoadDefaultDungeonLevel: DefaultLevelId %s is not valid; loading first enabled level %s."),
			*DungeonAsset->DefaultLevelId.ToString(), *CurrentDungeonLevelId.ToString());
		ApplyCurrentDungeonLevel();
		return;
	}
}

UE_LOG(LogTemp, Error, TEXT("LoadDefaultDungeonLevel failed: DungeonAsset %s has no enabled level with a LevelAsset."), *DungeonAsset->GetPathName());
}

void AGridLevelEditorActor::SyncPreviewRuntimeLevelAsset()
{
	ResolvePreviewRuntimeActor();

	if (!LevelAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("GridLevelEditorActor: cannot sync PreviewRuntimeActor because LevelAsset is null."));
		return;
	}

	if (!PreviewRuntimeActor)
	{
		UE_LOG(LogTemp, Error, TEXT("GridLevelEditorActor: cannot sync LevelAsset because PreviewRuntimeActor is null."));
		return;
	}

#if WITH_EDITOR
	PreviewRuntimeActor->Modify();
#endif
	PreviewRuntimeActor->LevelAsset = LevelAsset;
	PreviewRuntimeActor->DungeonAsset = DungeonAsset;
	PreviewRuntimeActor->CurrentDungeonLevelId = CurrentDungeonLevelId;
	SyncPreviewRuntimeObjectArchetypesFromPalette();
	PreviewRuntimeActor->RebuildLevel();

	LogEditorRuntimeAssetConsistency();
}

void AGridLevelEditorActor::PreparePIETestFromStart()
{
	FString Error;
	if (!PreparePIETestFromStartInternal(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("PreparePIETestFromStart failed: %s"), *Error);
	}
}

bool AGridLevelEditorActor::PreparePIETestFromStartInternal(FString& OutError)
{
	OutError.Reset();

	if (!LevelAsset)
	{
		OutError = TEXT("LevelAsset is null.");
		return false;
	}

	if (!LevelAsset->IsStartCellValid())
	{
		OutError = FString::Printf(
			TEXT("Start cell is invalid: X=%d Y=%d Facing=%s."), LevelAsset->StartCellX, LevelAsset->StartCellY, *GetGridEdgeText(LevelAsset->StartFacing));
		return false;
	}

	if (DungeonAsset)
	{
		if (CurrentDungeonLevelId.IsNone())
		{
			OutError = TEXT("CurrentDungeonLevelId is None while DungeonAsset is assigned.");
			return false;
		}

		if (!DungeonAsset->IsValidLevelId(CurrentDungeonLevelId))
		{
			OutError = FString::Printf(TEXT("CurrentDungeonLevelId '%s' is not an enabled level with a LevelAsset in DungeonAsset %s."),
				*CurrentDungeonLevelId.ToString(), *DungeonAsset->GetPathName());
			return false;
		}

		UGridLevelAsset* DungeonLevelAsset = DungeonAsset->GetLevelAssetById(CurrentDungeonLevelId);
		if (DungeonLevelAsset != LevelAsset)
		{
			OutError =
				FString::Printf(TEXT("CurrentDungeonLevelId '%s' resolves to %s but EditorActor.LevelAsset is %s. Apply Current Dungeon Level before PIE."),
					*CurrentDungeonLevelId.ToString(), DungeonLevelAsset ? *DungeonLevelAsset->GetPathName() : TEXT("None"), *LevelAsset->GetPathName());
			return false;
		}
	}

	ResolvePreviewRuntimeActor();

	if (!PreviewRuntimeActor)
	{
		OutError = TEXT("PreviewRuntimeActor is missing.");
		return false;
	}

#if WITH_EDITOR
	PreviewRuntimeActor->Modify();
#endif
	PreviewRuntimeActor->bApplyLevelStartOnBeginPlay = true;
	PreviewRuntimeActor->DungeonRuntimeState = FGridDungeonRuntimeState();
	PreviewRuntimeActor->LevelAsset = LevelAsset;
	PreviewRuntimeActor->DungeonAsset = DungeonAsset;
	PreviewRuntimeActor->CurrentDungeonLevelId = CurrentDungeonLevelId;
	PreviewRuntimeActor->RebuildLevel();
	PreviewRuntimeActor->LogPIEReadinessDiagnostics();

	UE_LOG(LogTemp, Log, TEXT("PreparePIETestFromStart OK: %s is ready to test LevelAsset %s from StartCell X=%d Y=%d Facing=%s."),
		*GetNameSafe(PreviewRuntimeActor), *GetNameSafe(LevelAsset), LevelAsset->StartCellX, LevelAsset->StartCellY, *GetGridEdgeText(LevelAsset->StartFacing));

	return true;
}

void AGridLevelEditorActor::SetStartFromSelection()
{
	if (!LevelAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("GridLevelEditorActor: cannot set start from selection because LevelAsset is null."));
		return;
	}

	if (!LevelAsset->IsValidCoord(SelectedCellX, SelectedCellY))
	{
		UE_LOG(LogTemp, Error, TEXT("GridLevelEditorActor: cannot set start from invalid selection X=%d Y=%d."), SelectedCellX, SelectedCellY);
		return;
	}

	const FGridLevelCellData& SelectedCell = LevelAsset->GetCell(SelectedCellX, SelectedCellY);
	if (SelectedCell.CellType == EGridCellType::Empty || SelectedCell.bBlocksOccupancy)
	{
		UE_LOG(LogTemp, Error, TEXT("GridLevelEditorActor: cannot set start from non-walkable selection X=%d Y=%d."), SelectedCellX, SelectedCellY);
		return;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	LevelAsset->StartCellX = SelectedCellX;
	LevelAsset->StartCellY = SelectedCellY;
	LevelAsset->StartFacing = SelectedEdge == EGridEdge::None ? EGridEdge::North : SelectedEdge;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	UE_LOG(LogTemp, Log, TEXT("GridLevelEditorActor: level start set to X=%d Y=%d Facing=%s."), LevelAsset->StartCellX, LevelAsset->StartCellY,
		*GetGridEdgeText(LevelAsset->StartFacing));

	LogEditorRuntimeAssetConsistency();
}
