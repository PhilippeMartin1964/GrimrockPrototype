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

	ConfigureStairsTransitionArchetype(*StairsUpArchetype, FName(TEXT("Stairs_Up")), TEXT("Stairs Up"), StairsUpMesh, false);
	ConfigureStairsTransitionArchetype(*StairsDownArchetype, FName(TEXT("Stairs_Down")), TEXT("Stairs Down"), StairsDownMesh, true);

	// WORLDOBJ-MIG03.4B: the Grid Editor must persist the target visual contract,
	// even while the old Configure* helpers are still being removed incrementally.
	const auto ApplyTargetStairsVisual = [](UGridObjectArchetypeAsset& Archetype, UStaticMesh* Mesh)
	{
		Archetype.PlacementSurface = EGridObjectPlacementKind::Floor;
		Archetype.DefaultLocalPosition = FGridSurfaceLocalPosition();
		Archetype.StaticPart.Mesh = Mesh;
		Archetype.StaticPart.LocalTransform = FTransform::Identity;
		Archetype.MovingParts = FGridWorldObjectMovingParts();
		Archetype.RefreshPlacementRuntimeProjection();
		Archetype.MarkPackageDirty();
	};
	ApplyTargetStairsVisual(*StairsUpArchetype, StairsUpMesh);
	ApplyTargetStairsVisual(*StairsDownArchetype, StairsDownMesh);

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

	UE_LOG(LogTemp, Log, TEXT("Stairs transition archetypes ensured: Stairs_Up=%s Stairs_Down=%s Palette=%s CreatedUp=%s CreatedDown=%s."),
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

	ConfigurePitTrapdoorArchetype(*PitArchetype, PitMesh);

	// WORLDOBJ-MIG03.4B: persist the target StaticPart/MovingParts contract.
	PitArchetype->PlacementSurface = EGridObjectPlacementKind::Floor;
	PitArchetype->DefaultLocalPosition = FGridSurfaceLocalPosition();
	PitArchetype->StaticPart.Mesh = PitMesh;
	PitArchetype->StaticPart.LocalTransform = FTransform::Identity;

	// A single target leaf is never a valid trapdoor. Reset it first, then allow
	// the one-time editor migration below to rebuild a complete pair if possible.
	if (PitArchetype->MovingParts.NumDefined() == 1)
	{
		PitArchetype->MovingParts = FGridWorldObjectMovingParts();
		UE_LOG(LogTemp, Warning,
			TEXT("WORLDOBJ-MIG03.4B: incomplete Pit MovingParts reset for %s; a Pit requires either zero or two moving leaves."),
			*PitArchetype->GetPathName());
	}

	// One-time data migration only: runtime compatibility with the old leaf fields
	// has already been removed. Copy a complete legacy pair into the target schema
	// so the asset can be resaved before those fields are physically deleted.
	if (PitArchetype->MovingParts.IsEmpty() && PitArchetype->PitLeftLeafMesh && PitArchetype->PitRightLeafMesh)
	{
		const FGridPitAnimationParams& PitAnimation = PitArchetype->DefaultBehavior.PitAnimation;
		const float OpenAngle = FMath::Abs(PitAnimation.OpenAngleDegrees);
		const float MoveDuration = FMath::Max(0.0f, PitAnimation.MoveDuration);

		PitArchetype->MovingParts.Part0.Mesh = PitArchetype->PitLeftLeafMesh.Get();
		PitArchetype->MovingParts.Part0.LocalTransform = FTransform::Identity;
		PitArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Rotation;
		PitArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Y;
		PitArchetype->MovingParts.Part0.Motion.Pivot = PitAnimation.LeftHingeLocation;
		PitArchetype->MovingParts.Part0.Motion.Amount = -OpenAngle;
		PitArchetype->MovingParts.Part0.Motion.Duration = MoveDuration;

		PitArchetype->MovingParts.Part1.Mesh = PitArchetype->PitRightLeafMesh.Get();
		PitArchetype->MovingParts.Part1.LocalTransform = FTransform::Identity;
		PitArchetype->MovingParts.Part1.Motion.Type = EGridWorldObjectMotionType::Rotation;
		PitArchetype->MovingParts.Part1.Motion.Axis = EGridWorldObjectMotionAxis::Y;
		PitArchetype->MovingParts.Part1.Motion.Pivot = PitAnimation.RightHingeLocation;
		PitArchetype->MovingParts.Part1.Motion.Amount = OpenAngle;
		PitArchetype->MovingParts.Part1.Motion.Duration = MoveDuration;

		UE_LOG(LogTemp, Log, TEXT("WORLDOBJ-MIG03.4B: migrated legacy Pit leaf pair to MovingParts for %s."), *PitArchetype->GetPathName());
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

	UE_LOG(LogTemp, Log, TEXT("Pit trapdoor archetype ensured: Pit=%s Palette=%s Created=%s."), *PitArchetype->GetPathName(),
		*ObjectPalette->GetPathName(), bCreated ? TEXT("true") : TEXT("false"));
	return true;
#else
	OutError = TEXT("EnsurePitTrapdoorArchetype is editor-only.");
	return false;
#endif
}

