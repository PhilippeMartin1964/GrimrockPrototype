TArray<FGridLevelValidationMessage> AGridLevelEditorActor::ValidateCurrentLevel()
{
	LastValidationMessages.Reset();

	auto AddMessage = [this](EGridLevelValidationSeverity Severity, const FString& Message, const FGuid& OptionalObjectId = FGuid())
	{
		FGridLevelValidationMessage ValidationMessage;
		ValidationMessage.Severity = Severity;
		ValidationMessage.Message = Message;
		ValidationMessage.OptionalObjectId = OptionalObjectId;
		LastValidationMessages.Add(ValidationMessage);
	};

	auto AddDefinitionValidationMessages = [this, &AddMessage]()
	{
		if (!ObjectPalette)
		{
			return;
		}

		TSet<const UGridWorldObjectDefinitionAsset*> ValidatedDefinitions;

		TArray<FGridWorldObjectDefinitionValidationMessage> PaletteMessages;
		ObjectPalette->ValidatePalette(PaletteMessages);
		for (const FGridWorldObjectDefinitionValidationMessage& PaletteMessage : PaletteMessages)
		{
			AddMessage(ConvertDefinitionValidationSeverity(PaletteMessage.Severity), FString::Printf(TEXT("ObjectPalette: %s"), *PaletteMessage.Message));
		}

		for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
		{
			const UGridWorldObjectDefinitionAsset* Definition = Entry.DefaultWorldObjectDefinition.Get();
			if (!Definition)
			{
				continue;
			}

			const FString DefinitionName = Definition->DefinitionId.IsNone() ? Definition->GetName() : Definition->DefinitionId.ToString();

			if (ValidatedDefinitions.Contains(Definition))
			{
				continue;
			}

			ValidatedDefinitions.Add(Definition);

			TArray<FGridWorldObjectDefinitionValidationMessage> DefinitionMessages;
			Definition->ValidateDefinition(DefinitionMessages);

			for (const FGridWorldObjectDefinitionValidationMessage& DefinitionMessage : DefinitionMessages)
			{
				AddMessage(ConvertDefinitionValidationSeverity(DefinitionMessage.Severity),
					FString::Printf(TEXT("Definition %s: %s"), *DefinitionName, *DefinitionMessage.Message));
			}
		}
	};

	auto AddExpectedConcreteDefinitionMessages = [this, &AddMessage]()
	{
		if (!ObjectPalette)
		{
			return;
		}

		for (const FExpectedConcreteDefinitionSpec& ExpectedSpec : ExpectedConcreteDefinitions)
		{
			const FName ExpectedWorldObjectDefinitionId(ExpectedSpec.WorldObjectDefinitionId);
			const FGridObjectPaletteEntry* MatchingEntry = nullptr;

			for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
			{
				if (Entry.GetEffectiveWorldObjectDefinitionId() == ExpectedWorldObjectDefinitionId)
				{
					MatchingEntry = &Entry;
					break;
				}
			}

			if (!MatchingEntry)
			{
				AddMessage(EGridLevelValidationSeverity::Warning,
					FString::Printf(
						TEXT(
							"ObjectPalette should expose concrete definition '%s'. Variants must be palette entries/definitions, not new EGridLevelObjectType values."),
						ExpectedSpec.WorldObjectDefinitionId));
				continue;
			}

			const EGridLevelObjectType EffectiveType = MatchingEntry->GetEffectiveObjectType();
			if (EffectiveType != ExpectedSpec.ExpectedType)
			{
				AddMessage(EGridLevelValidationSeverity::Error,
					FString::Printf(TEXT("ObjectPalette definition '%s' should use Type=%s, but currently uses Type=%s."), ExpectedSpec.WorldObjectDefinitionId,
						*ToGridObjectTypeText(ExpectedSpec.ExpectedType), *ToGridObjectTypeText(EffectiveType)));
			}
		}
	};

	if (!DungeonAsset)
	{
		AddMessage(EGridLevelValidationSeverity::Warning,
			TEXT("DungeonAsset is missing. The editor can use LevelAsset directly, but dungeon level ids and transitions cannot be fully validated."));
	}
	else
	{
		TSet<FName> SeenLevelIds;
		TSet<FIntVector> SeenLogicalPositions;
		bool bHasEnabledFallbackLevel = false;

		if (DungeonAsset->Levels.Num() == 0)
		{
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("DungeonAsset contains no level entries."));
		}

		for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
		{
			if (Entry.LevelId.IsNone())
			{
				AddMessage(EGridLevelValidationSeverity::Error, TEXT("DungeonAsset contains a level entry with an empty LevelId."));
			}
			else if (SeenLevelIds.Contains(Entry.LevelId))
			{
				AddMessage(
					EGridLevelValidationSeverity::Error, FString::Printf(TEXT("DungeonAsset contains duplicate LevelId '%s'."), *Entry.LevelId.ToString()));
			}
			SeenLevelIds.Add(Entry.LevelId);

			if (SeenLogicalPositions.Contains(Entry.LogicalPosition))
			{
				AddMessage(EGridLevelValidationSeverity::Error,
					FString::Printf(TEXT("DungeonAsset contains duplicate LogicalPosition (%d,%d,%d)."), Entry.LogicalPosition.X, Entry.LogicalPosition.Y,
						Entry.LogicalPosition.Z));
			}
			SeenLogicalPositions.Add(Entry.LogicalPosition);

			if (!Entry.LevelAsset)
			{
				AddMessage(Entry.bEnabled ? EGridLevelValidationSeverity::Error : EGridLevelValidationSeverity::Warning,
					FString::Printf(TEXT("Dungeon level '%s' has no LevelAsset."), Entry.LevelId.IsNone() ? TEXT("None") : *Entry.LevelId.ToString()));
			}
			else if (Entry.bEnabled && !Entry.LevelId.IsNone())
			{
				bHasEnabledFallbackLevel = true;
			}
		}

		if (!DungeonAsset->IsValidLevelId(DungeonAsset->DefaultLevelId))
		{
			AddMessage(bHasEnabledFallbackLevel ? EGridLevelValidationSeverity::Warning : EGridLevelValidationSeverity::Error,
				bHasEnabledFallbackLevel ? TEXT("DefaultLevelId is invalid; runtime/editor fallback will use the first enabled level with a LevelAsset.")
										 : TEXT("DefaultLevelId is invalid and no enabled fallback level with a LevelAsset exists."));
		}
	}

	AddDefinitionValidationMessages();
	AddExpectedConcreteDefinitionMessages();

	if (!LevelAsset)
	{
		AddMessage(EGridLevelValidationSeverity::Error, TEXT("LevelAsset is missing."));
		return LastValidationMessages;
	}

	if (LevelAsset->Width <= 0)
	{
		AddMessage(EGridLevelValidationSeverity::Error, TEXT("LevelAsset Width must be greater than zero."));
	}
	if (LevelAsset->Height <= 0)
	{
		AddMessage(EGridLevelValidationSeverity::Error, TEXT("LevelAsset Height must be greater than zero."));
	}
	if (LevelAsset->CellSize <= 0.f)
	{
		AddMessage(EGridLevelValidationSeverity::Error, TEXT("LevelAsset CellSize must be greater than zero."));
	}

	const int32 ExpectedCellCount = FMath::Max(1, LevelAsset->Width) * FMath::Max(1, LevelAsset->Height);
	if (LevelAsset->Cells.Num() != ExpectedCellCount)
	{
		AddMessage(EGridLevelValidationSeverity::Error,
			FString::Printf(TEXT("LevelAsset Cells.Num()=%d but expected %d for Width=%d Height=%d."), LevelAsset->Cells.Num(), ExpectedCellCount,
				LevelAsset->Width, LevelAsset->Height));
	}

	if (!LevelAsset->IsStartCellValid())
	{
		AddMessage(EGridLevelValidationSeverity::Error,
			FString::Printf(TEXT("Start cell X=%d Y=%d Facing=%s is invalid. It must be inside the grid, non-empty and not block occupancy."),
				LevelAsset->StartCellX, LevelAsset->StartCellY, *GetGridEdgeText(LevelAsset->StartFacing)));
	}

	if (LevelAsset->Width > 0 && LevelAsset->Height > 0 && LevelAsset->Cells.Num() == ExpectedCellCount)
	{
		int32 OverlappingSharedWallCount = 0;
		int32 DirectionalSharedWallCount = 0;
		FString FirstOverlappingSharedWall;
		FString FirstDirectionalSharedWall;

		for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
		{
			for (int32 X = 0; X < LevelAsset->Width; ++X)
			{
				const FGridLevelCellData& Cell = LevelAsset->GetCell(X, Y);

				auto ValidateSharedEdge = [&OverlappingSharedWallCount, &DirectionalSharedWallCount, &FirstOverlappingSharedWall, &FirstDirectionalSharedWall,
											  X,
											  Y](const TCHAR* EdgeName, EGridWallType LocalWall, EGridWallType OppositeWall, int32 NeighborX, int32 NeighborY)
				{
					if (LocalWall != EGridWallType::None && OppositeWall != EGridWallType::None)
					{
						++OverlappingSharedWallCount;
						if (FirstOverlappingSharedWall.IsEmpty())
						{
							FirstOverlappingSharedWall = FString::Printf(TEXT("%s between (%d,%d) and (%d,%d)"), EdgeName, X, Y, NeighborX, NeighborY);
						}
					}
					else if (LocalWall != OppositeWall)
					{
						++DirectionalSharedWallCount;
						if (FirstDirectionalSharedWall.IsEmpty())
						{
							FirstDirectionalSharedWall = FString::Printf(TEXT("%s between (%d,%d) and (%d,%d), %s vs %s"), EdgeName, X, Y, NeighborX, NeighborY,
								*UEnum::GetValueAsString(LocalWall), *UEnum::GetValueAsString(OppositeWall));
						}
					}
				};

				if (X + 1 < LevelAsset->Width)
				{
					ValidateSharedEdge(TEXT("East/West"), Cell.EastWall, LevelAsset->GetCell(X + 1, Y).WestWall, X + 1, Y);
				}
				if (Y + 1 < LevelAsset->Height)
				{
					ValidateSharedEdge(TEXT("North/South"), Cell.NorthWall, LevelAsset->GetCell(X, Y + 1).SouthWall, X, Y + 1);
				}
			}
		}

		if (OverlappingSharedWallCount > 0)
		{
			AddMessage(EGridLevelValidationSeverity::Warning,
				FString::Printf(TEXT("%d shared edges have walls on both sides; runtime rendering may create overlapping wall instances. First: %s."),
					OverlappingSharedWallCount, *FirstOverlappingSharedWall));
		}
		if (DirectionalSharedWallCount > 0)
		{
			AddMessage(EGridLevelValidationSeverity::Warning,
				FString::Printf(TEXT("%d shared edges are directional; movement depends on the source cell. First: %s."), DirectionalSharedWallCount,
					*FirstDirectionalSharedWall));
		}
	}

	TSet<FGuid> SeenObjectIds;
	TMap<FGuid, int32> OutgoingLinkCountBySourceId;
	TMap<FGuid, int32> ReceptacleItemInsertedLinkCountBySourceId;
	TMap<FGuid, int32> ReceptacleItemRemovedLinkCountBySourceId;
	TMap<FGuid, int32> ReceptacleItemChangedLinkCountBySourceId;
	TMap<FIntPoint, FGuid> EnabledMonsterSpawnByCell;
	TMap<FName, TMap<int32, TMap<FIntPoint, FGuid>>> EncounterMonsterSpawnByWaveAndCell;

	const auto ValidatePlacement = [this, &AddMessage, &SeenObjectIds](FGuid ObjectId, int32 CellX, int32 CellY, FName PaletteEntryId, EGridLevelObjectType Type)
	{
		if (!ObjectId.IsValid())
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Object at X=%d Y=%d has an invalid ObjectId."), CellX, CellY));
		else if (SeenObjectIds.Contains(ObjectId))
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("Duplicate ObjectId found."), ObjectId);
		else
			SeenObjectIds.Add(ObjectId);
		if (ObjectPalette && !PaletteEntryId.IsNone())
		{
			const FGridObjectPaletteEntry* Entry = ObjectPalette->FindEntryById(PaletteEntryId);
			if (!Entry)
				AddMessage(EGridLevelValidationSeverity::Warning, FString::Printf(TEXT("Placed object PaletteEntryId '%s' no longer exists in the assigned ObjectPalette."), *PaletteEntryId.ToString()), ObjectId);
			else if (Entry->GetEffectiveObjectType() != Type)
				AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Placed object type does not match its PaletteEntryId."), ObjectId);
		}
		if (!LevelAsset->IsValidCoord(CellX, CellY))
		{
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Object is outside grid bounds at X=%d Y=%d."), CellX, CellY), ObjectId);
			return false;
		}
		if (!LevelAsset->Cells.IsValidIndex(LevelAsset->GetIndex(CellX, CellY)))
		{
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("Object cannot resolve its cell because the level cell array is incomplete."), ObjectId);
			return false;
		}
		return true;
	};
	const auto IsInitiallyEnabled = [this](FGuid ObjectId)
	{
		if (const FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(ObjectId)) return WorldObjectInstance->bInitiallyEnabled;
		if (const FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(ObjectId)) return LooseItemInstance->bInitiallyEnabled;
		if (const FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset->FindMonsterSpawnInstanceById(ObjectId)) return MonsterSpawn->bInitiallyEnabled;
		if (const FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(ObjectId)) return ItemSpawn->bInitiallyEnabled;
		if (const FGridLogicObjectInstance* LogicInstance = LevelAsset->FindLogicObjectInstanceById(ObjectId)) return LogicInstance->bInitiallyEnabled;
		return false;
	};
	const auto GetValidationAnchorKey = [this](FGuid ObjectId)
	{
		int32 CellX = 0, CellY = 0;
		EGridEdge Edge = EGridEdge::None;
		LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge);
		return IsEdgePlacedObject(ObjectId) && Edge != EGridEdge::None ? GetGridEdgeText(Edge) : FString(TEXT("Center"));
	};
	const auto GetObjectValidationName = [](const FGridWorldObjectInstance& WorldObjectInstance)
	{
		if (!WorldObjectInstance.LogicId.IsNone()) return WorldObjectInstance.LogicId.ToString();
		if (!WorldObjectInstance.WorldObjectDefinitionId.IsNone()) return WorldObjectInstance.WorldObjectDefinitionId.ToString();
		return WorldObjectInstance.InstanceId.IsValid() ? WorldObjectInstance.InstanceId.ToString().Left(8) : FString(TEXT("InvalidObjectId"));
	};
	const auto IsCardinal = [](EGridEdge Edge)
	{
		return Edge == EGridEdge::North || Edge == EGridEdge::East || Edge == EGridEdge::South || Edge == EGridEdge::West;
	};

	for (const FGridWorldObjectInstance& Obj : LevelAsset->WorldObjectInstances)
	{
		const FGuid ObjectId = Obj.InstanceId;
		if (!ValidatePlacement(ObjectId, Obj.CellX, Obj.CellY, Obj.PaletteEntryId, Obj.Type)) continue;
		const UGridWorldObjectDefinitionAsset* Definition = FindWorldObjectDefinitionById(Obj.WorldObjectDefinitionId);
		if (Obj.WorldObjectDefinitionId.IsNone())
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("Placed world object has no WorldObjectDefinitionId."), ObjectId);
		else if (!Definition)
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Placed object definition '%s' cannot be resolved."), *Obj.WorldObjectDefinitionId.ToString()), ObjectId);
		if (GridLevelPlacement::GetBucket(Obj.Type) != EGridLevelPlacementBucket::WorldObject)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("WorldObjectInstances contains a type belonging to another placement collection."), ObjectId);
		if (Definition && Obj.Type != Definition->SupportedType)
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Placed object Type=%s does not match definition '%s' SupportedType=%s."), *ToGridObjectTypeText(Obj.Type), *Obj.WorldObjectDefinitionId.ToString(), *ToGridObjectTypeText(Definition->SupportedType)), ObjectId);
		if (ObjectPalette && !Obj.PaletteEntryId.IsNone())
		{
			const FGridObjectPaletteEntry* Entry = ObjectPalette->FindEntryById(Obj.PaletteEntryId);
			if (Entry && Entry->GetEffectiveWorldObjectDefinitionId() != Obj.WorldObjectDefinitionId)
				AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Placed object PaletteEntryId now resolves to a different world object definition."), ObjectId);
		}
		if (Definition)
		{
			if (Definition->bIsReadable)
			{
				if (Obj.ReadableTextOverride.IsEmpty() && Definition->ReadableText.IsEmpty())
					AddMessage(EGridLevelValidationSeverity::Warning, Obj.Notes.IsEmpty() ? TEXT("Readable placed object has no text in either its instance override or definition.") : TEXT("Readable placed object has no text. Notes are editor-only and are not displayed at runtime."), ObjectId);
				if (!Obj.bInitiallyEnabled)
					AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Readable placed object is initially disabled and cannot be read until enabled."), ObjectId);
			}
			else if (!Obj.ReadableTextOverride.IsEmpty())
				AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Placed object has a readable-text override, but its definition is not readable; the override is ignored at runtime."), ObjectId);
			if ((Definition->PlacementSurface == EGridObjectPlacementKind::Floor || Definition->PlacementSurface == EGridObjectPlacementKind::Ceiling) && Obj.WallSide != EGridEdge::None)
				AddMessage(EGridLevelValidationSeverity::Warning, FString::Printf(TEXT("Center-placed object has a cardinal Edge=%s; runtime center placement ignores this edge."), *GetGridEdgeText(Obj.WallSide)), ObjectId);
			if (Definition->bBlocksMovement && LevelAsset->GetCell(Obj.CellX, Obj.CellY).bBlocksOccupancy)
				AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Object blocks movement on a cell that already blocks occupancy."), ObjectId);
		}
		if (IsEdgePlacedObject(ObjectId) && !IsCardinal(Obj.WallSide))
			AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Edge or wall placed object requires a cardinal edge."), ObjectId);
		if (Obj.Type == EGridLevelObjectType::Door)
		{
			if (GetWallTypeForEdge(LevelAsset->GetCell(Obj.CellX, Obj.CellY), Obj.WallSide) == EGridWallType::Solid)
				AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Door is placed on an edge whose wall is Solid. A door edge must use WallType=None."), ObjectId);
			int32 NeighborX = Obj.CellX, NeighborY = Obj.CellY;
			switch (Obj.WallSide)
			{
				case EGridEdge::North: ++NeighborY; break;
				case EGridEdge::East: ++NeighborX; break;
				case EGridEdge::South: --NeighborY; break;
				case EGridEdge::West: --NeighborX; break;
				default: break;
			}
			if (Obj.WallSide != EGridEdge::None && !LevelAsset->IsValidCoord(NeighborX, NeighborY))
				AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Door is placed on an outer grid edge with no neighboring cell to cross."), ObjectId);
		}
		const bool bIsPit = Obj.Type == EGridLevelObjectType::Pit;
		const FGridObjectTransitionParams& Transition = Obj.InstanceConfig.Transition;
		if (bIsPit || Transition.bIsTransition)
		{
			const bool bSameCell = bIsPit && Obj.InstanceConfig.Pit.bUseSameCellCoordinates;
			const int32 TargetX = bSameCell ? Obj.CellX : Transition.TargetCellX;
			const int32 TargetY = bSameCell ? Obj.CellY : Transition.TargetCellY;
			FName TargetLevelId = Transition.TargetLevelId;
			bool bAutoResolved = false;
			if (bIsPit && DungeonAsset && (TargetLevelId.IsNone() || !DungeonAsset->IsValidLevelId(TargetLevelId)))
			{
				const FGridDungeonLevelEntry* CurrentEntry = DungeonAsset->Levels.FindByPredicate([this](const FGridDungeonLevelEntry& Entry){ return Entry.LevelAsset.Get() == LevelAsset; });
				const FGridDungeonLevelEntry* LowerLevel = CurrentEntry ? DungeonAsset->FindLevelBelow(CurrentEntry->LevelId) : nullptr;
				if (LowerLevel) { TargetLevelId = LowerLevel->LevelId; bAutoResolved = true; }
			}
			if (TargetLevelId.IsNone())
				AddMessage(EGridLevelValidationSeverity::Error, bIsPit ? TEXT("Pit has no enabled dungeon level below it.") : TEXT("Transition has no TargetLevelId."), ObjectId);
			if (!bIsPit && !IsCardinal(Transition.TargetFacing))
				AddMessage(EGridLevelValidationSeverity::Error, TEXT("Transition TargetFacing must be cardinal."), ObjectId);
			const UGridLevelAsset* TargetLevel = DungeonAsset ? DungeonAsset->GetLevelAssetById(TargetLevelId) : nullptr;
			if (!DungeonAsset)
				AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Transition cannot validate TargetLevelId because DungeonAsset is null."), ObjectId);
			else if (!TargetLevelId.IsNone() && !TargetLevel)
				AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Transition target LevelId '%s' was not found as an enabled level with a LevelAsset in the DungeonAsset."), *TargetLevelId.ToString()), ObjectId);
			else if (bAutoResolved && !Transition.TargetLevelId.IsNone())
				AddMessage(EGridLevelValidationSeverity::Warning, FString::Printf(TEXT("Pit explicit TargetLevelId '%s' is unavailable; runtime will fall to automatic lower level '%s'."), *Transition.TargetLevelId.ToString(), *TargetLevelId.ToString()), ObjectId);
			if (TargetLevel && !TargetLevel->IsValidCoord(TargetX, TargetY))
				AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Transition target cell X=%d Y=%d is outside target level bounds."), TargetX, TargetY), ObjectId);
			else if (!TargetLevel && !LevelAsset->IsValidCoord(TargetX, TargetY))
				AddMessage(EGridLevelValidationSeverity::Warning, FString::Printf(TEXT("Transition target cell X=%d Y=%d is outside the current level bounds; target level bounds could not be validated."), TargetX, TargetY), ObjectId);
			if (bIsPit && Obj.InstanceConfig.Pit.bInitiallyOpen && TargetLevel && TargetLevel->IsValidCoord(TargetX, TargetY))
			{
				const bool bOpenPitAtDestination = TargetLevel->WorldObjectInstances.ContainsByPredicate([TargetX, TargetY](const FGridWorldObjectInstance& Candidate)
				{
					return Candidate.Type == EGridLevelObjectType::Pit && Candidate.CellX == TargetX && Candidate.CellY == TargetY && Candidate.bInitiallyEnabled && Candidate.InstanceConfig.Pit.bInitiallyOpen;
				});
				if (bOpenPitAtDestination)
					AddMessage(EGridLevelValidationSeverity::Error, TEXT("PIT01 destination contains another initially open pit; chained falls are not supported yet."), ObjectId);
			}
		}
		if (Obj.Type == EGridLevelObjectType::Receptacle)
		{
			if (Definition)
			{
				const FGridReceptacleBehaviorParams& Receptacle = Definition->DefaultBehavior.Receptacle;
				if (!Receptacle.bAcceptAnyItem && Receptacle.AcceptedItems.IsEmpty())
					AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Receptacle does not accept any item because AcceptedItems is empty."), ObjectId);
				for (const FGridReceptacleAcceptedItemConfig& AcceptedItem : Receptacle.AcceptedItems)
					if (!AcceptedItem.ItemDefinition)
						AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Receptacle AcceptedItems contains an entry without an ItemDefinition."), ObjectId);
			}
			for (const FGridReceptacleInitialItemConfig& InitialItem : Obj.InstanceConfig.ReceptacleInitialContent)
			{
				if (!InitialItem.ItemDefinition)
					AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Receptacle InitialContent contains an entry without an ItemDefinition."), ObjectId);
				if (InitialItem.Quantity < 1)
					AddMessage(EGridLevelValidationSeverity::Error, TEXT("Receptacle InitialContent requires Quantity >= 1."), ObjectId);
			}
		}
		if (Definition)
		{
			const FString Anchor = GetValidationAnchorKey(ObjectId);
			for (FGuid OtherId : LevelAsset->GetTypedPlacementIdsAtCell(Obj.CellX, Obj.CellY))
			{
				if (OtherId == ObjectId) continue;
				if (!Definition->bCanShareCell)
				{
					AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Object does not allow sharing its cell but another object is placed there."), ObjectId);
					break;
				}
				if (!Definition->bCanShareAnchor && Anchor == GetValidationAnchorKey(OtherId))
				{
					AddMessage(EGridLevelValidationSeverity::Warning, FString::Printf(TEXT("Object does not allow sharing anchor '%s' but another object uses it."), *Anchor), ObjectId);
					break;
				}
			}
		}
	}

	const auto ValidateItemDefinition = [&AddMessage](FGuid ObjectId, const UGridItemDefinitionAsset* Definition, int32 Quantity)
	{
		if (!Definition)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("Item placement requires an ItemDefinition asset."), ObjectId);
		else if (Definition->ItemDefinitionId.IsNone())
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("Placed item references an item definition asset whose ItemDefinitionId is empty."), ObjectId);
		if (Quantity < 1)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("Item placement requires Quantity >= 1."), ObjectId);
	};
	for (const FGridLooseItemInstance& LooseItemInstance : LevelAsset->LooseItemInstances)
	{
		const bool bValidCell = ValidatePlacement(LooseItemInstance.InstanceId, LooseItemInstance.CellX, LooseItemInstance.CellY, LooseItemInstance.PaletteEntryId, EGridLevelObjectType::Item);
		ValidateItemDefinition(LooseItemInstance.InstanceId, LooseItemInstance.ItemDefinition, LooseItemInstance.Quantity);
		if (bValidCell)
		{
			const FGridLevelCellData& Cell = LevelAsset->GetCell(LooseItemInstance.CellX, LooseItemInstance.CellY);
			if (Cell.CellType == EGridCellType::Empty || Cell.bBlocksOccupancy)
				AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Placed item is on a non-walkable cell; normal party pickup cannot reach this cell."), LooseItemInstance.InstanceId);
		}
	}
	for (const FGridItemSpawnInstance& ItemSpawn : LevelAsset->ItemSpawns)
	{
		ValidatePlacement(ItemSpawn.SpawnId, ItemSpawn.CellX, ItemSpawn.CellY, ItemSpawn.PaletteEntryId, EGridLevelObjectType::ItemSpawn);
		ValidateItemDefinition(ItemSpawn.SpawnId, ItemSpawn.ItemDefinition, ItemSpawn.Quantity);
	}
	for (const FGridMonsterSpawnInstance& MonsterSpawn : LevelAsset->MonsterSpawns)
	{
		const FGuid ObjectId = MonsterSpawn.SpawnId;
		const bool bValidCell = ValidatePlacement(ObjectId, MonsterSpawn.CellX, MonsterSpawn.CellY, MonsterSpawn.PaletteEntryId, EGridLevelObjectType::MonsterSpawn);
		if (!MonsterSpawn.MonsterDefinition)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("MonsterSpawn requires MonsterDefinition."), ObjectId);
		else
		{
			FString DefinitionError;
			if (!MonsterSpawn.MonsterDefinition->ValidateDefinition(DefinitionError))
				AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("MonsterSpawn references an invalid MonsterDefinition: %s"), *DefinitionError), ObjectId);
		}
		if (!IsCardinal(MonsterSpawn.Facing))
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("MonsterSpawn requires a cardinal InitialFacing."), ObjectId);
		if (MonsterSpawn.InitialMonsterState != EGridMonsterState::Idle && MonsterSpawn.InitialMonsterState != EGridMonsterState::Dormant)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("MonsterSpawn requires InitialMonsterState Idle or Dormant."), ObjectId);
		if (bValidCell)
		{
			const FGridLevelCellData& Cell = LevelAsset->GetCell(MonsterSpawn.CellX, MonsterSpawn.CellY);
			if (Cell.CellType == EGridCellType::Empty || Cell.bBlocksOccupancy)
				AddMessage(EGridLevelValidationSeverity::Error, TEXT("MonsterSpawn must be placed on a non-empty cell that allows occupancy."), ObjectId);
			const FIntPoint CellKey(MonsterSpawn.CellX, MonsterSpawn.CellY);
			if (MonsterSpawn.bInitiallyEnabled)
			{
				if (const FGuid* ExistingId = EnabledMonsterSpawnByCell.Find(CellKey))
					AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("MonsterSpawn shares its initial cell with enabled MonsterSpawn %s."), *ExistingId->ToString()), ObjectId);
				else EnabledMonsterSpawnByCell.Add(CellKey, ObjectId);
			}
			if (!MonsterSpawn.EncounterGroupId.IsNone() && MonsterSpawn.EncounterWaveIndex >= 0)
			{
				TMap<FIntPoint, FGuid>& SpawnByCell = EncounterMonsterSpawnByWaveAndCell.FindOrAdd(MonsterSpawn.EncounterGroupId).FindOrAdd(MonsterSpawn.EncounterWaveIndex);
				if (const FGuid* ExistingId = SpawnByCell.Find(CellKey))
					AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("MonsterSpawn shares encounter wave %d cell with MonsterSpawn %s in encounter '%s'."), MonsterSpawn.EncounterWaveIndex, *ExistingId->ToString(), *MonsterSpawn.EncounterGroupId.ToString()), ObjectId);
				else SpawnByCell.Add(CellKey, ObjectId);
			}
		}
		if (MonsterSpawn.EncounterWaveIndex < 0)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("MonsterSpawn requires EncounterWaveIndex >= 0."), ObjectId);
		if (MonsterSpawn.EncounterGroupId.IsNone() && MonsterSpawn.EncounterWaveIndex > 0)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("MonsterSpawn requires EncounterGroupId when EncounterWaveIndex is greater than 0."), ObjectId);
		if (!MonsterSpawn.EncounterGroupId.IsNone() && MonsterSpawn.EncounterWaveIndex > 0 && MonsterSpawn.bInitiallyEnabled)
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("MonsterSpawn belongs to future encounter wave %d and must be disabled at start."), MonsterSpawn.EncounterWaveIndex), ObjectId);
		if (MonsterSpawn.PatrolMode != EGridMonsterPatrolMode::None && MonsterSpawn.PatrolWaypoints.Num() < 2)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("MonsterSpawn patrol mode requires at least two waypoints."), ObjectId);
		for (int32 WaypointIndex = 0; WaypointIndex < MonsterSpawn.PatrolWaypoints.Num(); ++WaypointIndex)
		{
			const FGridMonsterPatrolWaypoint& Waypoint = MonsterSpawn.PatrolWaypoints[WaypointIndex];
			const bool bValidWaypoint = LevelAsset->IsValidCoord(Waypoint.Cell.X, Waypoint.Cell.Y) && LevelAsset->Cells.IsValidIndex(LevelAsset->GetIndex(Waypoint.Cell.X, Waypoint.Cell.Y));
			if (!bValidWaypoint)
				AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("MonsterSpawn patrol waypoint %d is outside grid bounds or has no cell."), WaypointIndex), ObjectId);
			else
			{
				const FGridLevelCellData& Cell = LevelAsset->GetCell(Waypoint.Cell.X, Waypoint.Cell.Y);
				if (Cell.CellType == EGridCellType::Empty || Cell.bBlocksOccupancy)
					AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("MonsterSpawn patrol waypoint %d must use a non-empty cell that allows occupancy."), WaypointIndex), ObjectId);
			}
			if (Waypoint.Facing != EGridEdge::None && !IsCardinal(Waypoint.Facing))
				AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("MonsterSpawn patrol waypoint %d requires Facing=None or a cardinal direction."), WaypointIndex), ObjectId);
			if (!FMath::IsFinite(Waypoint.WaitSeconds) || Waypoint.WaitSeconds < 0.f)
				AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("MonsterSpawn patrol waypoint %d requires a finite non-negative WaitSeconds."), WaypointIndex), ObjectId);
		}
	}
	for (const FGridLogicObjectInstance& LogicInstance : LevelAsset->LogicObjects)
	{
		ValidatePlacement(LogicInstance.InstanceId, LogicInstance.CellX, LogicInstance.CellY, LogicInstance.PaletteEntryId, LogicInstance.Type);
		if (GridLevelPlacement::GetBucket(LogicInstance.Type) != EGridLevelPlacementBucket::LogicObject)
			AddMessage(EGridLevelValidationSeverity::Error, TEXT("LogicObjects contains a type belonging to another placement collection."), LogicInstance.InstanceId);
	}

	TSet<FString> SeenLinkKeys;
	TMap<FString, uint8> DoorCommandDirectionsBySourceEvent;
	for (int32 LinkIndex = 0; LinkIndex < LevelAsset->Links.Num(); ++LinkIndex)
	{
		const FGridObjectLink& Link = LevelAsset->Links[LinkIndex];
		const bool bHasSource = LevelAsset->ContainsTypedPlacementId(Link.SourceObjectId);
		const bool bHasTarget = LevelAsset->ContainsTypedPlacementId(Link.TargetObjectId);
		const EGridLevelObjectType SourceType = LevelAsset->GetTypedPlacementType(Link.SourceObjectId);
		const EGridLevelObjectType TargetType = LevelAsset->GetTypedPlacementType(Link.TargetObjectId);
		const FGridLogicObjectInstance* SourceLogic = LevelAsset->FindLogicObjectInstanceById(Link.SourceObjectId);
		const FGridLogicObjectInstance* TargetLogic = LevelAsset->FindLogicObjectInstanceById(Link.TargetObjectId);
		const EGridLogicNodeType SourceNodeType = SourceLogic ? SourceLogic->Logic.NodeType : EGridLogicNodeType::Relay;
		const EGridLogicNodeType TargetNodeType = TargetLogic ? TargetLogic->Logic.NodeType : EGridLogicNodeType::Relay;

		const FString LinkKey = FString::Printf(TEXT("%s|%s|%d|%d|%d|%s|%s|%d|%d|%.9g|%d"), *Link.SourceObjectId.ToString(EGuidFormats::Digits),
			*Link.TargetObjectId.ToString(EGuidFormats::Digits), static_cast<int32>(Link.SourceEvent), static_cast<int32>(Link.Command),
			static_cast<int32>(Link.Condition), *Link.ConditionItemDefinitionId.ToString(), *Link.ConditionItemTag.ToString(),
			static_cast<int32>(Link.ConditionItemType), Link.ConditionCount, Link.ConditionWeight, Link.bInvertCondition ? 1 : 0);
		if (SeenLinkKeys.Contains(LinkKey))
		{
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Link %d duplicates an identical link."), LinkIndex), Link.SourceObjectId);
		}
		else
		{
			SeenLinkKeys.Add(LinkKey);
		}

		if (!Link.SourceObjectId.IsValid())
		{
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Link %d has an invalid SourceObjectId."), LinkIndex));
		}
		else if (!bHasSource)
		{
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Link %d SourceObjectId was not found."), LinkIndex), Link.SourceObjectId);
		}
		else
		{
			int32& OutgoingCount = OutgoingLinkCountBySourceId.FindOrAdd(Link.SourceObjectId);
			++OutgoingCount;

			if (!GridEditorLinkPolicy::GetSupportedEventsForSource(SourceType, SourceNodeType).Contains(Link.SourceEvent))
			{
				AddMessage(EGridLevelValidationSeverity::Warning,
					FString::Printf(TEXT("Link %d uses SourceEvent=%s, which is not emitted by the current C++ runtime for source type %s."), LinkIndex,
						*ToGridObjectEventText(Link.SourceEvent), *ToGridObjectTypeText(SourceType)),
					Link.SourceObjectId);
			}
			const bool bDisabledMonsterLifecycleSource = SourceType == EGridLevelObjectType::MonsterSpawn;
			if (!IsInitiallyEnabled(Link.SourceObjectId) && !bDisabledMonsterLifecycleSource)
			{
				AddMessage(EGridLevelValidationSeverity::Warning, FString::Printf(TEXT("Link %d source object is initially disabled."), LinkIndex),
					Link.SourceObjectId);
			}

			switch (Link.SourceEvent)
			{
				case EGridObjectEvent::ItemInserted:
				{
					int32& EventCount = ReceptacleItemInsertedLinkCountBySourceId.FindOrAdd(Link.SourceObjectId);
					++EventCount;
					break;
				}

				case EGridObjectEvent::ItemRemoved:
				{
					int32& EventCount = ReceptacleItemRemovedLinkCountBySourceId.FindOrAdd(Link.SourceObjectId);
					++EventCount;
					break;
				}

				case EGridObjectEvent::ItemChanged:
				{
					int32& EventCount = ReceptacleItemChangedLinkCountBySourceId.FindOrAdd(Link.SourceObjectId);
					++EventCount;
					break;
				}

				default:
					break;
			}
		}

		if (!Link.TargetObjectId.IsValid())
		{
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Link %d has an invalid TargetObjectId."), LinkIndex));
		}
		else if (!bHasTarget)
		{
			AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Link %d TargetObjectId was not found."), LinkIndex), Link.TargetObjectId);
		}
		else
		{
			if (GridEditorLinkPolicy::GetCommandRuntimeSupport(TargetType, TargetNodeType, Link.Command) != EGridEditorCommandRuntimeSupport::Gameplay)
			{
				AddMessage(EGridLevelValidationSeverity::Error,
					FString::Printf(TEXT("Link %d command %s is not supported by the current runtime for target type %s."), LinkIndex,
						*ToGridObjectCommandText(Link.Command), *ToGridObjectTypeText(TargetType)),
					Link.TargetObjectId);
			}
			const bool bCommandCreatesDisabledMonster = TargetType == EGridLevelObjectType::MonsterSpawn &&
				(Link.Command == EGridObjectCommand::Spawn || Link.Command == EGridObjectCommand::Activate || Link.Command == EGridObjectCommand::Enable ||
					Link.Command == EGridObjectCommand::Toggle || Link.Command == EGridObjectCommand::StartEncounter);
			if (!IsInitiallyEnabled(Link.TargetObjectId) && !bCommandCreatesDisabledMonster)
			{
				AddMessage(EGridLevelValidationSeverity::Warning,
					FString::Printf(TEXT("Link %d target object is initially disabled and may have no spawned runtime actor."), LinkIndex),
					Link.TargetObjectId);
			}

			const FGridMonsterSpawnInstance* TargetMonster = LevelAsset->FindMonsterSpawnInstanceById(Link.TargetObjectId);
			if (TargetMonster && Link.Command == EGridObjectCommand::StartEncounter && TargetMonster->EncounterGroupId.IsNone())
			{
				AddMessage(EGridLevelValidationSeverity::Error,
					FString::Printf(TEXT("Link %d command Start Encounter requires a MonsterSpawn target with EncounterGroupId."), LinkIndex),
					Link.TargetObjectId);
			}

			if (TargetType == EGridLevelObjectType::Door)
			{
				const bool bOpensDoor = Link.Command == EGridObjectCommand::Open || Link.Command == EGridObjectCommand::Activate;
				const bool bClosesDoor = Link.Command == EGridObjectCommand::Close || Link.Command == EGridObjectCommand::Deactivate;
				if (bOpensDoor || bClosesDoor)
				{
					const FString DoorCommandKey = FString::Printf(TEXT("%s|%s|%d"), *Link.SourceObjectId.ToString(EGuidFormats::Digits),
						*Link.TargetObjectId.ToString(EGuidFormats::Digits), static_cast<int32>(Link.SourceEvent));
					uint8& DirectionMask = DoorCommandDirectionsBySourceEvent.FindOrAdd(DoorCommandKey);
					DirectionMask |= bOpensDoor ? 1 : 2;
					if (DirectionMask == 3)
					{
						AddMessage(EGridLevelValidationSeverity::Warning,
							FString::Printf(TEXT("Link %d conflicts with another link: the same source event both opens and closes this door."), LinkIndex),
							Link.TargetObjectId);
					}
				}
			}
		}

		if (Link.SourceObjectId.IsValid() && Link.SourceObjectId == Link.TargetObjectId)
		{
			AddMessage(EGridLevelValidationSeverity::Warning, FString::Printf(TEXT("Link %d targets its own source object."), LinkIndex), Link.SourceObjectId);
		}

		if (Link.Condition != EGridObjectCondition::None)
		{
			if (!bHasTarget || TargetType != EGridLevelObjectType::Receptacle)
			{
				AddMessage(EGridLevelValidationSeverity::Error,
					FString::Printf(TEXT("Link %d condition %s requires a receptacle target."), LinkIndex, *ToGridObjectConditionText(Link.Condition)),
					Link.TargetObjectId);
			}

			switch (Link.Condition)
			{
				case EGridObjectCondition::ReceptacleContainsItemDefinition:
					if (Link.ConditionItemDefinitionId.IsNone())
					{
						AddMessage(EGridLevelValidationSeverity::Error,
							FString::Printf(TEXT("Link %d condition requires ConditionItemDefinitionId."), LinkIndex), Link.SourceObjectId);
					}
					break;

				case EGridObjectCondition::ReceptacleContainsItemTag:
					if (Link.ConditionItemTag.IsNone())
					{
						AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Link %d condition requires ConditionItemTag."), LinkIndex),
							Link.SourceObjectId);
					}
					break;

				case EGridObjectCondition::ReceptacleContainsItemType:
					if (Link.ConditionItemType == EGridItemType::None)
					{
						AddMessage(EGridLevelValidationSeverity::Error,
							FString::Printf(TEXT("Link %d condition requires a non-None ConditionItemType."), LinkIndex), Link.SourceObjectId);
					}
					break;

				case EGridObjectCondition::ReceptacleItemCountAtLeast:
					if (Link.ConditionCount <= 0)
					{
						AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Link %d condition requires ConditionCount > 0."), LinkIndex),
							Link.SourceObjectId);
					}
					break;

				case EGridObjectCondition::ReceptacleWeightAtLeast:
					if (Link.ConditionWeight <= 0.0f)
					{
						AddMessage(EGridLevelValidationSeverity::Error, FString::Printf(TEXT("Link %d condition requires ConditionWeight > 0."), LinkIndex),
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

	for (const FGridWorldObjectInstance& Obj : LevelAsset->WorldObjectInstances)
	{
		if (Obj.Type == EGridLevelObjectType::Trigger && !OutgoingLinkCountBySourceId.Contains(Obj.InstanceId))
		{
			AddMessage(EGridLevelValidationSeverity::Warning, TEXT("Trigger has no outgoing links."), Obj.InstanceId);
		}

		if (Obj.Type == EGridLevelObjectType::Receptacle)
		{
			const int32 ItemInsertedCount = ReceptacleItemInsertedLinkCountBySourceId.FindRef(Obj.InstanceId);
			const int32 ItemRemovedCount = ReceptacleItemRemovedLinkCountBySourceId.FindRef(Obj.InstanceId);
			const int32 ItemChangedCount = ReceptacleItemChangedLinkCountBySourceId.FindRef(Obj.InstanceId);

			if (ItemInsertedCount == 0 && ItemRemovedCount == 0 && ItemChangedCount > 0)
			{
				continue;
			}

			if (ItemRemovedCount > 0 && ItemInsertedCount == 0)
			{
				AddMessage(EGridLevelValidationSeverity::Warning,
					FString::Printf(
						TEXT(
							"Receptacle '%s' has ItemRemoved links but no ItemInserted links. This may be intentional, but the puzzle will not reset when an item is inserted again."),
						*GetObjectValidationName(Obj)),
					Obj.InstanceId);
			}

			if (ItemInsertedCount > 0 && ItemRemovedCount == 0)
			{
				AddMessage(EGridLevelValidationSeverity::Warning,
					FString::Printf(
						TEXT(
							"Receptacle '%s' has ItemInserted links but no ItemRemoved links. This may be intentional, but the puzzle will not react when the item is removed."),
						*GetObjectValidationName(Obj)),
					Obj.InstanceId);
			}
		}
	}

	if (LastValidationMessages.Num() == 0)
	{
		AddMessage(EGridLevelValidationSeverity::Info, TEXT("Validation complete: no issues found."));
	}

	for (FGridLevelValidationMessage& ValidationMessage : LastValidationMessages)
	{
		ValidationMessage.Category = InferValidationCategory(ValidationMessage.Message);

		if (ValidationMessage.Message.StartsWith(TEXT("Link ")))
		{
			const int32 LinkIndex = FCString::Atoi(*ValidationMessage.Message.Mid(5));
			if (LevelAsset->Links.IsValidIndex(LinkIndex))
			{
				const FGridObjectLink& Link = LevelAsset->Links[LinkIndex];
				ValidationMessage.SourceObjectId = Link.SourceObjectId;
				ValidationMessage.TargetObjectId = Link.TargetObjectId;
			}
		}

		FGuid LocationObjectId = ValidationMessage.OptionalObjectId;
		if (!LocationObjectId.IsValid())
		{
			LocationObjectId = ValidationMessage.SourceObjectId.IsValid() ? ValidationMessage.SourceObjectId : ValidationMessage.TargetObjectId;
		}

		LevelAsset->TryGetTypedPlacementLocation(LocationObjectId, ValidationMessage.CellX, ValidationMessage.CellY, ValidationMessage.Edge);
	}

	return LastValidationMessages;
}
