TArray<FGridLevelValidationMessage> AGridLevelEditorActor::ValidateCurrentLevel ()
{
    LastValidationMessages.Reset ();

    auto AddMessage = [this] (
        EGridLevelValidationSeverity Severity,
        const FString& Message,
        const FGuid& OptionalObjectId = FGuid ())
    {
        FGridLevelValidationMessage ValidationMessage;
        ValidationMessage.Severity = Severity;
        ValidationMessage.Message = Message;
        ValidationMessage.OptionalObjectId = OptionalObjectId;
        LastValidationMessages.Add (ValidationMessage);
    };

    auto AddArchetypeValidationMessages = [this, &AddMessage] ()
    {
        if (!ObjectPalette)
        {
            return;
        }

        TSet<const UGridObjectArchetypeAsset*> ValidatedArchetypes;
        TSet<const UGridObjectArchetypeAsset*> DirectPaintItemArchetypes;

        TArray<FGridArchetypeValidationMessage> PaletteMessages;
        ObjectPalette->ValidatePalette (PaletteMessages);
        for (const FGridArchetypeValidationMessage& PaletteMessage : PaletteMessages)
        {
            AddMessage (
                ConvertArchetypeValidationSeverity (PaletteMessage.Severity),
                FString::Printf (TEXT ("ObjectPalette: %s"), *PaletteMessage.Message));
        }

        for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
        {
            const UGridObjectArchetypeAsset* Archetype = Entry.DefaultArchetype.Get ();
            if (!Archetype)
            {
                continue;
            }

            const FString ArchetypeName = Archetype->ArchetypeId.IsNone ()
                ? Archetype->GetName ()
                : Archetype->ArchetypeId.ToString ();

            if (Archetype->SupportedType == EGridLevelObjectType::Item && !DirectPaintItemArchetypes.Contains (Archetype))
            {
                DirectPaintItemArchetypes.Add (Archetype);
                AddMessage (
                    EGridLevelValidationSeverity::Info,
                    FString::Printf (
                        TEXT ("Archetype %s: Item archetype is directly available in the paint palette as a placed pickup item."),
                        *ArchetypeName));
            }

            if (ValidatedArchetypes.Contains (Archetype))
            {
                continue;
            }

            ValidatedArchetypes.Add (Archetype);

            TArray<FGridArchetypeValidationMessage> ArchetypeMessages;
            Archetype->ValidateArchetype (ArchetypeMessages);

            for (const FGridArchetypeValidationMessage& ArchetypeMessage : ArchetypeMessages)
            {
                AddMessage (
                    ConvertArchetypeValidationSeverity (ArchetypeMessage.Severity),
                    FString::Printf (
                        TEXT ("Archetype %s: %s"),
                        *ArchetypeName,
                        *ArchetypeMessage.Message));
            }
        }
    };

    auto AddExpectedConcreteArchetypeMessages = [this, &AddMessage] ()
    {
        if (!ObjectPalette)
        {
            return;
        }

        for (const FExpectedConcreteArchetypeSpec& ExpectedSpec : ExpectedConcreteArchetypes)
        {
            const FName ExpectedArchetypeId (ExpectedSpec.ArchetypeId);
            const FGridObjectPaletteEntry* MatchingEntry = nullptr;

            for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
            {
                if (Entry.GetEffectiveArchetypeId () == ExpectedArchetypeId)
                {
                    MatchingEntry = &Entry;
                    break;
                }
            }

            if (!MatchingEntry)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("ObjectPalette should expose concrete archetype '%s'. Variants must be palette entries/archetypes, not new EGridLevelObjectType values."),
                        ExpectedSpec.ArchetypeId));
                continue;
            }

            const EGridLevelObjectType EffectiveType = MatchingEntry->GetEffectiveObjectType ();
            if (EffectiveType != ExpectedSpec.ExpectedType)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("ObjectPalette archetype '%s' should use Type=%s, but currently uses Type=%s."),
                        ExpectedSpec.ArchetypeId,
                        *ToGridObjectTypeText (ExpectedSpec.ExpectedType),
                        *ToGridObjectTypeText (EffectiveType)));
            }
        }
    };

    if (!DungeonAsset)
    {
        AddMessage (
            EGridLevelValidationSeverity::Warning,
            TEXT ("DungeonAsset is missing. The editor can use LevelAsset directly, but dungeon level ids and transitions cannot be fully validated."));
    }
    else
    {
        TSet<FName> SeenLevelIds;
        TSet<FIntVector> SeenLogicalPositions;
        bool bHasEnabledFallbackLevel = false;

        if (DungeonAsset->Levels.Num () == 0)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("DungeonAsset contains no level entries."));
        }

        for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
        {
            if (Entry.LevelId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("DungeonAsset contains a level entry with an empty LevelId."));
            }
            else if (SeenLevelIds.Contains (Entry.LevelId))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (TEXT ("DungeonAsset contains duplicate LevelId '%s'."), *Entry.LevelId.ToString ()));
            }
            SeenLevelIds.Add (Entry.LevelId);

            if (SeenLogicalPositions.Contains (Entry.LogicalPosition))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("DungeonAsset contains duplicate LogicalPosition (%d,%d,%d)."),
                        Entry.LogicalPosition.X,
                        Entry.LogicalPosition.Y,
                        Entry.LogicalPosition.Z));
            }
            SeenLogicalPositions.Add (Entry.LogicalPosition);

            if (!Entry.LevelAsset)
            {
                AddMessage (
                    Entry.bEnabled ? EGridLevelValidationSeverity::Error : EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Dungeon level '%s' has no LevelAsset."),
                        Entry.LevelId.IsNone () ? TEXT ("None") : *Entry.LevelId.ToString ()));
            }
            else if (Entry.bEnabled && !Entry.LevelId.IsNone ())
            {
                bHasEnabledFallbackLevel = true;
            }
        }

        if (!DungeonAsset->IsValidLevelId (DungeonAsset->DefaultLevelId))
        {
            AddMessage (
                bHasEnabledFallbackLevel ? EGridLevelValidationSeverity::Warning : EGridLevelValidationSeverity::Error,
                bHasEnabledFallbackLevel
                    ? TEXT ("DefaultLevelId is invalid; runtime/editor fallback will use the first enabled level with a LevelAsset.")
                    : TEXT ("DefaultLevelId is invalid and no enabled fallback level with a LevelAsset exists."));
        }
    }

    AddArchetypeValidationMessages ();
    AddExpectedConcreteArchetypeMessages ();

    if (!LevelAsset)
    {
        AddMessage (
            EGridLevelValidationSeverity::Error,
            TEXT ("LevelAsset is missing."));
        return LastValidationMessages;
    }

    if (LevelAsset->Width <= 0)
    {
        AddMessage (EGridLevelValidationSeverity::Error, TEXT ("LevelAsset Width must be greater than zero."));
    }
    if (LevelAsset->Height <= 0)
    {
        AddMessage (EGridLevelValidationSeverity::Error, TEXT ("LevelAsset Height must be greater than zero."));
    }
    if (LevelAsset->CellSize <= 0.f)
    {
        AddMessage (EGridLevelValidationSeverity::Error, TEXT ("LevelAsset CellSize must be greater than zero."));
    }

    const int32 ExpectedCellCount = FMath::Max (1, LevelAsset->Width) * FMath::Max (1, LevelAsset->Height);
    if (LevelAsset->Cells.Num () != ExpectedCellCount)
    {
        AddMessage (
            EGridLevelValidationSeverity::Error,
            FString::Printf (
                TEXT ("LevelAsset Cells.Num()=%d but expected %d for Width=%d Height=%d."),
                LevelAsset->Cells.Num (),
                ExpectedCellCount,
                LevelAsset->Width,
                LevelAsset->Height));
    }

    if (!LevelAsset->IsStartCellValid ())
    {
        AddMessage (
            EGridLevelValidationSeverity::Error,
            FString::Printf (
                TEXT ("Start cell X=%d Y=%d Facing=%s is invalid. It must be inside the grid, non-empty and not block occupancy."),
                LevelAsset->StartCellX,
                LevelAsset->StartCellY,
                *GetGridEdgeText (LevelAsset->StartFacing)));
    }

    if (LevelAsset->Width > 0 && LevelAsset->Height > 0 && LevelAsset->Cells.Num () == ExpectedCellCount)
    {
        int32 OverlappingSharedWallCount = 0;
        int32 DirectionalSharedWallCount = 0;
        FString FirstOverlappingSharedWall;
        FString FirstDirectionalSharedWall;

        for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
        {
            for (int32 X = 0; X < LevelAsset->Width; ++X)
            {
                const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);

                auto ValidateSharedEdge = [
                    &OverlappingSharedWallCount,
                    &DirectionalSharedWallCount,
                    &FirstOverlappingSharedWall,
                    &FirstDirectionalSharedWall,
                    X,
                    Y] (
                    const TCHAR* EdgeName,
                    EGridWallType LocalWall,
                    EGridWallType OppositeWall,
                    int32 NeighborX,
                    int32 NeighborY)
                {
                    if (LocalWall != EGridWallType::None && OppositeWall != EGridWallType::None)
                    {
                        ++OverlappingSharedWallCount;
                        if (FirstOverlappingSharedWall.IsEmpty ())
                        {
                            FirstOverlappingSharedWall = FString::Printf (
                                TEXT ("%s between (%d,%d) and (%d,%d)"),
                                EdgeName, X, Y, NeighborX, NeighborY);
                        }
                    }
                    else if (LocalWall != OppositeWall)
                    {
                        ++DirectionalSharedWallCount;
                        if (FirstDirectionalSharedWall.IsEmpty ())
                        {
                            FirstDirectionalSharedWall = FString::Printf (
                                TEXT ("%s between (%d,%d) and (%d,%d), %s vs %s"),
                                EdgeName,
                                X,
                                Y,
                                NeighborX,
                                NeighborY,
                                *UEnum::GetValueAsString (LocalWall),
                                *UEnum::GetValueAsString (OppositeWall));
                        }
                    }
                };

                if (X + 1 < LevelAsset->Width)
                {
                    ValidateSharedEdge (
                        TEXT ("East/West"),
                        Cell.EastWall,
                        LevelAsset->GetCell (X + 1, Y).WestWall,
                        X + 1,
                        Y);
                }
                if (Y + 1 < LevelAsset->Height)
                {
                    ValidateSharedEdge (
                        TEXT ("North/South"),
                        Cell.NorthWall,
                        LevelAsset->GetCell (X, Y + 1).SouthWall,
                        X,
                        Y + 1);
                }
            }
        }

        if (OverlappingSharedWallCount > 0)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                FString::Printf (
                    TEXT ("%d shared edges have walls on both sides; runtime rendering may create overlapping wall instances. First: %s."),
                    OverlappingSharedWallCount,
                    *FirstOverlappingSharedWall));
        }
        if (DirectionalSharedWallCount > 0)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                FString::Printf (
                    TEXT ("%d shared edges are directional; movement depends on the source cell. First: %s."),
                    DirectionalSharedWallCount,
                    *FirstDirectionalSharedWall));
        }
    }

    TSet<FGuid> SeenObjectIds;
    TMap<FGuid, const FGridLevelObjectData*> ObjectsById;
    TMap<FGuid, int32> OutgoingLinkCountBySourceId;
    TMap<FGuid, int32> ReceptacleItemInsertedLinkCountBySourceId;
    TMap<FGuid, int32> ReceptacleItemRemovedLinkCountBySourceId;
    TMap<FGuid, int32> ReceptacleItemChangedLinkCountBySourceId;
    TMap<FIntPoint, FGuid> EnabledMonsterSpawnByCell;
    TMap<FName, TMap<int32, TMap<FIntPoint, FGuid>>>
        EncounterMonsterSpawnByWaveAndCell;

    auto IsEdgeOrWallPlacedObject = [this] (const FGridLevelObjectData& ObjectData) -> bool
    {
        return IsEdgePlacedObject (ObjectData);
    };

    auto GetValidationAnchorKey = [&IsEdgeOrWallPlacedObject] (const FGridLevelObjectData& ObjectData) -> FString
    {
        if (!IsEdgeOrWallPlacedObject (ObjectData))
        {
            return TEXT ("Center");
        }

        switch (ObjectData.Edge)
        {
            case EGridEdge::North:
                return TEXT ("North");

            case EGridEdge::East:
                return TEXT ("East");

            case EGridEdge::South:
                return TEXT ("South");

            case EGridEdge::West:
                return TEXT ("West");

            case EGridEdge::None:
            default:
                return TEXT ("Center");
        }
    };

    auto GetObjectValidationName = [] (const FGridLevelObjectData& ObjectData) -> FString
    {
        if (!ObjectData.Tag.IsNone ())
        {
            return ObjectData.Tag.ToString ();
        }

        if (!ObjectData.ArchetypeId.IsNone ())
        {
            return ObjectData.ArchetypeId.ToString ();
        }

        return ObjectData.ObjectId.IsValid ()
            ? ObjectData.ObjectId.ToString ().Left (8)
            : FString (TEXT ("InvalidObjectId"));
    };

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (!Obj.ObjectId.IsValid ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Object at X=%d Y=%d has an invalid ObjectId."),
                    Obj.CellX,
                    Obj.CellY));
        } else if (SeenObjectIds.Contains (Obj.ObjectId))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("Duplicate ObjectId found."),
                Obj.ObjectId);
        } else
        {
            SeenObjectIds.Add (Obj.ObjectId);
            ObjectsById.Add (Obj.ObjectId, &Obj);
        }

        if (!LevelAsset->IsValidCoord (Obj.CellX, Obj.CellY))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Object is outside grid bounds at X=%d Y=%d."),
                    Obj.CellX,
                    Obj.CellY),
                Obj.ObjectId);
            continue;
        }

        const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (Obj.ArchetypeId);
        if (Obj.ArchetypeId.IsNone ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("Placed object has no ArchetypeId. Preview and runtime archetype lookup cannot resolve it."),
                Obj.ObjectId);
        }
        else if (ObjectPalette && !Archetype)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Placed object ArchetypeId '%s' is not exposed by the assigned ObjectPalette."),
                    *Obj.ArchetypeId.ToString ()),
                Obj.ObjectId);
        }

        if (Archetype && Obj.Type != Archetype->SupportedType)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Placed object Type=%s does not match archetype '%s' SupportedType=%s."),
                    *ToGridObjectTypeText (Obj.Type),
                    *Obj.ArchetypeId.ToString (),
                    *ToGridObjectTypeText (Archetype->SupportedType)),
                Obj.ObjectId);
        }

        if (Archetype)
        {
            if (Archetype->bIsReadable)
            {
                const FText EffectiveReadableText = Obj.OverrideReadableText.IsEmpty ()
                    ? Archetype->ReadableText
                    : Obj.OverrideReadableText;
                if (EffectiveReadableText.IsEmpty ())
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Warning,
                        Obj.Notes.IsEmpty ()
                            ? TEXT ("Readable placed object has no text in either its instance override or archetype.")
                            : TEXT ("Readable placed object has no text. Notes are editor-only and are not displayed at runtime."),
                        Obj.ObjectId);
                }
                if (!Obj.bInitiallyEnabled)
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Warning,
                        TEXT ("Readable placed object is initially disabled and cannot be read until enabled."),
                        Obj.ObjectId);
                }
            }
            else if (!Obj.OverrideReadableText.IsEmpty ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Placed object has a readable-text override, but its archetype is not readable; the override is ignored at runtime."),
                    Obj.ObjectId);
            }
        }

        if (Archetype &&
            Obj.Type != EGridLevelObjectType::Item &&
            Archetype->IsCenterPlaced () &&
            Obj.Edge != EGridEdge::None)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                FString::Printf (
                    TEXT ("Center-placed object has a cardinal Edge=%s; runtime center placement ignores this edge."),
                    *GetGridEdgeText (Obj.Edge)),
                Obj.ObjectId);
        }

        if (ObjectPalette && !Obj.PaletteEntryId.IsNone ())
        {
            const FGridObjectPaletteEntry* PaletteEntry = ObjectPalette->FindEntryById (Obj.PaletteEntryId);
            if (!PaletteEntry)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Placed object PaletteEntryId '%s' no longer exists in the assigned ObjectPalette."),
                        *Obj.PaletteEntryId.ToString ()),
                    Obj.ObjectId);
            }
            else if (PaletteEntry->GetEffectiveArchetypeId () != Obj.ArchetypeId)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Placed object PaletteEntryId '%s' now resolves to archetype '%s', but the object stores ArchetypeId '%s'."),
                        *Obj.PaletteEntryId.ToString (),
                        *PaletteEntry->GetEffectiveArchetypeId ().ToString (),
                        *Obj.ArchetypeId.ToString ()),
                    Obj.ObjectId);
            }
        }

        if (IsEdgeOrWallPlacedObject (Obj) && Obj.Edge == EGridEdge::None)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                TEXT ("Edge or wall placed object has Edge=None."),
                Obj.ObjectId);
        }

        if (Archetype && Archetype->bBlocksMovement)
        {
            const FGridLevelCellData& CellData = LevelAsset->GetCell (Obj.CellX, Obj.CellY);
            if (CellData.bBlocksOccupancy)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Object blocks movement on a cell that already blocks occupancy."),
                    Obj.ObjectId);
            }
        }

        if (Obj.Type == EGridLevelObjectType::Item)
        {
            const UGridItemDefinitionAsset* ArchetypeItemDefinition =
                Archetype ? Archetype->DefaultBehavior.Item.ItemDefinitionAsset.Get () : nullptr;
            const FName ArchetypeItemDefinitionId =
                ArchetypeItemDefinition && !ArchetypeItemDefinition->ItemDefinitionId.IsNone ()
                    ? ArchetypeItemDefinition->ItemDefinitionId
                    : (Archetype ? Archetype->DefaultBehavior.Item.ItemDefinitionId : NAME_None);
            const FName LocalAssetDefinitionId =
                Obj.ItemDefinitionAsset ? Obj.ItemDefinitionAsset->ItemDefinitionId : NAME_None;
            const FName EffectiveItemDefinitionId =
                !LocalAssetDefinitionId.IsNone ()
                    ? LocalAssetDefinitionId
                    : (!Obj.ItemDefinitionId.IsNone () ? Obj.ItemDefinitionId : ArchetypeItemDefinitionId);

            if (EffectiveItemDefinitionId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Placed item has no resolvable item definition in the object instance or its archetype defaults."),
                    Obj.ObjectId);
            }

            if (Obj.ItemDefinitionAsset && LocalAssetDefinitionId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Placed item references an item definition asset whose ItemDefinitionId is empty."),
                    Obj.ObjectId);
            }

            if (Obj.ItemDefinitionAsset &&
                !Obj.ItemDefinitionId.IsNone () &&
                Obj.ItemDefinitionId != LocalAssetDefinitionId)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Placed item stores ItemDefinitionId '%s' but its definition asset resolves to '%s'."),
                        *Obj.ItemDefinitionId.ToString (),
                        *LocalAssetDefinitionId.ToString ()),
                    Obj.ObjectId);
            }

            const FGridLevelCellData& ItemCell = LevelAsset->GetCell (Obj.CellX, Obj.CellY);
            if (ItemCell.CellType == EGridCellType::Empty || ItemCell.bBlocksOccupancy)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Placed item is on a non-walkable cell; normal party pickup cannot reach this cell."),
                    Obj.ObjectId);
            }
        }

        if (Obj.Type == EGridLevelObjectType::MonsterSpawn)
        {
            const UGridMonsterDefinitionAsset* Definition =
                Obj.MonsterDefinitionAsset;
            const FName AssetDefinitionId = Definition
                ? Definition->MonsterId
                : NAME_None;
            const FName ResolvedDefinitionId =
                !AssetDefinitionId.IsNone ()
                ? AssetDefinitionId
                : Obj.MonsterDefinitionId;

            if (ResolvedDefinitionId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("MonsterSpawn requires MonsterDefinitionAsset or MonsterDefinitionId."),
                    Obj.ObjectId);
            }

            if (Definition)
            {
                FString DefinitionError;
                if (!Definition->ValidateDefinition (DefinitionError))
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("MonsterSpawn references an invalid MonsterDefinition: %s"),
                            *DefinitionError),
                        Obj.ObjectId);
                }

                if (!Obj.MonsterDefinitionId.IsNone () &&
                    Obj.MonsterDefinitionId != AssetDefinitionId)
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("MonsterSpawn stores MonsterDefinitionId '%s' but its asset resolves to '%s'."),
                            *Obj.MonsterDefinitionId.ToString (),
                            *AssetDefinitionId.ToString ()),
                        Obj.ObjectId);
                }
            }

            const bool bCardinalFacing =
                Obj.InitialFacing == EGridEdge::North ||
                Obj.InitialFacing == EGridEdge::East ||
                Obj.InitialFacing == EGridEdge::South ||
                Obj.InitialFacing == EGridEdge::West;
            if (!bCardinalFacing)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("MonsterSpawn requires a cardinal InitialFacing."),
                    Obj.ObjectId);
            }
            else if (!FMath::IsNearlyEqual (
                Obj.LocalYaw,
                GetYawForOrientation (Obj.InitialFacing)))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("MonsterSpawn LocalYaw preview mirror differs from InitialFacing; InitialFacing remains authoritative."),
                    Obj.ObjectId);
            }

            if (Obj.Edge != EGridEdge::None)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("MonsterSpawn is cell-centered and requires Edge=None."),
                    Obj.ObjectId);
            }

            const FGridLevelCellData& SpawnCell =
                LevelAsset->GetCell (Obj.CellX, Obj.CellY);
            if (SpawnCell.CellType == EGridCellType::Empty ||
                SpawnCell.bBlocksOccupancy)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("MonsterSpawn must be placed on a non-empty cell that allows occupancy."),
                    Obj.ObjectId);
            }

            if (Obj.bInitiallyEnabled)
            {
                const FIntPoint CellKey (Obj.CellX, Obj.CellY);
                if (const FGuid* ExistingSpawnId =
                    EnabledMonsterSpawnByCell.Find (CellKey))
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("MonsterSpawn shares its initial cell with enabled MonsterSpawn %s."),
                            *ExistingSpawnId->ToString ()),
                        Obj.ObjectId);
                }
                else
                {
                    EnabledMonsterSpawnByCell.Add (
                        CellKey,
                    Obj.ObjectId);
                }
            }

            if (Obj.EncounterWaveIndex < 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("MonsterSpawn requires EncounterWaveIndex >= 0."),
                    Obj.ObjectId);
            }
            if (Obj.EncounterGroupId.IsNone () &&
                Obj.EncounterWaveIndex > 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("MonsterSpawn requires EncounterGroupId when EncounterWaveIndex is greater than 0."),
                    Obj.ObjectId);
            }
            if (!Obj.EncounterGroupId.IsNone () &&
                Obj.EncounterWaveIndex > 0 &&
                Obj.bInitiallyEnabled)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("MonsterSpawn belongs to future encounter wave %d and must be disabled at start."),
                        Obj.EncounterWaveIndex),
                    Obj.ObjectId);
            }
            if (!Obj.EncounterGroupId.IsNone () &&
                Obj.EncounterWaveIndex >= 0)
            {
                TMap<FIntPoint, FGuid>& SpawnByCell =
                    EncounterMonsterSpawnByWaveAndCell
                        .FindOrAdd (Obj.EncounterGroupId)
                        .FindOrAdd (Obj.EncounterWaveIndex);
                const FIntPoint CellKey (Obj.CellX, Obj.CellY);
                if (const FGuid* ExistingSpawnId =
                    SpawnByCell.Find (CellKey))
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("MonsterSpawn shares encounter wave %d cell with MonsterSpawn %s in encounter '%s'."),
                            Obj.EncounterWaveIndex,
                            *ExistingSpawnId->ToString (),
                            *Obj.EncounterGroupId.ToString ()),
                        Obj.ObjectId);
                }
                else
                {
                    SpawnByCell.Add (CellKey, Obj.ObjectId);
                }
            }
        }

        if (Obj.Type == EGridLevelObjectType::Door)
        {
            const FGridLevelCellData& CellData = LevelAsset->GetCell (Obj.CellX, Obj.CellY);
            const EGridWallType WallType = GetWallTypeForEdge (CellData, Obj.Edge);
            if (WallType == EGridWallType::Solid)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Door is placed on an edge whose wall is Solid. A door edge must use WallType=None."),
                Obj.ObjectId);
            }

            int32 NeighborX = Obj.CellX;
            int32 NeighborY = Obj.CellY;
            switch (Obj.Edge)
            {
                case EGridEdge::North: ++NeighborY; break;
                case EGridEdge::East:  ++NeighborX; break;
                case EGridEdge::South: --NeighborY; break;
                case EGridEdge::West:  --NeighborX; break;
                case EGridEdge::None:
                default:
                    break;
            }
            if (Obj.Edge != EGridEdge::None && !LevelAsset->IsValidCoord (NeighborX, NeighborY))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Door is placed on an outer grid edge with no neighboring cell to cross."),
                    Obj.ObjectId);
            }
        }

        if (Obj.Behavior.Transition.bIsTransition)
        {
            const FGridObjectTransitionParams& Transition = Obj.Behavior.Transition;
            if (Transition.TargetLevelId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Transition has no TargetLevelId."),
                    Obj.ObjectId);
            }

            if (Transition.TargetFacing == EGridEdge::None)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Transition TargetFacing cannot be None."),
                    Obj.ObjectId);
            }

            const UGridLevelAsset* TargetLevelAsset = nullptr;
            if (DungeonAsset)
            {
                TargetLevelAsset = DungeonAsset->GetLevelAssetById (Transition.TargetLevelId);
                if (!Transition.TargetLevelId.IsNone () && !TargetLevelAsset)
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("Transition target LevelId '%s' was not found as an enabled level with a LevelAsset in the DungeonAsset."),
                            *Transition.TargetLevelId.ToString ()),
                        Obj.ObjectId);
                }
            }
            else
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Transition cannot validate TargetLevelId because DungeonAsset is null."),
                    Obj.ObjectId);
            }

            if (TargetLevelAsset)
            {
                if (!TargetLevelAsset->IsValidCoord (Transition.TargetCellX, Transition.TargetCellY))
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("Transition target cell X=%d Y=%d is outside target level bounds."),
                            Transition.TargetCellX,
                            Transition.TargetCellY),
                        Obj.ObjectId);
                }
            }
            else if (!LevelAsset->IsValidCoord (Transition.TargetCellX, Transition.TargetCellY))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Transition target cell X=%d Y=%d is outside the current level bounds; target level bounds could not be validated."),
                        Transition.TargetCellX,
                        Transition.TargetCellY),
                    Obj.ObjectId);
            }
        }

        if (Obj.Type == EGridLevelObjectType::Receptacle)
        {
            const FGridReceptacleBehaviorParams& Receptacle = Obj.Behavior.Receptacle;

            if (!Receptacle.bAcceptAnyItem &&
                Receptacle.AcceptedItems.Num () == 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Receptacle does not accept any item because AcceptedItems is empty."),
                    Obj.ObjectId);
            }

            for (const FGridReceptacleAcceptedItemConfig& AcceptedItem : Receptacle.AcceptedItems)
            {
                if (!AcceptedItem.ItemDefinition)
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Warning,
                        TEXT ("Receptacle AcceptedItems contains an entry without an ItemDefinition."),
                        Obj.ObjectId);
                }
            }

            for (const FGridReceptacleInitialItemConfig& InitialItem : Receptacle.InitialContent)
            {
                if (!InitialItem.ItemDefinition)
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Warning,
                        TEXT ("Receptacle InitialContent contains an entry without an ItemDefinition."),
                        Obj.ObjectId);
                }
            }
        }
    }

    for (int32 ObjectIndex = 0; ObjectIndex < LevelAsset->Objects.Num (); ++ObjectIndex)
    {
        const FGridLevelObjectData& ObjectA = LevelAsset->Objects[ObjectIndex];
        const UGridObjectArchetypeAsset* ArchetypeA = FindObjectArchetypeById (ObjectA.ArchetypeId);
        if (!ArchetypeA || !LevelAsset->IsValidCoord (ObjectA.CellX, ObjectA.CellY))
        {
            continue;
        }

        const FString AnchorA = GetValidationAnchorKey (ObjectA);
        for (int32 OtherIndex = 0; OtherIndex < LevelAsset->Objects.Num (); ++OtherIndex)
        {
            if (ObjectIndex == OtherIndex)
            {
                continue;
            }

            const FGridLevelObjectData& ObjectB = LevelAsset->Objects[OtherIndex];
            if (ObjectA.CellX != ObjectB.CellX || ObjectA.CellY != ObjectB.CellY)
            {
                continue;
            }

            if (!ArchetypeA->bCanShareCell)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Object does not allow sharing its cell but another object is placed there."),
                    ObjectA.ObjectId);
                break;
            }

            if (!ArchetypeA->bCanShareAnchor && AnchorA == GetValidationAnchorKey (ObjectB))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (TEXT ("Object does not allow sharing anchor '%s' but another object uses it."), *AnchorA),
                    ObjectA.ObjectId);
                break;
            }
        }
    }

    TSet<FString> SeenLinkKeys;
    TMap<FString, uint8> DoorCommandDirectionsBySourceEvent;
    for (int32 LinkIndex = 0; LinkIndex < LevelAsset->Links.Num (); ++LinkIndex)
    {
        const FGridObjectLink& Link = LevelAsset->Links[LinkIndex];
        const FGridLevelObjectData* const* SourceObjectPtr = ObjectsById.Find (Link.SourceObjectId);
        const FGridLevelObjectData* const* TargetObjectPtr = ObjectsById.Find (Link.TargetObjectId);
        const FGridLevelObjectData* SourceObject = SourceObjectPtr ? *SourceObjectPtr : nullptr;
        const FGridLevelObjectData* TargetObject = TargetObjectPtr ? *TargetObjectPtr : nullptr;

        const FString LinkKey = FString::Printf (
            TEXT ("%s|%s|%d|%d|%d|%s|%s|%d|%d|%.9g|%d"),
            *Link.SourceObjectId.ToString (EGuidFormats::Digits),
            *Link.TargetObjectId.ToString (EGuidFormats::Digits),
            static_cast<int32> (Link.SourceEvent),
            static_cast<int32> (Link.Command),
            static_cast<int32> (Link.Condition),
            *Link.ConditionItemDefinitionId.ToString (),
            *Link.ConditionItemTag.ToString (),
            static_cast<int32> (Link.ConditionItemType),
            Link.ConditionCount,
            Link.ConditionWeight,
            Link.bInvertCondition ? 1 : 0);
        if (SeenLinkKeys.Contains (LinkKey))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d duplicates an identical link."), LinkIndex),
                Link.SourceObjectId);
        }
        else
        {
            SeenLinkKeys.Add (LinkKey);
        }

        if (!Link.SourceObjectId.IsValid ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d has an invalid SourceObjectId."), LinkIndex));
        }
        else if (!SourceObject)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d SourceObjectId was not found."), LinkIndex),
                Link.SourceObjectId);
        }
        else
        {
            int32& OutgoingCount = OutgoingLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
            ++OutgoingCount;

            if (!IsEventEmittedByCurrentRuntime (SourceObject->Type, Link.SourceEvent))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Link %d uses SourceEvent=%s, which is not emitted by the current C++ runtime for source type %s."),
                        LinkIndex,
                        *ToGridObjectEventText (Link.SourceEvent),
                        *ToGridObjectTypeText (SourceObject->Type)),
                    Link.SourceObjectId);
            }
            const bool bDisabledMonsterLifecycleSource =
                SourceObject->Type ==
                    EGridLevelObjectType::MonsterSpawn;
            if (!SourceObject->bInitiallyEnabled &&
                !bDisabledMonsterLifecycleSource)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (TEXT ("Link %d source object is initially disabled."), LinkIndex),
                    Link.SourceObjectId);
            }

            switch (Link.SourceEvent)
            {
                case EGridObjectEvent::ItemInserted:
                {
                    int32& EventCount = ReceptacleItemInsertedLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
                    ++EventCount;
                    break;
                }

                case EGridObjectEvent::ItemRemoved:
                {
                    int32& EventCount = ReceptacleItemRemovedLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
                    ++EventCount;
                    break;
                }

                case EGridObjectEvent::ItemChanged:
                {
                    int32& EventCount = ReceptacleItemChangedLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
                    ++EventCount;
                    break;
                }

                default:
                    break;
            }
        }

        if (!Link.TargetObjectId.IsValid ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d has an invalid TargetObjectId."), LinkIndex));
        }
        else if (!TargetObject)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d TargetObjectId was not found."), LinkIndex),
                Link.TargetObjectId);
        }
        else
        {
            if (!IsCommandSupportedByCurrentRuntime (TargetObject->Type, Link.Command))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Link %d command %s is not supported by the current runtime for target type %s."),
                        LinkIndex,
                        *ToGridObjectCommandText (Link.Command),
                        *ToGridObjectTypeText (TargetObject->Type)),
                    Link.TargetObjectId);
            }
            const bool bCommandCreatesDisabledMonster =
                TargetObject->Type == EGridLevelObjectType::MonsterSpawn &&
                (Link.Command == EGridObjectCommand::Spawn ||
                    Link.Command == EGridObjectCommand::Activate ||
                    Link.Command == EGridObjectCommand::Enable ||
                    Link.Command == EGridObjectCommand::Toggle ||
                    Link.Command == EGridObjectCommand::StartEncounter);
            if (!TargetObject->bInitiallyEnabled &&
                !bCommandCreatesDisabledMonster)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (TEXT ("Link %d target object is initially disabled and may have no spawned runtime actor."), LinkIndex),
                    Link.TargetObjectId);
            }

            if (TargetObject->Type ==
                    EGridLevelObjectType::MonsterSpawn &&
                Link.Command == EGridObjectCommand::StartEncounter &&
                TargetObject->EncounterGroupId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Link %d command Start Encounter requires a MonsterSpawn target with EncounterGroupId."),
                        LinkIndex),
                    Link.TargetObjectId);
            }

            if (TargetObject->Type == EGridLevelObjectType::Door)
            {
                const bool bOpensDoor =
                    Link.Command == EGridObjectCommand::Open ||
                    Link.Command == EGridObjectCommand::Activate;
                const bool bClosesDoor =
                    Link.Command == EGridObjectCommand::Close ||
                    Link.Command == EGridObjectCommand::Deactivate;
                if (bOpensDoor || bClosesDoor)
                {
                    const FString DoorCommandKey = FString::Printf (
                        TEXT ("%s|%s|%d"),
                        *Link.SourceObjectId.ToString (EGuidFormats::Digits),
                        *Link.TargetObjectId.ToString (EGuidFormats::Digits),
                        static_cast<int32> (Link.SourceEvent));
                    uint8& DirectionMask = DoorCommandDirectionsBySourceEvent.FindOrAdd (DoorCommandKey);
                    DirectionMask |= bOpensDoor ? 1 : 2;
                    if (DirectionMask == 3)
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Warning,
                            FString::Printf (
                                TEXT ("Link %d conflicts with another link: the same source event both opens and closes this door."),
                                LinkIndex),
                            Link.TargetObjectId);
                    }
                }
            }
        }

        if (Link.SourceObjectId.IsValid () && Link.SourceObjectId == Link.TargetObjectId)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                FString::Printf (TEXT ("Link %d targets its own source object."), LinkIndex),
                Link.SourceObjectId);
        }

        if (Link.Condition != EGridObjectCondition::None)
        {
            if (!TargetObject || TargetObject->Type != EGridLevelObjectType::Receptacle)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Link %d condition %s requires a receptacle target."),
                        LinkIndex,
                        *ToGridObjectConditionText (Link.Condition)),
                    Link.TargetObjectId);
            }

            switch (Link.Condition)
            {
                case EGridObjectCondition::ReceptacleContainsItemDefinition:
                    if (Link.ConditionItemDefinitionId.IsNone ())
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires ConditionItemDefinitionId."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleContainsItemTag:
                    if (Link.ConditionItemTag.IsNone ())
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires ConditionItemTag."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleContainsItemType:
                    if (Link.ConditionItemType == EGridItemType::None)
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires a non-None ConditionItemType."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleItemCountAtLeast:
                    if (Link.ConditionCount <= 0)
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires ConditionCount > 0."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleWeightAtLeast:
                    if (Link.ConditionWeight <= 0.0f)
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires ConditionWeight > 0."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleIsEmpty:
                case EGridObjectCondition::ReceptacleHasAnyItem:
                case EGridObjectCondition::None:
                default:
                    break;
            }
        }
    }

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.Type == EGridLevelObjectType::Trigger && !OutgoingLinkCountBySourceId.Contains (Obj.ObjectId))
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                TEXT ("Trigger has no outgoing links."),
                Obj.ObjectId);
        }

        if (Obj.Type == EGridLevelObjectType::Receptacle)
        {
            const int32 ItemInsertedCount = ReceptacleItemInsertedLinkCountBySourceId.FindRef (Obj.ObjectId);
            const int32 ItemRemovedCount = ReceptacleItemRemovedLinkCountBySourceId.FindRef (Obj.ObjectId);
            const int32 ItemChangedCount = ReceptacleItemChangedLinkCountBySourceId.FindRef (Obj.ObjectId);

            if (ItemInsertedCount == 0 && ItemRemovedCount == 0 && ItemChangedCount > 0)
            {
                continue;
            }

            if (ItemRemovedCount > 0 && ItemInsertedCount == 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Receptacle '%s' has ItemRemoved links but no ItemInserted links. This may be intentional, but the puzzle will not reset when an item is inserted again."),
                        *GetObjectValidationName (Obj)),
                    Obj.ObjectId);
            }

            if (ItemInsertedCount > 0 && ItemRemovedCount == 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Receptacle '%s' has ItemInserted links but no ItemRemoved links. This may be intentional, but the puzzle will not react when the item is removed."),
                        *GetObjectValidationName (Obj)),
                    Obj.ObjectId);
            }
        }
    }

    if (LastValidationMessages.Num () == 0)
    {
        AddMessage (
            EGridLevelValidationSeverity::Info,
            TEXT ("Validation complete: no issues found."));
    }

    for (FGridLevelValidationMessage& ValidationMessage : LastValidationMessages)
    {
        ValidationMessage.Category = InferValidationCategory (ValidationMessage.Message);

        if (ValidationMessage.Message.StartsWith (TEXT ("Link ")))
        {
            const int32 LinkIndex = FCString::Atoi (*ValidationMessage.Message.Mid (5));
            if (LevelAsset->Links.IsValidIndex (LinkIndex))
            {
                const FGridObjectLink& Link = LevelAsset->Links[LinkIndex];
                ValidationMessage.SourceObjectId = Link.SourceObjectId;
                ValidationMessage.TargetObjectId = Link.TargetObjectId;
            }
        }

        FGuid LocationObjectId = ValidationMessage.OptionalObjectId;
        if (!LocationObjectId.IsValid ())
        {
            LocationObjectId = ValidationMessage.SourceObjectId.IsValid ()
                ? ValidationMessage.SourceObjectId
                : ValidationMessage.TargetObjectId;
        }

        if (const FGridLevelObjectData* const* ObjectPtr = ObjectsById.Find (LocationObjectId))
        {
            ValidationMessage.CellX = (*ObjectPtr)->CellX;
            ValidationMessage.CellY = (*ObjectPtr)->CellY;
            ValidationMessage.Edge = (*ObjectPtr)->Edge;
        }
    }

    return LastValidationMessages;
}

