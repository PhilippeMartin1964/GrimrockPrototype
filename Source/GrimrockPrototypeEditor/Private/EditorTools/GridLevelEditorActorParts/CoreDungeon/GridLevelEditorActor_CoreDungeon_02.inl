			}
			default:
				return false;
		}
	}

	bool IsCommandSupportedByCurrentRuntime(EGridLevelObjectType TargetType, EGridObjectCommand Command)
	{
		// The type-only diagnostic remains permissive for LogicReset; link validation resolves the real node.
		const EGridLogicNodeType NodeType = TargetType == EGridLevelObjectType::Logic && Command == EGridObjectCommand::LogicReset
			? EGridLogicNodeType::Latch : EGridLogicNodeType::Relay;
		return GridEditorLinkPolicy::GetCommandRuntimeSupport(TargetType, NodeType, Command) == EGridEditorCommandRuntimeSupport::Gameplay;
	}

	FString GetLevelAssetStatsText(const UGridLevelAsset* Asset)
	{
		return Asset ? FString::Printf(TEXT("Cells=%d Objects=%d Links=%d"), Asset->Cells.Num(), Asset->GetTypedPlacementCount(), Asset->Links.Num())
			: TEXT("Cells=0 Objects=0 Links=0");
	}

	FString GetGridEdgeText(EGridEdge Edge)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>()) return EdgeEnum->GetNameStringByValue(static_cast<int64>(Edge));
		return FString::Printf(TEXT("%d"), static_cast<int32>(Edge));
	}

	FString GetLevelStartText(const UGridLevelAsset* Asset)
	{
		if (!Asset) return TEXT("Cell=None Facing=None Valid=false");
		return FString::Printf(TEXT("Cell=(%d,%d) Facing=%s Valid=%s"), Asset->StartCellX, Asset->StartCellY, *GetGridEdgeText(Asset->StartFacing),
			Asset->IsStartCellValid() ? TEXT("true") : TEXT("false"));
	}

	FString GetObjectWorkflowAssetName(const UObject* Object)
	{
		return Object ? Object->GetName() : TEXT("None");
	}

	FString GetReceptacleWorkflowStatus(const TArray<FGridReceptacleInitialItemConfig>& InitialContent)
	{
		if (InitialContent.IsEmpty()) return TEXT("EMPTY_RECEPTACLE");
		for (const FGridReceptacleInitialItemConfig& InitialItem : InitialContent)
		{
			if (!InitialItem.ItemDefinition || InitialItem.ItemDefinition->ItemDefinitionId.IsNone()) return TEXT("ERROR_INVALID_INITIAL_CONTENT");
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
		const int32 ItemPlacements = Asset->LooseItemInstances.Num();
		int32 ItemPlacementsUsingDefinitionAsset = 0;
		const int32 ItemPlacementsUsingDefinitionId = 0;
		const int32 ItemPlacementsUsingLegacyFallback = 0;
		int32 Receptacles = 0;
		int32 ReceptaclesUsingInitialDefinition = 0;
		Result += FString::Printf(TEXT("Level=%s Asset=%s\n"), *LevelLabel, *Asset->GetName());
		for (const FGridLooseItemInstance& LooseItemInstance : Asset->LooseItemInstances)
		{
			if (LooseItemInstance.ItemDefinition) ++ItemPlacementsUsingDefinitionAsset;
			Result += FString::Printf(TEXT("  Item ObjectId=%s Cell=(%d,%d) ItemDefinitionAsset=%s Status=%s\n"),
				*LooseItemInstance.InstanceId.ToString(), LooseItemInstance.CellX, LooseItemInstance.CellY,
				*GetObjectWorkflowAssetName(LooseItemInstance.ItemDefinition),
				LooseItemInstance.ItemDefinition ? TEXT("OK_ITEM_DEFINITION_ASSET") : TEXT("ERROR_NO_ITEM_DEFINITION"));
		}
		for (const FGridWorldObjectInstance& WorldObjectInstance : Asset->WorldObjectInstances)
		{
			if (WorldObjectInstance.Type != EGridLevelObjectType::Receptacle) continue;
			++Receptacles;
			const TArray<FGridReceptacleInitialItemConfig>& InitialContent = WorldObjectInstance.InstanceConfig.ReceptacleInitialContent;
			const FString Status = GetReceptacleWorkflowStatus(InitialContent);
			if (Status == TEXT("OK_INITIAL_CONTENT")) ++ReceptaclesUsingInitialDefinition;
			Result += FString::Printf(TEXT("  Receptacle ObjectId=%s ArchetypeId=%s InitialContentCount=%d Status=%s\n"),
				*WorldObjectInstance.InstanceId.ToString(), *WorldObjectInstance.WorldObjectDefinitionId.ToString(), InitialContent.Num(), *Status);
		}
		Result += FString::Printf(
			TEXT("  ItemDefinitionWorkflow: ItemPlacements=%d ItemPlacementsUsingDefinitionAsset=%d ItemPlacementsUsingDefinitionId=%d ItemPlacementsUsingLegacyFallback=%d Receptacles=%d ReceptaclesUsingInitialDefinition=%d\n"),
			ItemPlacements,
			ItemPlacementsUsingDefinitionAsset,
