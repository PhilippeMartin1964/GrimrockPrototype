#include "Core/GridDungeonAsset.h"

const FGridDungeonLevelEntry* UGridDungeonAsset::FindLevelEntry (FName LevelId) const
{
    if (LevelId.IsNone ())
    {
        return nullptr;
    }

    return Levels.FindByPredicate ([LevelId] (const FGridDungeonLevelEntry& Entry)
    {
        return Entry.LevelId == LevelId;
    });
}

bool UGridDungeonAsset::IsValidLevelId (FName LevelId) const
{
    const FGridDungeonLevelEntry* Entry = FindLevelEntry (LevelId);
    return Entry && Entry->bEnabled && Entry->LevelAsset != nullptr;
}

UGridLevelAsset* UGridDungeonAsset::GetLevelAssetById (FName LevelId) const
{
    const FGridDungeonLevelEntry* Entry = FindLevelEntry (LevelId);
    if (!Entry || !Entry->bEnabled)
    {
        return nullptr;
    }

    return Entry->LevelAsset.Get ();
}

UGridLevelAsset* UGridDungeonAsset::GetDefaultLevelAsset () const
{
    if (UGridLevelAsset* DefaultAsset = GetLevelAssetById (DefaultLevelId))
    {
        return DefaultAsset;
    }

    for (const FGridDungeonLevelEntry& Entry : Levels)
    {
        if (Entry.bEnabled && Entry.LevelAsset)
        {
            return Entry.LevelAsset.Get ();
        }
    }

    return nullptr;
}

FString UGridDungeonAsset::GetDungeonDiagnostics () const
{
    FString Result;
    Result += FString::Printf (TEXT ("GridDungeonAsset: %s\n"), *GetPathName ());
    Result += FString::Printf (TEXT ("DefaultLevelId: %s\n"), *DefaultLevelId.ToString ());
    Result += FString::Printf (TEXT ("LevelCount: %d\n"), Levels.Num ());

    int32 EnabledCount = 0;
    int32 MissingAssetCount = 0;
    TSet<FName> SeenIds;
    TSet<FName> DuplicateIds;

    for (const FGridDungeonLevelEntry& Entry : Levels)
    {
        if (Entry.bEnabled)
        {
            ++EnabledCount;
        }
        if (!Entry.LevelAsset)
        {
            ++MissingAssetCount;
        }
        if (!Entry.LevelId.IsNone ())
        {
            if (SeenIds.Contains (Entry.LevelId))
            {
                DuplicateIds.Add (Entry.LevelId);
            }
            SeenIds.Add (Entry.LevelId);
        }

        Result += FString::Printf (
            TEXT ("- LevelId=%s DisplayName=%s Enabled=%s LogicalPosition=(%d,%d,%d) LevelAsset=%s\n"),
            *Entry.LevelId.ToString (),
            *Entry.DisplayName.ToString (),
            Entry.bEnabled ? TEXT ("true") : TEXT ("false"),
            Entry.LogicalPosition.X,
            Entry.LogicalPosition.Y,
            Entry.LogicalPosition.Z,
            Entry.LevelAsset ? *Entry.LevelAsset->GetPathName () : TEXT ("None"));
    }

    Result += FString::Printf (TEXT ("EnabledLevels: %d\n"), EnabledCount);
    Result += FString::Printf (TEXT ("MissingLevelAssets: %d\n"), MissingAssetCount);
    Result += FString::Printf (TEXT ("DuplicateLevelIds: %d\n"), DuplicateIds.Num ());

    if (Levels.Num () == 0)
    {
        Result += TEXT ("Status: WARNING - Dungeon has no levels.");
    }
    else if (MissingAssetCount > 0)
    {
        Result += TEXT ("Status: WARNING - One or more level entries have no LevelAsset.");
    }
    else if (DuplicateIds.Num () > 0)
    {
        Result += TEXT ("Status: WARNING - Duplicate LevelId values found.");
    }
    else if (!DefaultLevelId.IsNone () && !IsValidLevelId (DefaultLevelId))
    {
        Result += TEXT ("Status: WARNING - DefaultLevelId does not reference an enabled level with a LevelAsset.");
    }
    else
    {
        Result += TEXT ("Status: OK");
    }

    return Result;
}
