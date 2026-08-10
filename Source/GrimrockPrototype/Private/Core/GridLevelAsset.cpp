#include "Core/GridLevelAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace
{
    bool IsCardinalFacing (EGridEdge Facing)
    {
        return Facing == EGridEdge::North ||
            Facing == EGridEdge::East ||
            Facing == EGridEdge::South ||
            Facing == EGridEdge::West;
    }

    float GetYawForFacing (EGridEdge Facing)
    {
        switch (Facing)
        {
            case EGridEdge::East:  return 90.0f;
            case EGridEdge::South: return 180.0f;
            case EGridEdge::West:  return 270.0f;
            case EGridEdge::North:
            case EGridEdge::None:
            default:               return 0.0f;
        }
    }

    EGridEdge GetFacingForLegacyYaw (float LocalYaw)
    {
        if (!FMath::IsFinite (LocalYaw))
        {
            return EGridEdge::North;
        }

        const float NormalizedYaw = FMath::Fmod (LocalYaw, 360.0f);
        const float PositiveYaw = NormalizedYaw < 0.0f
            ? NormalizedYaw + 360.0f
            : NormalizedYaw;
        const int32 QuarterTurn =
            FMath::RoundToInt (PositiveYaw / 90.0f) % 4;

        switch (QuarterTurn)
        {
            case 1:  return EGridEdge::East;
            case 2:  return EGridEdge::South;
            case 3:  return EGridEdge::West;
            case 0:
            default: return EGridEdge::North;
        }
    }

    void NormalizeMonsterSpawnData (FGridLevelObjectData& ObjectData)
    {
        if (ObjectData.Type != EGridLevelObjectType::MonsterSpawn)
        {
            return;
        }

        if (!IsCardinalFacing (ObjectData.InitialFacing))
        {
            ObjectData.InitialFacing =
                GetFacingForLegacyYaw (ObjectData.LocalYaw);
        }

        // InitialFacing is authoritative; LocalYaw is retained only so the
        // existing generic editor preview keeps the same orientation.
        ObjectData.LocalYaw = GetYawForFacing (ObjectData.InitialFacing);

        if (ObjectData.MonsterDefinitionAsset &&
            ObjectData.MonsterDefinitionId.IsNone ())
        {
            ObjectData.MonsterDefinitionId =
                ObjectData.MonsterDefinitionAsset->MonsterId;
        }
    }

    FString GetMonsterSpawnLabel (const FGridLevelObjectData& Spawn)
    {
        return Spawn.ObjectId.IsValid ()
            ? Spawn.ObjectId.ToString (EGuidFormats::DigitsWithHyphens)
            : FString::Printf (
                TEXT ("at (%d,%d)"),
                Spawn.CellX,
                Spawn.CellY);
    }
}

void UGridLevelAsset::PostLoad ()
{
    Super::PostLoad ();

    for (FGridLevelObjectData& ObjectData : Objects)
    {
        NormalizeMonsterSpawnData (ObjectData);
    }
}

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

bool UGridLevelAsset::IsStartCellValid () const
{
    if (!IsValidCoord (StartCellX, StartCellY))
    {
        return false;
    }

    const int32 CellIndex = GetIndex (StartCellX, StartCellY);
    if (!Cells.IsValidIndex (CellIndex))
    {
        return false;
    }

    const FGridLevelCellData& StartCell = Cells[CellIndex];
    return StartCell.CellType != EGridCellType::Empty && !StartCell.bBlocksOccupancy;
}

FIntPoint UGridLevelAsset::GetStartCell () const
{
    return FIntPoint (StartCellX, StartCellY);
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

    NormalizeMonsterSpawnData (Obj);

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
        [&] (const FGridObjectLink& Link)
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

        NormalizeMonsterSpawnData (Obj);
    }

#if WITH_EDITOR
    MarkPackageDirty ();
#endif
}

