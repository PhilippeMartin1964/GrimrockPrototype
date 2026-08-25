MonsterSpawn.Type = EGridLevelObjectType::MonsterSpawn;
return GridEditorLinkPolicy::GetSupportedEventsForSource(MonsterSpawn).Contains(Event);
}

default:
	return false;
	}
	}

	bool IsCommandSupportedByCurrentRuntime(EGridLevelObjectType TargetType, EGridObjectCommand Command)
	{
		if (TargetType == EGridLevelObjectType::MonsterSpawn)
		{
			FGridLevelObjectData MonsterSpawn;
			MonsterSpawn.Type = EGridLevelObjectType::MonsterSpawn;
			return GridEditorLinkPolicy::GetSupportedCommandsForTarget(MonsterSpawn).Contains(Command);
		}

		if (IsReceptacleCommand(Command))
		{
			return TargetType == EGridLevelObjectType::Receptacle;
		}

		const bool bStateCommand = Command == EGridObjectCommand::Toggle || Command == EGridObjectCommand::Open || Command == EGridObjectCommand::Close ||
			Command == EGridObjectCommand::Activate || Command == EGridObjectCommand::Deactivate;
		if (!bStateCommand)
		{
			return false;
		}

		switch (TargetType)
		{
			case EGridLevelObjectType::Door:
			case EGridLevelObjectType::Button:
			case EGridLevelObjectType::PressurePlate:
			case EGridLevelObjectType::Lever:
			case EGridLevelObjectType::Decoration:
			case EGridLevelObjectType::ItemSpawn:
			case EGridLevelObjectType::Item:
			case EGridLevelObjectType::Light:
			case EGridLevelObjectType::Teleporter:
			case EGridLevelObjectType::Trigger:
			case EGridLevelObjectType::Receptacle:
				return true;

			case EGridLevelObjectType::None:
			default:
				return false;
		}
	}

	FString GetLevelAssetStatsText(const UGridLevelAsset* Asset)
	{
		if (!Asset)
		{
			return TEXT("Cells=0 Objects=0 Links=0");
		}

		return FString::Printf(TEXT("Cells=%d Objects=%d Links=%d"), Asset->Cells.Num(), Asset->Objects.Num(), Asset->Links.Num());
	}

	FString GetGridEdgeText(EGridEdge Edge)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			return EdgeEnum->GetNameStringByValue(static_cast<int64>(Edge));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Edge));
	}

	FString GetLevelStartText(const UGridLevelAsset* Asset)
	{
		if (!Asset)
		{
			return TEXT("Cell=None Facing=None Valid=false");
		}

		return FString::Printf(TEXT("Cell=(%d,%d) Facing=%s Valid=%s"), Asset->StartCellX, Asset->StartCellY, *GetGridEdgeText(Asset->StartFacing),
			Asset->IsStartCellValid() ? TEXT("true") : TEXT("false"));
	}

	FString GetObjectWorkflowAssetName(const UObject* Object)
	{
		return Object ? Object->GetName() : TEXT("None");
	}

	FString GetItemPlacementWorkflowStatus(const FGridLevelObjectData& Object)
	{
		if (Object.ItemDefinitionAsset)
		{
			const FName AssetId = Object.ItemDefinitionAsset->ItemDefinitionId;
			if (!Object.ItemDefinitionId.IsNone() && Object.ItemDefinitionId != AssetId)
			{
				return TEXT("ERROR_CONFLICTING_DEFINITIONS");
			}
			if (!Object.ArchetypeId.IsNone() && !AssetId.IsNone() && Object.ArchetypeId != AssetId)
			{
				return TEXT("ERROR_CONFLICTING_DEFINITIONS");
			}
			return TEXT("OK_ITEM_DEFINITION_ASSET");
		}

		if (!Object.ItemDefinitionId.IsNone())
		{
			if (!Object.ArchetypeId.IsNone() && Object.ArchetypeId != Object.ItemDefinitionId)
			{
				return TEXT("ERROR_CONFLICTING_DEFINITIONS");
			}
			return TEXT("OK_ITEM_DEFINITION_ID");
		}

		if (!Object.ArchetypeId.IsNone())
		{
			return TEXT("LEGACY_ARCHETYPE_FALLBACK");
		}

		return TEXT("ERROR_NO_ITEM_DEFINITION");
	}

	FString GetReceptacleWorkflowStatus(const FGridReceptacleBehaviorParams& Receptacle)
	{
		if (Receptacle.InitialContent.Num() == 0)
		{
			return TEXT("EMPTY_RECEPTACLE");
		}

		for (const FGridReceptacleInitialItemConfig& InitialItem : Receptacle.InitialContent)
		{
			if (!InitialItem.ItemDefinition || InitialItem.ItemDefinition->ItemDefinitionId.IsNone())
			{
				return TEXT("ERROR_INVALID_INITIAL_CONTENT");
			}
		}

		return TEXT("OK_INITIAL_CONTENT");
	}

	void AppendItemWorkflowDiagnosticsForLevel(FString& Result, const UGridLevelAsset* Asset, const FString& LevelLabel)
	{
		if (!Asset)
		{
			Result += FString::Printf(TEXT("Level=%s Status=ERROR_MISSING_LEVEL_ASSET\n"), *LevelLabel);
			return;
		}

		int32 ItemPlacements = 0;
		int32 ItemPlacementsUsingDefinitionAsset = 0;
		int32 ItemPlacementsUsingDefinitionId = 0;
		int32 ItemPlacementsUsingLegacyFallback = 0;
		int32 Receptacles = 0;
		int32 ReceptaclesUsingInitialDefinition = 0;

		Result += FString::Printf(TEXT("Level=%s Asset=%s\n"), *LevelLabel, *Asset->GetName());

		for (const FGridLevelObjectData& Object : Asset->Objects)
		{
			if (Object.Type == EGridLevelObjectType::Item)
			{
				++ItemPlacements;
				const FString Status = GetItemPlacementWorkflowStatus(Object);
				if (Status == TEXT("OK_ITEM_DEFINITION_ASSET"))
				{
					++ItemPlacementsUsingDefinitionAsset;
				}
				else if (Status == TEXT("OK_ITEM_DEFINITION_ID"))
				{
					++ItemPlacementsUsingDefinitionId;
				}
				else if (Status == TEXT("LEGACY_ARCHETYPE_FALLBACK"))
				{
					++ItemPlacementsUsingLegacyFallback;
				}

				Result += FString::Printf(TEXT("  Item ObjectId=%s Cell=(%d,%d) ArchetypeId=%s ItemDefinitionAsset=%s ItemDefinitionId=%s Status=%s\n"),
					*Object.ObjectId.ToString(), Object.CellX, Object.CellY, *Object.ArchetypeId.ToString(),
					*GetObjectWorkflowAssetName(Object.ItemDefinitionAsset), *Object.ItemDefinitionId.ToString(), *Status);
			}

			if (Object.Type == EGridLevelObjectType::Receptacle)
			{
				++Receptacles;
				const FGridReceptacleBehaviorParams& Receptacle = Object.Behavior.Receptacle;
				const FString Status = GetReceptacleWorkflowStatus(Receptacle);
				if (Status == TEXT("OK_INITIAL_CONTENT"))
				{
					++ReceptaclesUsingInitialDefinition;
				}

				Result += FString::Printf(TEXT("  Receptacle ObjectId=%s ArchetypeId=%s InitialContentCount=%d Status=%s\n"), *Object.ObjectId.ToString(),
					*Object.ArchetypeId.ToString(), Receptacle.InitialContent.Num(), *Status);
			}
		}

		Result += FString::Printf (
            TEXT ("  ItemDefinitionWorkflow: ItemPlacements=%d ItemPlacementsUsingDefinitionAsset=%d ItemPlacementsUsingDefinitionId=%d ItemPlacementsUsingLegacyFallback=%d Receptacles=%d ReceptaclesUsingInitialDefinition=%d\n"),
            ItemPlacements,
            ItemPlacementsUsingDefinitionAsset,
