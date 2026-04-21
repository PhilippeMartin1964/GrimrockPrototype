#include "Core/GridLevelAsset.h"

void UGridLevelAsset::EnsureCellCount ()
{
    const int32 Expected = FMath::Max (1, Width) * FMath::Max (1, Height);

    if (Cells.Num () != Expected)
    {
        Cells.SetNum (Expected);
    }
}

bool UGridLevelAsset::IsValidCoord (int32 X, int32 Y) const
{
    return X >= 0 && X < Width && Y >= 0 && Y < Height;
}

int32 UGridLevelAsset::GetIndex (int32 X, int32 Y) const
{
    return Y * Width + X;
}

const FGridLevelCellData& UGridLevelAsset::GetCell (int32 X, int32 Y) const
{
    check (IsValidCoord (X, Y));
    return Cells[GetIndex (X, Y)];
}

FGridLevelCellData& UGridLevelAsset::GetCellMutable (int32 X, int32 Y)
{
    check (IsValidCoord (X, Y));
    return Cells[GetIndex (X, Y)];
}

void UGridLevelAsset::ClearLevel ()
{
#if WITH_EDITOR
    Modify ();
#endif

    EnsureCellCount ();

    for (FGridLevelCellData& Cell : Cells)
    {
        Cell = FGridLevelCellData ();
    }

    Objects.Reset ();
    Links.Reset ();

#if WITH_EDITOR
    MarkPackageDirty ();
#endif
}

FGuid UGridLevelAsset::AddObject (const FGridLevelObjectData& NewObject)
{
#if WITH_EDITOR
    Modify ();
#endif

    FGridLevelObjectData Obj = NewObject;

    if (!Obj.ObjectId.IsValid ())
    {
        Obj.ObjectId = FGuid::NewGuid ();
    }

    Objects.Add (Obj);

#if WITH_EDITOR
    MarkPackageDirty ();
#endif

    return Obj.ObjectId;
}

bool UGridLevelAsset::RemoveObjectById (const FGuid& ObjectId)
{
#if WITH_EDITOR
    Modify ();
#endif

    const int32 Index = Objects.IndexOfByPredicate (
        [&] (const FGridLevelObjectData& Obj)
    {
        return Obj.ObjectId == ObjectId;
    });

    if (Index == INDEX_NONE)
    {
        return false;
    }

    Objects.RemoveAt (Index);

    RemoveLinksForObject (ObjectId);

#if WITH_EDITOR
    MarkPackageDirty ();
#endif

    return true;
}

void UGridLevelAsset::RemoveLinksForObject (const FGuid& ObjectId)
{
    Links.RemoveAll (
        [&] (const FGridLevelLinkData& Link)
    {
        return Link.SourceObjectId == ObjectId ||
            Link.TargetObjectId == ObjectId;
    });
}

void UGridLevelAsset::EnsureObjectIds ()
{
#if WITH_EDITOR
    Modify ();
#endif

    for (FGridLevelObjectData& Obj : Objects)
    {
        if (!Obj.ObjectId.IsValid ())
        {
            Obj.ObjectId = FGuid::NewGuid ();
        }
    }

#if WITH_EDITOR
    MarkPackageDirty ();
#endif
}