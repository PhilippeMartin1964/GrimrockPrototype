
bool AGridLevelEditorActor::ApplyCurrentDungeonLevel ()
{
    if (!DungeonAsset)
    {
        UE_LOG (LogTemp, Warning, TEXT ("ApplyCurrentDungeonLevel failed: DungeonAsset is null."));
        return false;
    }

    const FName RequestedLevelId = CurrentDungeonLevelId.IsNone ()
        ? DungeonAsset->DefaultLevelId
        : CurrentDungeonLevelId;
    if (RequestedLevelId.IsNone ())
    {
        UE_LOG (LogTemp, Error,
            TEXT ("ApplyCurrentDungeonLevel failed: CurrentDungeonLevelId and DefaultLevelId are both None."));
        return false;
    }

    const FGridDungeonLevelEntry* Entry = DungeonAsset->FindLevelEntry (RequestedLevelId);
    if (!Entry)
    {
        UE_LOG (LogTemp, Error, TEXT ("ApplyCurrentDungeonLevel failed: LevelId %s was not found."), *RequestedLevelId.ToString ());
        return false;
    }

    if (!Entry->bEnabled)
    {
        UE_LOG (LogTemp, Error, TEXT ("ApplyCurrentDungeonLevel failed: LevelId %s is disabled."), *RequestedLevelId.ToString ());
        return false;
    }

    if (!Entry->LevelAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("ApplyCurrentDungeonLevel failed: LevelId %s has no LevelAsset."), *RequestedLevelId.ToString ());
        return false;
    }

#if WITH_EDITOR
    Modify ();
#endif

    CurrentDungeonLevelId = RequestedLevelId;
    LevelAsset = Entry->LevelAsset;

    SyncPreviewRuntimeLevelAsset ();

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("ApplyCurrentDungeonLevel OK: LevelId=%s LevelAsset=%s."),
        *CurrentDungeonLevelId.ToString (),
        *GetNameSafe (LevelAsset));
    return true;
}

void AGridLevelEditorActor::ApplyCurrentDungeonLevelInEditor ()
{
    ApplyCurrentDungeonLevel ();
}

void AGridLevelEditorActor::LoadDefaultDungeonLevelInEditor ()
{
    if (!DungeonAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("LoadDefaultDungeonLevel failed: DungeonAsset is null."));
        return;
    }

    if (DungeonAsset->IsValidLevelId (DungeonAsset->DefaultLevelId))
    {
#if WITH_EDITOR
        Modify ();
#endif
