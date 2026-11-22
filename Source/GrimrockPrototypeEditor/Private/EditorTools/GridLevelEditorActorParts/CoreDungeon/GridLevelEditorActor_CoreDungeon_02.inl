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

	FString GetReceptacleWorkflowStatus(const TArray<FGridReceptacleInitialItemConfig>& InitialContent)
	{
		if (InitialContent.IsEmpty()) return TEXT("EMPTY_RECEPTACLE");
		for (const FGridReceptacleInitialItemConfig& InitialItem : InitialContent)
		{
			if (InitialItem.ItemId.IsNone() ||
				InitialItem.ItemLevel < 0 || InitialItem.ItemLevel > 20 || InitialItem.Quantity < 1)
				return TEXT("ERROR_INVALID_INITIAL_CONTENT");
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
		const int32 ItemPlacementsUsingDefinitionAsset = 0;
		int32 ItemPlacementsUsingDefinitionId = 0;
		const int32 ItemPlacementsUsingLegacyFallback = 0;
		int32 Receptacles = 0;
		int32 ReceptaclesUsingInitialDefinition = 0;
		Result += FString::Printf(TEXT("Level=%s Asset=%s\n"), *LevelLabel, *Asset->GetName());
		for (const FGridLooseItemInstance& LooseItemInstance : Asset->LooseItemInstances)
		{
			if (!LooseItemInstance.ItemId.IsNone()) ++ItemPlacementsUsingDefinitionId;
			const bool bHasPhysicalFields = LooseItemInstance.InstanceId.IsValid() &&
				!LooseItemInstance.ItemId.IsNone() && LooseItemInstance.Quantity > 0 &&
				LooseItemInstance.ItemLevel >= 0 && LooseItemInstance.ItemLevel <= 20;
			Result += FString::Printf(TEXT("  Item ObjectId=%s Cell=(%d,%d) ItemId=%s N%d Status=%s\n"),
				*LooseItemInstance.InstanceId.ToString(), LooseItemInstance.CellX, LooseItemInstance.CellY,
				*LooseItemInstance.ItemId.ToString(), LooseItemInstance.ItemLevel,
				bHasPhysicalFields ? TEXT("AUTHORED_CANONICAL_FIELDS") : TEXT("ERROR_INVALID_CANONICAL_FIELDS"));
		}
		for (const FGridWorldObjectInstance& WorldObjectInstance : Asset->WorldObjectInstances)
		{
			if (WorldObjectInstance.Type != EGridLevelObjectType::Receptacle) continue;
			++Receptacles;
			const TArray<FGridReceptacleInitialItemConfig>& InitialContent = WorldObjectInstance.InstanceConfig.ReceptacleInitialContent;
			const FString Status = GetReceptacleWorkflowStatus(InitialContent);
			if (Status == TEXT("OK_INITIAL_CONTENT")) ++ReceptaclesUsingInitialDefinition;
			Result += FString::Printf(TEXT("  Receptacle ObjectId=%s WorldObjectDefinitionId=%s InitialContentCount=%d Status=%s\n"),
				*WorldObjectInstance.InstanceId.ToString(), *WorldObjectInstance.WorldObjectDefinitionId.ToString(), InitialContent.Num(), *Status);
		}
		Result += FString::Printf(
			TEXT("  ItemDefinitionWorkflow: ItemPlacements=%d ItemPlacementsUsingDefinitionAsset=%d ItemPlacementsUsingDefinitionId=%d ItemPlacementsUsingLegacyFallback=%d Receptacles=%d ReceptaclesUsingInitialDefinition=%d\n"),
			ItemPlacements,
			ItemPlacementsUsingDefinitionAsset,
