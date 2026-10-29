bool AGridLevelEditorActor::EnsureStairsRelocationDefinitions(FString& OutError)
{
	OutError.Reset();

	if (!ObjectPalette)
	{
		OutError = TEXT("ObjectPalette is null.");
		return false;
	}

#if WITH_EDITOR
	UStaticMesh* StairsUpMesh = FindStaticMeshByAssetName(FName(TEXT("SM_Stairs_Up_01")));
	UStaticMesh* StairsDownMesh = FindStaticMeshByAssetName(FName(TEXT("SM_Stairs_Down_01")));

	if (!StairsUpMesh || !StairsDownMesh)
	{
		OutError = FString::Printf(TEXT("Missing stair mesh asset(s): SM_Stairs_Up_01=%s SM_Stairs_Down_01=%s."), StairsUpMesh ? TEXT("OK") : TEXT("Missing"),
			StairsDownMesh ? TEXT("OK") : TEXT("Missing"));
		UE_LOG(LogGridEditor, Error, TEXT("%s"), *OutError);
		return false;
	}

	bool bCreatedUp = false;
	bool bCreatedDown = false;
	UGridWorldObjectDefinitionAsset* StairsUpDefinition = LoadOrCreateWorldObjectDefinitionAsset(
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Up"), TEXT("DA_Stairs_Up"), bCreatedUp);
	UGridWorldObjectDefinitionAsset* StairsDownDefinition = LoadOrCreateWorldObjectDefinitionAsset(
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Down"), TEXT("DA_Stairs_Down"), bCreatedDown);

	if (!StairsUpDefinition || !StairsDownDefinition)
	{
		OutError = TEXT("Failed to load or create Stairs_Up / Stairs_Down definition assets.");
		return false;
	}

	// Provision missing assets only. Existing definitions are authored data:
	// rewriting FText::FromString on each palette refresh generates new localization keys.
	const auto InitializeNewStairsDefinition = [](UGridWorldObjectDefinitionAsset& Definition, FName WorldObjectDefinitionId,
		const FText& DisplayName, const FText& Description, UStaticMesh* Mesh, bool bHideCellFloor)
	{
		Definition.Modify();
		Definition.DefinitionId = WorldObjectDefinitionId;
		Definition.DisplayName = DisplayName;
		Definition.SupportedType = EGridLevelObjectType::Relocation;
		Definition.Description = Description;
		Definition.DefaultBehavior = FGridObjectBehaviorParams();
		Definition.PlacementSurface = EGridObjectPlacementKind::Floor;
		Definition.DefaultLocalPosition = FGridSurfaceLocalPosition();
		Definition.bReplacesStandardWall = false;
		Definition.bBlocksMovement = false;
		Definition.bHideCellFloor = bHideCellFloor;
		Definition.bIsInteractable = false;
		Definition.bIsReadable = false;
		Definition.bIsLightSource = false;
		Definition.StaticPart.Mesh = Mesh;
		Definition.StaticPart.LocalTransform = FTransform::Identity;
		Definition.MovingParts.Reset();
		Definition.RuntimeActorClass = AGridGenericObjectActor::StaticClass();
		Definition.MarkPackageDirty();
	};

	if (bCreatedUp)
	{
		InitializeNewStairsDefinition(*StairsUpDefinition, FName(TEXT("Stairs_Up")),
			NSLOCTEXT("Grimrock.WorldObjects", "StairsUp.DisplayName", "Stairs Up"),
			NSLOCTEXT("Grimrock.WorldObjects", "StairsUp.Description", "Dungeon relocation stair for automatic party navigation."),
			StairsUpMesh, false);
	}
	if (bCreatedDown)
	{
		InitializeNewStairsDefinition(*StairsDownDefinition, FName(TEXT("Stairs_Down")),
			NSLOCTEXT("Grimrock.WorldObjects", "StairsDown.DisplayName", "Stairs Down"),
			NSLOCTEXT("Grimrock.WorldObjects", "StairsDown.Description", "Dungeon relocation stair for automatic party navigation."),
			StairsDownMesh, true);
	}

	bool bPaletteChanged = false;
	const auto AddOrUpdatePaletteEntry = [this, &bPaletteChanged](FName EntryId, UGridWorldObjectDefinitionAsset* Definition)
	{
		FGridObjectPaletteEntry* Entry = ObjectPalette->Entries.FindByPredicate(
			[EntryId](const FGridObjectPaletteEntry& Candidate) { return Candidate.EntryId == EntryId; });

		const FName NavigationCategory(TEXT("Navigation"));
		if (Entry && Entry->PaletteCategory == NavigationCategory && Entry->DefaultWorldObjectDefinition == Definition)
		{
			return;
		}

		if (!bPaletteChanged)
		{
			ObjectPalette->Modify();
			bPaletteChanged = true;
		}
		if (!Entry)
		{
			Entry = &ObjectPalette->Entries.AddDefaulted_GetRef();
			Entry->EntryId = EntryId;
		}
		Entry->PaletteCategory = NavigationCategory;
		Entry->DefaultWorldObjectDefinition = Definition;
	};

	AddOrUpdatePaletteEntry(FName(TEXT("Stairs_Up")), StairsUpDefinition);
	AddOrUpdatePaletteEntry(FName(TEXT("Stairs_Down")), StairsDownDefinition);
	if (bPaletteChanged)
	{
		ObjectPalette->MarkPackageDirty();
	}

	ResolvePreviewRuntimeActor();
	if (PreviewRuntimeActor &&
		(!PreviewRuntimeActor->WorldObjectDefinitions.Contains(StairsUpDefinition) ||
		 !PreviewRuntimeActor->WorldObjectDefinitions.Contains(StairsDownDefinition)))
	{
		PreviewRuntimeActor->Modify();
		PreviewRuntimeActor->WorldObjectDefinitions.AddUnique(StairsUpDefinition);
		PreviewRuntimeActor->WorldObjectDefinitions.AddUnique(StairsDownDefinition);
	}

	TArray<UPackage*> PackagesToSave;
	if (bCreatedUp) PackagesToSave.AddUnique(StairsUpDefinition->GetOutermost());
	if (bCreatedDown) PackagesToSave.AddUnique(StairsDownDefinition->GetOutermost());
	if (bPaletteChanged) PackagesToSave.AddUnique(ObjectPalette->GetOutermost());
	if (!PackagesToSave.IsEmpty())
	{
		UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
	}

	UE_LOG(LogGridEditor, Verbose, TEXT("Stairs relocation definitions ensured: Stairs_Up=%s Stairs_Down=%s Palette=%s CreatedUp=%s CreatedDown=%s PaletteChanged=%s."),
		*StairsUpDefinition->GetPathName(), *StairsDownDefinition->GetPathName(), *ObjectPalette->GetPathName(), bCreatedUp ? TEXT("true") : TEXT("false"),
		bCreatedDown ? TEXT("true") : TEXT("false"), bPaletteChanged ? TEXT("true") : TEXT("false"));

	return true;
#else
	OutError = TEXT("EnsureStairsRelocationDefinitions is editor-only.");
	return false;
#endif
}

bool AGridLevelEditorActor::EnsurePitTrapdoorDefinition(FString& OutError)
{
	OutError.Reset();
	if (!ObjectPalette)
	{
		OutError = TEXT("ObjectPalette is null.");
		return false;
	}

#if WITH_EDITOR
	UStaticMesh* PitMesh = FindStaticMeshByAssetName(FName(TEXT("SM_Pit_Stone_01")));
	if (!PitMesh)
	{
		OutError = TEXT("Missing pit mesh asset: SM_Pit_Stone_01.");
		UE_LOG(LogGridEditor, Error, TEXT("%s"), *OutError);
		return false;
	}

	bool bCreated = false;
	UGridWorldObjectDefinitionAsset* PitDefinition = LoadOrCreateWorldObjectDefinitionAsset(
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Pit_Stone_01"), TEXT("DA_Pit_Stone_01"), bCreated);
	if (!PitDefinition)
	{
		OutError = TEXT("Failed to load or create DA_Pit_Stone_01.");
		return false;
	}

	PitDefinition->Modify();
	PitDefinition->DefinitionId = FName(TEXT("Pit_Stone_01"));
	PitDefinition->DisplayName = FText::FromString(TEXT("Stone Pit"));
	PitDefinition->SupportedType = EGridLevelObjectType::Pit;
	PitDefinition->Description = FText::FromString(TEXT("Controlled inter-level pit with optional dual-part trapdoor cover."));
	PitDefinition->DefaultBehavior = FGridObjectBehaviorParams();
	PitDefinition->DefaultBehavior.Pit.bInitiallyOpen = true;
	PitDefinition->DefaultBehavior.Pit.bUseSameCellCoordinates = true;
	PitDefinition->DefaultBehavior.Relocation.TargetLevelId = NAME_None;
	PitDefinition->DefaultBehavior.Relocation.TargetCellX = 0;
	PitDefinition->DefaultBehavior.Relocation.TargetCellY = 0;
	PitDefinition->DefaultBehavior.Relocation.TargetFacing = EGridEdge::North;
	PitDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	PitDefinition->DefaultLocalPosition = FGridSurfaceLocalPosition();
	PitDefinition->bReplacesStandardWall = false;
	PitDefinition->bBlocksMovement = false;
	PitDefinition->bHideCellFloor = true;
	PitDefinition->bIsInteractable = false;
	PitDefinition->bIsReadable = false;
	PitDefinition->bIsLightSource = false;
	PitDefinition->StaticPart.Mesh = PitMesh;
	PitDefinition->StaticPart.LocalTransform = FTransform::Identity;
	PitDefinition->RuntimeActorClass = AGridPitTrapdoorActor::StaticClass();

	// WORLDOBJ-MIG04: a pit has either no moving cover or a complete Part0/Part1 pair.
	// When a complete pair exists, Motion is the sole persisted hinge/angle/duration authority.
	if (PitDefinition->GetDefinedMovingPartCount() == 1)
	{
		PitDefinition->MovingParts.Reset();
		UE_LOG(LogGridEditor, Warning,
			TEXT("WORLDOBJ-MIG04: incomplete Pit MovingParts reset for %s; a Pit requires either zero or two moving parts."),
			*PitDefinition->GetPathName());
	}
	else if (PitDefinition->GetDefinedMovingPartCount() == 2)
	{
		PitDefinition->MovingParts[0].Motion.Type = EGridWorldObjectMotionType::Rotation;
		PitDefinition->MovingParts[0].Motion.Axis = EGridWorldObjectMotionAxis::Y;
		PitDefinition->MovingParts[0].Motion.Pivot = FVector(-85.f, 0.f, -5.f);
		// With the authored +/-85 cm hinges, these signs rotate both leaves downward.
		PitDefinition->MovingParts[0].Motion.Amount = 80.f;
		PitDefinition->MovingParts[0].Motion.Duration = 0.75f;

		PitDefinition->MovingParts[1].Motion.Type = EGridWorldObjectMotionType::Rotation;
		PitDefinition->MovingParts[1].Motion.Axis = EGridWorldObjectMotionAxis::Y;
		PitDefinition->MovingParts[1].Motion.Pivot = FVector(85.f, 0.f, -5.f);
		PitDefinition->MovingParts[1].Motion.Amount = -80.f;
		PitDefinition->MovingParts[1].Motion.Duration = 0.75f;
	}

	PitDefinition->MarkPackageDirty();

	ObjectPalette->Modify();
	FGridObjectPaletteEntry* Entry = ObjectPalette->Entries.FindByPredicate(
		[](const FGridObjectPaletteEntry& Candidate)
		{
			return Candidate.EntryId == FName(TEXT("Pit_Stone_01"));
		});
	if (!Entry)
	{
		Entry = &ObjectPalette->Entries.AddDefaulted_GetRef();
	}
	Entry->EntryId = FName(TEXT("Pit_Stone_01"));
	Entry->PaletteCategory = FName(TEXT("Hazards"));
	Entry->DefaultWorldObjectDefinition = PitDefinition;
	ObjectPalette->MarkPackageDirty();

	ResolvePreviewRuntimeActor();
	if (PreviewRuntimeActor)
	{
		PreviewRuntimeActor->Modify();
		PreviewRuntimeActor->WorldObjectDefinitions.AddUnique(PitDefinition);
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.AddUnique(PitDefinition->GetOutermost());
	PackagesToSave.AddUnique(ObjectPalette->GetOutermost());
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	UE_LOG(LogGridEditor, Verbose, TEXT("Pit trapdoor definition ensured from generic Motion: Pit=%s Palette=%s Created=%s MovingParts=%d."),
		*PitDefinition->GetPathName(), *ObjectPalette->GetPathName(), bCreated ? TEXT("true") : TEXT("false"), PitDefinition->GetDefinedMovingPartCount());
	return true;
#else
	OutError = TEXT("EnsurePitTrapdoorDefinition is editor-only.");
	return false;
#endif
}
