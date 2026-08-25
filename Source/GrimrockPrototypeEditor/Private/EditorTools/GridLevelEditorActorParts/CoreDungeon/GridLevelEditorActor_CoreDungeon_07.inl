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
