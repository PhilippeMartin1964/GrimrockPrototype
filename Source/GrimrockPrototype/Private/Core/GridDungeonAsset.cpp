#include "Core/GridDungeonAsset.h"

namespace
{
    FString GetGridEdgeName (EGridEdge Edge)
    {
        if (const UEnum* EdgeEnum = StaticEnum<EGridEdge> ())
        {
            return EdgeEnum->GetNameStringByValue (static_cast<int64> (Edge));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (Edge));
    }

    FString GetGridObjectTypeName (EGridLevelObjectType Type)
    {
        if (const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ())
        {
            return TypeEnum->GetNameStringByValue (static_cast<int64> (Type));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (Type));
    }

    int32 CountTransitionObjects (const UGridLevelAsset* LevelAsset)
    {
        if (!LevelAsset)
        {
            return 0;
        }

        int32 TransitionCount = 0;
        for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
        {
            if (Obj.Behavior.Transition.bIsTransition)
            {
                ++TransitionCount;
            }
        }

        return TransitionCount;
    }
}

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
    Result += TEXT ("GridDungeonAsset Diagnostics\n");
    Result += FString::Printf (TEXT ("Asset: %s\n"), *GetPathName ());
    Result += FString::Printf (TEXT ("DungeonName: %s\n"), *DungeonName.ToString ());
    Result += FString::Printf (TEXT ("Author: %s\n"), *Author.ToString ());
    Result += FString::Printf (TEXT ("Version: %s\n"), *Version);
    Result += FString::Printf (TEXT ("DefaultLevelId: %s\n"), *DefaultLevelId.ToString ());
    Result += FString::Printf (TEXT ("Levels: %d\n"), Levels.Num ());

    int32 EnabledCount = 0;
    int32 MissingAssetCount = 0;
    int32 TransitionObjectCount = 0;
    TSet<FName> SeenIds;
    TSet<FName> DuplicateIds;

    for (int32 LevelIndex = 0; LevelIndex < Levels.Num (); ++LevelIndex)
    {
        const FGridDungeonLevelEntry& Entry = Levels[LevelIndex];

        if (Entry.bEnabled)
        {
            ++EnabledCount;
        }
        if (!Entry.LevelAsset)
        {
            ++MissingAssetCount;
        }
        if (Entry.bEnabled)
        {
            TransitionObjectCount += CountTransitionObjects (Entry.LevelAsset);
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
            TEXT ("[%d] LevelId=%s DisplayName=%s Enabled=%s LogicalPosition=(%d,%d,%d) LevelAsset=%s\n"),
            LevelIndex,
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
    Result += FString::Printf (TEXT ("TransitionObjects=%d\n"), TransitionObjectCount);

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

FString UGridDungeonAsset::GetTransitionDiagnostics () const
{
    FString Result;
    Result += TEXT ("GridDungeonAsset Transition Diagnostics\n");
    Result += FString::Printf (TEXT ("DungeonAsset=%s\n"), *GetPathName ());

    int32 TransitionObjectCount = 0;
    int32 ErrorCount = 0;
    int32 WarningCount = 0;

    for (const FGridDungeonLevelEntry& SourceEntry : Levels)
    {
        if (!SourceEntry.bEnabled || !SourceEntry.LevelAsset)
        {
            continue;
        }

        const UGridLevelAsset* SourceLevelAsset = SourceEntry.LevelAsset.Get ();
        for (const FGridLevelObjectData& Obj : SourceLevelAsset->Objects)
        {
            const FGridObjectTransitionParams& Transition = Obj.Behavior.Transition;
            if (!Transition.bIsTransition)
            {
                continue;
            }

            FString TransitionStatus = TEXT ("OK");
            TArray<FString> StatusMessages;
            int32 LocalErrors = 0;
            int32 LocalWarnings = 0;

            const FGridDungeonLevelEntry* TargetEntry = nullptr;
            const UGridLevelAsset* TargetLevelAsset = nullptr;

            if (Transition.TargetLevelId.IsNone ())
            {
                ++LocalErrors;
                StatusMessages.Add (TEXT ("TargetLevelId is None"));
            }
            else
            {
                TargetEntry = FindLevelEntry (Transition.TargetLevelId);
                if (!TargetEntry)
                {
                    ++LocalErrors;
                    StatusMessages.Add (TEXT ("TargetLevelId does not exist in DungeonAsset"));
                }
                else if (!TargetEntry->bEnabled)
                {
                    ++LocalErrors;
                    StatusMessages.Add (TEXT ("TargetLevelId references a disabled dungeon level"));
                }
                else if (!TargetEntry->LevelAsset)
                {
                    ++LocalWarnings;
                    StatusMessages.Add (TEXT ("Target LevelAsset is null"));
                }
                else
                {
                    TargetLevelAsset = TargetEntry->LevelAsset.Get ();
                }
            }

            if (Transition.TargetFacing == EGridEdge::None)
            {
                ++LocalErrors;
                StatusMessages.Add (TEXT ("TargetFacing is None"));
            }

            if (TargetLevelAsset)
            {
                if (!TargetLevelAsset->IsValidCoord (Transition.TargetCellX, Transition.TargetCellY))
                {
                    ++LocalErrors;
                    StatusMessages.Add (TEXT ("Target cell is outside target level bounds"));
                }
                else
                {
                    const FGridLevelCellData& TargetCell = TargetLevelAsset->GetCell (Transition.TargetCellX, Transition.TargetCellY);
                    if (TargetCell.CellType == EGridCellType::Empty)
                    {
                        ++LocalWarnings;
                        StatusMessages.Add (TEXT ("Target cell is Empty"));
                    }
                    if (TargetCell.bBlocksOccupancy)
                    {
                        ++LocalWarnings;
                        StatusMessages.Add (TEXT ("Target cell blocks occupancy"));
                    }
                }
            }

            if (LocalErrors > 0)
            {
                TransitionStatus = TEXT ("ERROR");
            }
            else if (LocalWarnings > 0)
            {
                TransitionStatus = TEXT ("WARNING");
            }

            ErrorCount += LocalErrors;
            WarningCount += LocalWarnings;

            Result += FString::Printf (
                TEXT ("[%d] SourceLevelId=%s SourceDisplayName=%s SourceLevelAsset=%s ObjectId=%s ArchetypeId=%s Type=%s Cell=(%d,%d) Edge=%s TargetLevelId=%s TargetCell=(%d,%d) TargetFacing=%s bRequireUseAction=%s Status=%s"),
                TransitionObjectCount,
                *SourceEntry.LevelId.ToString (),
                *SourceEntry.DisplayName.ToString (),
                *SourceLevelAsset->GetPathName (),
                *Obj.ObjectId.ToString (),
                *Obj.ArchetypeId.ToString (),
                *GetGridObjectTypeName (Obj.Type),
                Obj.CellX,
                Obj.CellY,
                *GetGridEdgeName (Obj.Edge),
                *Transition.TargetLevelId.ToString (),
                Transition.TargetCellX,
                Transition.TargetCellY,
                *GetGridEdgeName (Transition.TargetFacing),
                Transition.bRequireUseAction ? TEXT ("true") : TEXT ("false"),
                *TransitionStatus);

            if (StatusMessages.Num () > 0)
            {
                Result += FString::Printf (TEXT (" - %s"), *FString::Join (StatusMessages, TEXT ("; ")));
            }
            Result += TEXT ("\n");

            ++TransitionObjectCount;
        }
    }

    Result += FString::Printf (TEXT ("TransitionObjects=%d\n"), TransitionObjectCount);
    Result += FString::Printf (TEXT ("Errors=%d\n"), ErrorCount);
    Result += FString::Printf (TEXT ("Warnings=%d\n"), WarningCount);

    if (ErrorCount > 0)
    {
        Result += TEXT ("Status=ERROR");
    }
    else if (WarningCount > 0)
    {
        Result += TEXT ("Status=WARNING");
    }
    else
    {
        Result += TEXT ("Status=OK");
    }

    return Result;
}
