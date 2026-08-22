    Result += FString::Printf (TEXT ("PreviewRuntimeActor: %s\n"), PreviewRuntimeActor ? *PreviewRuntimeActor->GetName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("Preview Runtime LevelAsset: %s\n"), PreviewLevelAsset ? *PreviewLevelAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("Preview Asset Stats: %s\n"), *GetLevelAssetStatsText (PreviewLevelAsset));
    Result += FString::Printf (TEXT ("Preview Start: %s\n"), *GetLevelStartText (PreviewLevelAsset));

    if (!LevelAsset)
    {
        Result += TEXT ("Status: ERROR - EditorActor LevelAsset is null.");
    }
    else if (!PreviewRuntimeActor)
    {
        Result += TEXT ("Status: ERROR - PreviewRuntimeActor is null.");
    }
    else if (!PreviewLevelAsset)
    {
        Result += TEXT ("Status: ERROR - PreviewRuntimeActor LevelAsset is null.");
    }
    else if (LevelAsset == PreviewLevelAsset)
    {
        Result += TEXT ("Status: OK - Editor and PreviewRuntimeActor use the same LevelAsset.");
    }
    else
    {
        Result += TEXT ("Status: WARNING - Editor and PreviewRuntimeActor use different LevelAssets.");
    }

    return Result;
}

void AGridLevelEditorActor::LogEditorRuntimeAssetConsistency () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetEditorRuntimeAssetConsistencyDiagnostics ());
}

FString AGridLevelEditorActor::GetItemWorkflowDiagnostics () const
{
    FString Result;
    Result += TEXT ("Grid ItemDefinition Workflow Diagnostics\n");
    Result += FString::Printf (TEXT ("EditorActor=%s\n"), *GetName ());
    Result += FString::Printf (TEXT ("DungeonAsset=%s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("LevelAsset=%s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));

    if (DungeonAsset && DungeonAsset->Levels.Num () > 0)
    {
        for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
        {
            if (!Entry.bEnabled)
            {
                continue;
            }
            const FString LevelLabel = Entry.LevelId.IsNone ()
                ? FString (TEXT ("None"))
                : Entry.LevelId.ToString ();
            AppendItemWorkflowDiagnosticsForLevel (Result, Entry.LevelAsset, LevelLabel);
        }
        return Result;
    }

    AppendItemWorkflowDiagnosticsForLevel (Result, LevelAsset, TEXT ("CurrentLevel"));
    return Result;
}

void AGridLevelEditorActor::LogItemWorkflowDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetItemWorkflowDiagnostics ());
}

FString AGridLevelEditorActor::GetDungeonDiagnostics () const
{
    FString Result;
    Result += TEXT ("GridLevelEditorActor Dungeon Diagnostics\n");
    Result += FString::Printf (TEXT ("EditorActor: %s\n"), *GetName ());
    Result += FString::Printf (TEXT ("DungeonAsset: %s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("CurrentDungeonLevelId: %s\n"), *CurrentDungeonLevelId.ToString ());
    Result += FString::Printf (TEXT ("Current LevelAsset: %s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));

    if (!DungeonAsset)
    {
        Result += TEXT ("Status: WARNING - DungeonAsset is null. Editor is using LevelAsset directly.");
        return Result;
    }

    Result += DungeonAsset->GetDungeonDiagnostics ();

    const FGridDungeonLevelEntry* CurrentEntry = DungeonAsset->FindLevelEntry (CurrentDungeonLevelId);
    if (!CurrentEntry)
    {
        Result += TEXT ("\nCurrentSelectionStatus: WARNING - CurrentDungeonLevelId was not found.");
    }
    else if (!CurrentEntry->bEnabled)
    {
        Result += TEXT ("\nCurrentSelectionStatus: WARNING - Current dungeon level is disabled.");
    }
    else if (!CurrentEntry->LevelAsset)
    {
        Result += TEXT ("\nCurrentSelectionStatus: ERROR - Current dungeon level has no LevelAsset.");
    }
    else
    {
        Result += TEXT ("\nCurrentSelectionStatus: OK");
    }

    return Result;
}

void AGridLevelEditorActor::LogDungeonDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetDungeonDiagnostics ());
}

void AGridLevelEditorActor::LogDungeonTransitionDiagnostics () const
{
    if (!DungeonAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("LogDungeonTransitionDiagnostics failed: DungeonAsset is null."));
        return;
    }

    UE_LOG (LogTemp, Log, TEXT ("%s"), *DungeonAsset->GetTransitionDiagnostics ());
}

bool AGridLevelEditorActor::CreateAndAddDungeonLevel (
    FName NewLevelId,
    FText DisplayName,
    FIntVector LogicalPosition,
    FString& OutError)
{
    OutError.Reset ();

    if (!DungeonAsset)
    {
        OutError = TEXT ("DungeonAsset is null.");
        return false;
    }

    if (NewLevelId.IsNone ())
    {
        OutError = TEXT ("Level Id is empty.");
        return false;
    }

    for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
    {
        if (Entry.LevelId == NewLevelId)
        {
            OutError = FString::Printf (TEXT ("Level Id '%s' already exists."), *NewLevelId.ToString ());
            return false;
        }

        if (Entry.LogicalPosition == LogicalPosition)
        {
            OutError = FString::Printf (
                TEXT ("Logical Position (%d,%d,%d) is already used by LevelId '%s'."),
                LogicalPosition.X,
                LogicalPosition.Y,
                LogicalPosition.Z,
                *Entry.LevelId.ToString ());
            return false;
        }
    }
