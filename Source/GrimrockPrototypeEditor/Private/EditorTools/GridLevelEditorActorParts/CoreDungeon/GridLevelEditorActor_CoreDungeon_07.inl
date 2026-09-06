bool AGridLevelEditorActor::EnsureStairsTransitionArchetypes(FString& OutError)
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
	UGridObjectArchetypeAsset* StairsUpArchetype = LoadOrCreateObjectArchetypeAsset(
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Up"), TEXT("DA_Stairs_Up"), bCreatedUp);
	UGridObjectArchetypeAsset* StairsDownArchetype = LoadOrCreateObjectArchetypeAsset(
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Down"), TEXT("DA_Stairs_Down"), bCreatedDown);

	if (!StairsUpArchetype || !StairsDownArchetype)
	{
		OutError = TEXT("Failed to load or create Stairs_Up / Stairs_Down archetype assets.");
		return false;
	}

	const auto ConfigureTargetStairsTransitionArchetype = [](UGridObjectArchetypeAsset& Archetype, FName ArchetypeId, const TCHAR* DisplayName,
		UStaticMesh* Mesh, bool bHideCellFloor)
	{
		Archetype.Modify();
		Archetype.ArchetypeId = ArchetypeId;
		Archetype.DisplayName = FText::FromString(DisplayName);
		Archetype.SupportedType = EGridLevelObjectType::Decoration;
		Archetype.Description = FText::FromString(TEXT("Dungeon transition stair object."));
		Archetype.bDefaultInitiallyEnabled = true;
		Archetype.bDefaultInitiallyActive = false;
		Archetype.DefaultTag = NAME_None;
		Archetype.DefaultBehavior = FGridObjectBehaviorParams();
		Archetype.DefaultBehavior.Transition.bIsTransition = true;
		Archetype.DefaultBehavior.Transition.TargetLevelId = NAME_None;
		Archetype.DefaultBehavior.Transition.TargetCellX = 0;
		Archetype.DefaultBehavior.Transition.TargetCellY = 0;
		Archetype.DefaultBehavior.Transition.TargetFacing = EGridEdge::North;
		Archetype.DefaultBehavior.Transition.bRequireUseAction = false;
		Archetype.Category = FName(TEXT("Transitions"));
		Archetype.ObjectCategory = EGridObjectCategory::Decoration;
		Archetype.PlacementSurface = EGridObjectPlacementKind::Floor;
		Archetype.DefaultLocalPosition = FGridSurfaceLocalPosition();
		Archetype.bCanShareCell = true;
		Archetype.bCanShareAnchor = true;
		Archetype.bReplacesStandardWall = false;
		Archetype.bBlocksMovement = false;
		Archetype.bHideCellFloor = bHideCellFloor;
		Archetype.bIsInteractable = false;
		Archetype.bIsReadable = false;
		Archetype.bIsLightSource = false;
		Archetype.StaticPart.Mesh = Mesh;
		Archetype.StaticPart.LocalTransform = FTransform::Identity;
		Archetype.MovingParts = FGridWorldObjectMovingParts();
		Archetype.RuntimeActorClass = AGridGenericObjectActor::StaticClass();
		Archetype.ItemActorClass = nullptr;
		Archetype.RefreshPlacementRuntimeProjection();
		Archetype.MarkPackageDirty();
	};

	ConfigureTargetStairsTransitionArchetype(*StairsUpArchetype, FName(TEXT("Stairs_Up")), TEXT("Stairs Up"), StairsUpMesh, false);
	ConfigureTargetStairsTransitionArchetype(*StairsDownArchetype, FName(TEXT("Stairs_Down")), TEXT("Stairs Down"), StairsDownMesh, true);

	ObjectPalette->Modify();

	const auto AddOrUpdatePaletteEntry = [this](FName EntryId, const FText& DisplayName, UGridObjectArchetypeAsset* Archetype)
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
		ExistingEntry->DefaultArchetype = Archetype;
	};

	AddOrUpdatePaletteEntry(FName(TEXT("Stairs_Up")), FText::FromString(TEXT("Stairs Up")), StairsUpArchetype);
	AddOrUpdatePaletteEntry(FName(TEXT("Stairs_Down")), FText::FromString(TEXT("Stairs Down")), StairsDownArchetype);
	ObjectPalette->MarkPackageDirty();

	ResolvePreviewRuntimeActor();
	if (PreviewRuntimeActor)
	{
		PreviewRuntimeActor->Modify();
		PreviewRuntimeActor->ObjectArchetypes.AddUnique(StairsUpArchetype);
		PreviewRuntimeActor->ObjectArchetypes.AddUnique(StairsDownArchetype);
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.AddUnique(StairsUpArchetype->GetOutermost());
	PackagesToSave.AddUnique(StairsDownArchetype->GetOutermost());
	PackagesToSave.AddUnique(ObjectPalette->GetOutermost());
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	UE_LOG(LogTemp, Log, TEXT("Stairs transition archetypes ensured from target visual composition: Stairs_Up=%s Stairs_Down=%s Palette=%s CreatedUp=%s CreatedDown=%s."),
		*StairsUpArchetype->GetPathName(), *StairsDownArchetype->GetPathName(), *ObjectPalette->GetPathName(), bCreatedUp ? TEXT("true") : TEXT("false"),
		bCreatedDown ? TEXT("true") : TEXT("false"));

	return true;
#else
	OutError = TEXT("EnsureStairsTransitionArchetypes is editor-only.");
	return false;
#endif
}

bool AGridLevelEditorActor::EnsurePitTrapdoorArchetype(FString& OutError)
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
	UGridObjectArchetypeAsset* PitArchetype = LoadOrCreateObjectArchetypeAsset(
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Pit_Stone_01"), TEXT("DA_Pit_Stone_01"), bCreated);
	if (!PitArchetype)
	{
		OutError = TEXT("Failed to load or create DA_Pit_Stone_01.");
		return false;
	}

	PitArchetype->Modify();
	PitArchetype->ArchetypeId = FName(TEXT("Pit_Stone_01"));
	PitArchetype->DisplayName = FText::FromString(TEXT("Stone Pit"));
	PitArchetype->SupportedType = EGridLevelObjectType::Pit;
	PitArchetype->Description = FText::FromString(TEXT("Controlled inter-level pit with optional dual-part trapdoor cover."));
	PitArchetype->bDefaultInitiallyEnabled = true;
	PitArchetype->bDefaultInitiallyActive = false;
	PitArchetype->DefaultTag = NAME_None;
	PitArchetype->DefaultBehavior = FGridObjectBehaviorParams();
	PitArchetype->DefaultBehavior.Pit.bInitiallyOpen = true;
	PitArchetype->DefaultBehavior.Pit.bUseSameCellCoordinates = true;
	PitArchetype->DefaultBehavior.Transition.bIsTransition = true;
	PitArchetype->DefaultBehavior.Transition.TargetLevelId = NAME_None;
	PitArchetype->DefaultBehavior.Transition.TargetCellX = 0;
	PitArchetype->DefaultBehavior.Transition.TargetCellY = 0;
	PitArchetype->DefaultBehavior.Transition.TargetFacing = EGridEdge::North;
	PitArchetype->DefaultBehavior.Transition.bRequireUseAction = false;
	PitArchetype->Category = FName(TEXT("Hazards"));
	PitArchetype->ObjectCategory = EGridObjectCategory::Mechanism;
	PitArchetype->PlacementSurface = EGridObjectPlacementKind::Floor;
	PitArchetype->DefaultLocalPosition = FGridSurfaceLocalPosition();
	PitArchetype->bCanShareCell = false;
	PitArchetype->bCanShareAnchor = false;
	PitArchetype->bReplacesStandardWall = false;
	PitArchetype->bBlocksMovement = false;
	PitArchetype->bHideCellFloor = true;
	PitArchetype->bIsInteractable = false;
	PitArchetype->bIsReadable = false;
	PitArchetype->bIsLightSource = false;
	PitArchetype->StaticPart.Mesh = PitMesh;
	PitArchetype->StaticPart.LocalTransform = FTransform::Identity;
	PitArchetype->RuntimeActorClass = AGridPitTrapdoorActor::StaticClass();
	PitArchetype->ItemActorClass = nullptr;

	// WORLDOBJ-MIG04: a pit has either no moving cover or a complete Part0/Part1 pair.
	// When a complete pair exists, Motion is the sole persisted hinge/angle/duration authority.
	if (PitArchetype->MovingParts.NumDefined() == 1)
	{
		PitArchetype->MovingParts = FGridWorldObjectMovingParts();
		UE_LOG(LogTemp, Warning,
			TEXT("WORLDOBJ-MIG04: incomplete Pit MovingParts reset for %s; a Pit requires either zero or two moving parts."),
			*PitArchetype->GetPathName());
	}
	else if (PitArchetype->MovingParts.NumDefined() == 2)
	{
		PitArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Rotation;
		PitArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Y;
		PitArchetype->MovingParts.Part0.Motion.Pivot = FVector(-85.f, 0.f, -5.f);
		PitArchetype->MovingParts.Part0.Motion.Amount = -80.f;
		PitArchetype->MovingParts.Part0.Motion.Duration = 0.75f;

		PitArchetype->MovingParts.Part1.Motion.Type = EGridWorldObjectMotionType::Rotation;
		PitArchetype->MovingParts.Part1.Motion.Axis = EGridWorldObjectMotionAxis::Y;
		PitArchetype->MovingParts.Part1.Motion.Pivot = FVector(85.f, 0.f, -5.f);
		PitArchetype->MovingParts.Part1.Motion.Amount = 80.f;
		PitArchetype->MovingParts.Part1.Motion.Duration = 0.75f;
	}

	PitArchetype->RefreshPlacementRuntimeProjection();
	PitArchetype->MarkPackageDirty();

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
	Entry->DefaultArchetype = PitArchetype;
	ObjectPalette->MarkPackageDirty();

	ResolvePreviewRuntimeActor();
	if (PreviewRuntimeActor)
	{
		PreviewRuntimeActor->Modify();
		PreviewRuntimeActor->ObjectArchetypes.AddUnique(PitArchetype);
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.AddUnique(PitArchetype->GetOutermost());
	PackagesToSave.AddUnique(ObjectPalette->GetOutermost());
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	UE_LOG(LogTemp, Log, TEXT("Pit trapdoor archetype ensured from generic Motion: Pit=%s Palette=%s Created=%s MovingParts=%d."),
		*PitArchetype->GetPathName(), *ObjectPalette->GetPathName(), bCreated ? TEXT("true") : TEXT("false"), PitArchetype->MovingParts.NumDefined());
	return true;
#else
	OutError = TEXT("EnsurePitTrapdoorArchetype is editor-only.");
	return false;
#endif
}

