#if WITH_EDITOR
const FString LevelAssetFolderPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels");
const FString SanitizedLevelId = SanitizeAssetNameToken(NewLevelId.ToString());
const FString BaseAssetName = FString::Printf(TEXT("DA_GridLevel_%s"), *SanitizedLevelId);

FString AssetName;
const FString PackageName = MakeUniqueGridLevelPackageName(LevelAssetFolderPath, BaseAssetName, AssetName);

UPackage* Package = CreatePackage(*PackageName);
if (!Package)
{
	OutError = FString::Printf(TEXT("Failed to create package '%s'."), *PackageName);
	return false;
}

UGridLevelAsset* NewLevelAsset = NewObject<UGridLevelAsset>(Package, UGridLevelAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);

if (!NewLevelAsset)
{
	OutError = FString::Printf(TEXT("Failed to create UGridLevelAsset '%s'."), *AssetName);
	return false;
}

NewLevelAsset->Modify();
NewLevelAsset->Width = 32;
NewLevelAsset->Height = 32;
NewLevelAsset->CellSize = 200.f;
NewLevelAsset->EnsureCellCount();
NewLevelAsset->Objects.Reset();
NewLevelAsset->Links.Reset();
NewLevelAsset->StartCellX = 1;
NewLevelAsset->StartCellY = 1;
NewLevelAsset->StartFacing = EGridEdge::North;

if (NewLevelAsset->IsValidCoord(NewLevelAsset->StartCellX, NewLevelAsset->StartCellY))
{
	FGridLevelCellData& StartCell = NewLevelAsset->GetCellMutable(NewLevelAsset->StartCellX, NewLevelAsset->StartCellY);
	StartCell.CellType = EGridCellType::Floor;
	StartCell.bHasCeiling = true;
	StartCell.bBlocksOccupancy = false;
}

DungeonAsset->Modify();
const FName PreviousDefaultLevelId = DungeonAsset->DefaultLevelId;
const FName PreviousCurrentDungeonLevelId = CurrentDungeonLevelId;
UGridLevelAsset* PreviousLevelAsset = LevelAsset;

FGridDungeonLevelEntry NewEntry;
NewEntry.LevelId = NewLevelId;
NewEntry.DisplayName = DisplayName.IsEmpty() ? FText::FromName(NewLevelId) : DisplayName;
NewEntry.LevelAsset = NewLevelAsset;
NewEntry.LogicalPosition = LogicalPosition;
NewEntry.bEnabled = true;
DungeonAsset->Levels.Add(NewEntry);

if (DungeonAsset->DefaultLevelId.IsNone())
{
	DungeonAsset->DefaultLevelId = NewLevelId;
}

Modify();
CurrentDungeonLevelId = NewLevelId;
LevelAsset = NewLevelAsset;
if (!ApplyCurrentDungeonLevel())
{
	DungeonAsset->Levels.RemoveAll(
		[NewLevelId](const FGridDungeonLevelEntry& Entry)
		{
			return Entry.LevelId == NewLevelId;
		});
	DungeonAsset->DefaultLevelId = PreviousDefaultLevelId;
	CurrentDungeonLevelId = PreviousCurrentDungeonLevelId;
	LevelAsset = PreviousLevelAsset;
	NewLevelAsset->ClearFlags(RF_Public | RF_Standalone);

	OutError = FString::Printf(TEXT("Level '%s' was created but could not be applied to the editor actor."), *NewLevelId.ToString());
	UE_LOG(LogTemp, Error, TEXT("%s"), *OutError);
	return false;
}

FAssetRegistryModule::AssetCreated(NewLevelAsset);
Package->MarkPackageDirty();
DungeonAsset->MarkPackageDirty();

TArray<UPackage*> PackagesToSave;
PackagesToSave.Add(Package);
PackagesToSave.Add(DungeonAsset->GetOutermost());
UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

UE_LOG(LogTemp, Log, TEXT("Created dungeon level %s at LogicalPosition=(%d,%d,%d), Asset=%s."), *NewLevelId.ToString(), LogicalPosition.X, LogicalPosition.Y,
	LogicalPosition.Z, *NewLevelAsset->GetPathName());

return true;
#else
OutError = TEXT("CreateAndAddDungeonLevel is editor-only.");
return false;
#endif
}