bool UGridLevelAsset::ValidateMonsterSpawns (
    TArray<FString>& OutErrors) const
{
    OutErrors.Reset ();

    TMap<FGuid, int32> ObjectIdCounts;
    for (const FGridLevelObjectData& ObjectData : Objects)
    {
        if (ObjectData.ObjectId.IsValid ())
        {
            ++ObjectIdCounts.FindOrAdd (ObjectData.ObjectId);
        }
    }

    TMap<FIntPoint, FGuid> EnabledSpawnByCell;
    for (const FGridLevelObjectData& Spawn : Objects)
    {
        if (Spawn.Type != EGridLevelObjectType::MonsterSpawn)
        {
            continue;
        }

        const FString SpawnLabel = GetMonsterSpawnLabel (Spawn);
        if (!Spawn.ObjectId.IsValid ())
        {
            OutErrors.Add (FString::Printf (
                TEXT ("MonsterSpawn %s requires a valid ObjectId/SpawnId."),
                *SpawnLabel));
        }
        else if (ObjectIdCounts.FindRef (Spawn.ObjectId) != 1)
        {
            OutErrors.Add (FString::Printf (
                TEXT ("MonsterSpawn %s does not have a unique ObjectId/SpawnId."),
                *SpawnLabel));
        }

        if (!IsValidCoord (Spawn.CellX, Spawn.CellY))
        {
            OutErrors.Add (FString::Printf (
                TEXT ("MonsterSpawn %s is outside grid bounds at (%d,%d)."),
                *SpawnLabel,
                Spawn.CellX,
                Spawn.CellY));
        }
        else
        {
            const int32 CellIndex = GetIndex (Spawn.CellX, Spawn.CellY);
            if (!Cells.IsValidIndex (CellIndex))
            {
                OutErrors.Add (FString::Printf (
                    TEXT ("MonsterSpawn %s cannot resolve its cell because the level cell array is incomplete."),
                    *SpawnLabel));
            }
            else
            {
                const FGridLevelCellData& Cell = Cells[CellIndex];
                if (Cell.CellType == EGridCellType::Empty ||
                    Cell.bBlocksOccupancy)
                {
                    OutErrors.Add (FString::Printf (
                        TEXT ("MonsterSpawn %s must be placed on a non-empty cell that allows occupancy."),
                        *SpawnLabel));
                }

                if (Spawn.bInitiallyEnabled)
                {
                    const FIntPoint SpawnCell (Spawn.CellX, Spawn.CellY);
                    if (const FGuid* ExistingSpawnId =
                        EnabledSpawnByCell.Find (SpawnCell))
                    {
                        OutErrors.Add (FString::Printf (
                            TEXT ("MonsterSpawn %s shares initial cell (%d,%d) with enabled MonsterSpawn %s."),
                            *SpawnLabel,
                            Spawn.CellX,
                            Spawn.CellY,
                            *ExistingSpawnId->ToString (
                                EGuidFormats::DigitsWithHyphens)));
                    }
                    else
                    {
                        EnabledSpawnByCell.Add (
                            SpawnCell,
                            Spawn.ObjectId);
                    }
                }
            }
        }

        if (Spawn.Edge != EGridEdge::None)
        {
            OutErrors.Add (FString::Printf (
                TEXT ("MonsterSpawn %s is cell-centered and requires Edge=None."),
                *SpawnLabel));
        }

        if (!IsCardinalFacing (Spawn.InitialFacing))
        {
            OutErrors.Add (FString::Printf (
                TEXT ("MonsterSpawn %s requires a cardinal InitialFacing."),
                *SpawnLabel));
        }

        const UGridMonsterDefinitionAsset* Definition =
            Spawn.MonsterDefinitionAsset;
        const FName AssetDefinitionId = Definition
            ? Definition->MonsterId
            : NAME_None;
        const FName ResolvedDefinitionId = !AssetDefinitionId.IsNone ()
            ? AssetDefinitionId
            : Spawn.MonsterDefinitionId;

        if (ResolvedDefinitionId.IsNone ())
        {
            OutErrors.Add (FString::Printf (
                TEXT ("MonsterSpawn %s requires MonsterDefinitionAsset or MonsterDefinitionId."),
                *SpawnLabel));
        }

        if (Definition)
        {
            FString DefinitionError;
            if (!Definition->ValidateDefinition (DefinitionError))
            {
                OutErrors.Add (FString::Printf (
                    TEXT ("MonsterSpawn %s references invalid MonsterDefinition '%s': %s"),
                    *SpawnLabel,
                    *GetNameSafe (Definition),
                    *DefinitionError));
            }

            if (!Spawn.MonsterDefinitionId.IsNone () &&
                Spawn.MonsterDefinitionId != AssetDefinitionId)
            {
                OutErrors.Add (FString::Printf (
                    TEXT ("MonsterSpawn %s stores MonsterDefinitionId '%s' but its asset resolves to '%s'."),
                    *SpawnLabel,
                    *Spawn.MonsterDefinitionId.ToString (),
                    *AssetDefinitionId.ToString ()));
            }
        }
    }

    return OutErrors.IsEmpty ();
}

const FGridLevelObjectData* UGridLevelAsset::FindMonsterSpawnById (
    const FGuid& SpawnId) const
{
    if (!SpawnId.IsValid ())
    {
        return nullptr;
    }

    return Objects.FindByPredicate (
        [&SpawnId] (const FGridLevelObjectData& ObjectData)
    {
        return ObjectData.Type == EGridLevelObjectType::MonsterSpawn &&
            ObjectData.ObjectId == SpawnId;
    });
}
