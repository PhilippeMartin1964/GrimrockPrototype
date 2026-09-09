bool AGridLevelEditorActor::EnsureStairsTransitionDefinitions(FString& OutError)
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
		UE_LOG(LogTemp, Error, TEXT("%s"), *OutError);
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

	const auto ConfigureTargetStairsTransitionDefinition = [](UGridWorldObjectDefinitionAsset& Definition, FName WorldObjectDefinitionId, const TCHAR* DisplayName,
		UStaticMesh* Mesh, bool bHideCellFloor)
	{
		Definition.Modify();
		Definition.DefinitionId = WorldObjectDefinitionId;
		Definition.DisplayName = FText::FromString(DisplayName);
		Definition.SupportedType = EGridLevelObjectType::Decoration;
		Definition.Description = FText::FromString(TEXT("Dungeon transition stair object."));
		Definition.bDefaultInitiallyEnabled = true;
		Definition.bDefaultInitiallyActive = false;
		Definition.DefaultTag = NAME_None;
		Definition.DefaultBehavior = FGridObjectBehaviorParams();
		Definition.DefaultBehavior.Transition.bIsTransition = true;
		Definition.DefaultBehavior.Transition.TargetLevelId = NAME_None;
		Definition.DefaultBehavior.Transition.TargetCellX = 0;
		Definition.DefaultBehavior.Transition.TargetCellY = 0;
		Definition.DefaultBehavior.Transition.TargetFacing = EGridEdge::North;
		Definition.DefaultBehavior.Transition.bRequireUseAction = false;
		Definition.Category = FName(TEXT("Transitions"));
		Definition.ObjectCategory = EGridObjectCategory::Decoration;
		Definition.PlacementSurface = EGridObjectPlacementKind::Floor;
		Definition.DefaultLocalPosition = FGridSurfaceLocalPosition();
		Definition.bCanShareCell = true;
		Definition.bCanShareAnchor = true;
		Definition.bReplacesStandardWall = false;
		Definition.bBlocksMovement = false;
		Definition.bHideCellFloor = bHideCellFloor;
		Definition.bIsInteractable = false;
		Definition.bIsReadable = false;
		Definition.bIsLightSource = false;
		Definition.StaticPart.Mesh = Mesh;
		Definition.StaticPart.LocalTransform = FTransform::Identity;
		Definition.MovingParts = FGridWorldObjectMovingParts();
		Definition.RuntimeActorClass = AGridGenericObjectActor::StaticClass();
		Definition.ItemActorClass = nullptr;
		Definition.MarkPackageDirty();
	};

	ConfigureTargetStairsTransitionDefinition(*StairsUpDefinition, FName(TEXT("Stairs_Up")), TEXT("Stairs Up"), StairsUpMesh, false);
	ConfigureTargetStairsTransitionDefinition(*StairsDownDefinition, FName(TEXT("Stairs_Down")), TEXT("Stairs Down"), StairsDownMesh, true);

	ObjectPalette->Modify();

	const auto AddOrUpdatePaletteEntry = [this](FName EntryId, const FText& DisplayName, UGridWorldObjectDefinitionAsset* Definition)
	{
		FGridObjectPaletteEntry* ExistingEntry = ObjectPalette->Entries.FindByPredicate(
			[EntryId](const FGridObjectPaletteEntry& Entry)
			{
				return Entry.EntryId == EntryId;
			});

		if (!ExistingEntry)
		{
			ExistingEntry = &ObjectPalette->Entries.AddDefaulted_GetRef();
		}

		ExistingEntry->EntryId = EntryId;
		ExistingEntry->DisplayNameOverride = DisplayName;
		ExistingEntry->CategoryOverride = FName(TEXT("Transitions"));
		ExistingEntry->DefaultWorldObjectDefinition = Definition;
	};

	AddOrUpdatePaletteEntry(FName(TEXT("Stairs_Up")), FText::FromString(TEXT("Stairs Up")), StairsUpDefinition);
	AddOrUpdatePaletteEntry(FName(TEXT("Stairs_Down")), FText::FromString(TEXT("Stairs Down")), StairsDownDefinition);
	ObjectPalette->MarkPackageDirty();

	ResolvePreviewRuntimeActor();
	if (PreviewRuntimeActor)
	{
		PreviewRuntimeActor->Modify();
		PreviewRuntimeActor->WorldObjectDefinitions.AddUnique(StairsUpDefinition);
		PreviewRuntimeActor->WorldObjectDefinitions.AddUnique(StairsDownDefinition);
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.AddUnique(StairsUpDefinition->GetOutermost());
	PackagesToSave.AddUnique(StairsDownDefinition->GetOutermost());
	PackagesToSave.AddUnique(ObjectPalette->GetOutermost());
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	UE_LOG(LogTemp, Log, TEXT("Stairs transition definitions ensured from target visual composition: Stairs_Up=%s Stairs_Down=%s Palette=%s CreatedUp=%s CreatedDown=%s."),
		*StairsUpDefinition->GetPathName(), *StairsDownDefinition->GetPathName(), *ObjectPalette->GetPathName(), bCreatedUp ? TEXT("true") : TEXT("false"),
		bCreatedDown ? TEXT("true") : TEXT("false"));

	return true;
#else
	OutError = TEXT("EnsureStairsTransitionDefinitions is editor-only.");
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
		UE_LOG(LogTemp, Error, TEXT("%s"), *OutError);
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
	PitDefinition->bDefaultInitiallyEnabled = true;
	PitDefinition->bDefaultInitiallyActive = false;
	PitDefinition->DefaultTag = NAME_None;
	PitDefinition->DefaultBehavior = FGridObjectBehaviorParams();
	PitDefinition->DefaultBehavior.Pit.bInitiallyOpen = true;
	PitDefinition->DefaultBehavior.Pit.bUseSameCellCoordinates = true;
	PitDefinition->DefaultBehavior.Transition.bIsTransition = true;
	PitDefinition->DefaultBehavior.Transition.TargetLevelId = NAME_None;
	PitDefinition->DefaultBehavior.Transition.TargetCellX = 0;
	PitDefinition->DefaultBehavior.Transition.TargetCellY = 0;
	PitDefinition->DefaultBehavior.Transition.TargetFacing = EGridEdge::North;
	PitDefinition->DefaultBehavior.Transition.bRequireUseAction = false;
	PitDefinition->Category = FName(TEXT("Hazards"));
	PitDefinition->ObjectCategory = EGridObjectCategory::Mechanism;
	PitDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	PitDefinition->DefaultLocalPosition = FGridSurfaceLocalPosition();
	PitDefinition->bCanShareCell = false;
	PitDefinition->bCanShareAnchor = false;
	PitDefinition->bReplacesStandardWall = false;
	PitDefinition->bBlocksMovement = false;
	PitDefinition->bHideCellFloor = true;
	PitDefinition->bIsInteractable = false;
	PitDefinition->bIsReadable = false;
	PitDefinition->bIsLightSource = false;
	PitDefinition->StaticPart.Mesh = PitMesh;
	PitDefinition->StaticPart.LocalTransform = FTransform::Identity;
	PitDefinition->RuntimeActorClass = AGridPitTrapdoorActor::StaticClass();
	PitDefinition->ItemActorClass = nullptr;

	// WORLDOBJ-MIG04: a pit has either no moving cover or a complete Part0/Part1 pair.
	// When a complete pair exists, Motion is the sole persisted hinge/angle/duration authority.
	if (PitDefinition->MovingParts.NumDefined() == 1)
	{
		PitDefinition->MovingParts = FGridWorldObjectMovingParts();
		UE_LOG(LogTemp, Warning,
			TEXT("WORLDOBJ-MIG04: incomplete Pit MovingParts reset for %s; a Pit requires either zero or two moving parts."),
			*PitDefinition->GetPathName());
	}
	else if (PitDefinition->MovingParts.NumDefined() == 2)
	{
		PitDefinition->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Rotation;
		PitDefinition->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Y;
		PitDefinition->MovingParts.Part0.Motion.Pivot = FVector(-85.f, 0.f, -5.f);
		// With the authored +/-85 cm hinges, these signs rotate both leaves downward.
		PitDefinition->MovingParts.Part0.Motion.Amount = 80.f;
		PitDefinition->MovingParts.Part0.Motion.Duration = 0.75f;

		PitDefinition->MovingParts.Part1.Motion.Type = EGridWorldObjectMotionType::Rotation;
		PitDefinition->MovingParts.Part1.Motion.Axis = EGridWorldObjectMotionAxis::Y;
		PitDefinition->MovingParts.Part1.Motion.Pivot = FVector(85.f, 0.f, -5.f);
		PitDefinition->MovingParts.Part1.Motion.Amount = -80.f;
		PitDefinition->MovingParts.Part1.Motion.Duration = 0.75f;
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
	Entry->DisplayNameOverride = FText::FromString(TEXT("Stone Pit"));
	Entry->CategoryOverride = FName(TEXT("Hazards"));
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

	UE_LOG(LogTemp, Log, TEXT("Pit trapdoor definition ensured from generic Motion: Pit=%s Palette=%s Created=%s MovingParts=%d."),
		*PitDefinition->GetPathName(), *ObjectPalette->GetPathName(), bCreated ? TEXT("true") : TEXT("false"), PitDefinition->MovingParts.NumDefined());
	return true;
#else
	OutError = TEXT("EnsurePitTrapdoorDefinition is editor-only.");
	return false;
#endif
}
