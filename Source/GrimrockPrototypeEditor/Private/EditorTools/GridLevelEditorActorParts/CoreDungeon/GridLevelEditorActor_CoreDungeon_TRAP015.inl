// TRAP01.5 — Provision discoverable, authorable dungeon traps in the normal
// WorldObject palette, using the established UGridLevelAsset placement pipeline.
// Never alter existing definitions: the user owns those authored assets.
bool AGridLevelEditorActor::EnsureDungeonTrapDefinitions(FString& OutError)
{
    OutError.Reset();
    if (!ObjectPalette)
    {
        OutError = TEXT("Cannot provision dungeon traps: ObjectPalette is not assigned.");
        return false;
    }

#if WITH_EDITOR
    struct FStarterTrap
    {
        const TCHAR* EntryId;
        const TCHAR* AssetName;
        const TCHAR* CandidateMeshName;
        const TCHAR* DisplayName;
        EGridObjectPlacementKind Surface;
        EGridDamageType DamageType;
        EGridPhysicalDamageSubtype PhysicalSubtype;
        EGridDungeonTrapDetectionSense Sense;
        int32 DetectionDC;
        int32 DisarmDC;
        int32 MinDamage;
        int32 MaxDamage;
        FVector PlaceholderScale;
    };
    static const FStarterTrap StarterTraps[] = {
        { TEXT("Trap_Floor_Spikes_01"), TEXT("DA_Trap_Floor_Spikes_01"),
          TEXT("SM_Trap_FloorDisarmable_01"), TEXT("Pointes dissimulees (sol)"),
          EGridObjectPlacementKind::Floor, EGridDamageType::Physical,
          EGridPhysicalDamageSubtype::Piercing, EGridDungeonTrapDetectionSense::Visual,
          10, 10, 6, 12, FVector(1.3f, 1.3f, 0.12f) },
        { TEXT("Trap_Wall_Darts_01"), TEXT("DA_Trap_Wall_Darts_01"),
          TEXT("SM_Trap_WallDarts_01"), TEXT("Dards dissimules (mur)"),
          EGridObjectPlacementKind::Wall, EGridDamageType::Physical,
          EGridPhysicalDamageSubtype::Piercing, EGridDungeonTrapDetectionSense::Visual,
          14, 15, 8, 14, FVector(1.2f, 0.12f, 0.65f) },
        { TEXT("Trap_Ceiling_Fire_01"), TEXT("DA_Trap_Ceiling_Fire_01"),
          TEXT("SM_Trap_CeilingFlame_01"), TEXT("Flammes dissimulees (plafond)"),
          EGridObjectPlacementKind::Ceiling, EGridDamageType::Fire,
          EGridPhysicalDamageSubtype::None, EGridDungeonTrapDetectionSense::Audible,
          18, 20, 12, 20, FVector(1.2f, 1.2f, 0.12f) }
    };

    // A low-cost engine mesh guarantees the first PIE test is possible
    // without importing any Blender asset. It appears only in EDITOR preview,
    // or in game AFTER successful detection/activation.
    UStaticMesh* PlaceholderMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!PlaceholderMesh)
    {
        OutError = TEXT("TRAP01.5: Engine basic-shapes Cube mesh is unavailable.");
        return false;
    }

    bool bPaletteChanged = false;
    TArray<UPackage*> PackagesToSave;
    for (const FStarterTrap& Spec : StarterTraps)
    {
        const FName DefinitionId(Spec.EntryId);
        const FString AssetName(Spec.AssetName);
        const FString PackagePath = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/%s"),
            Spec.AssetName);

        bool bCreated = false;
        UGridWorldObjectDefinitionAsset* Definition =
            LoadOrCreateWorldObjectDefinitionAsset(*PackagePath, *AssetName, bCreated);
        if (!Definition)
        {
            OutError = FString::Printf(TEXT("Failed to provision trap definition %s."),
                Spec.EntryId);
            return false;
        }

        if (bCreated)
        {
            Definition->Modify();
            Definition->DefinitionId = DefinitionId;
            Definition->DisplayName = FText::FromString(Spec.DisplayName);
            Definition->Description = FText::FromString(
                TEXT("Hidden dungeon hazard; visible only in the editor or after detection/activation."));
            Definition->SupportedType = EGridLevelObjectType::Trigger;
            Definition->PlacementSurface = Spec.Surface;
            Definition->DefaultLocalPosition = FGridSurfaceLocalPosition();
            Definition->bOccupiesBoundary = false; // Do not replace an authored wall.
            Definition->bReplacesStandardWall = false;
            Definition->bBlocksMovement = false;
            Definition->bIsInteractable = false; // Uses the discovered tool action, not a free click.
            Definition->bCanBeDisarmed = true;
            Definition->DisarmDifficulty = Spec.DisarmDC;
            Definition->DungeonTrap = FGridDungeonTrapDefinitionParams();
            Definition->DungeonTrap.bIsDungeonTrap = true;
            Definition->DungeonTrap.bInitiallyHidden = true;
            Definition->DungeonTrap.DetectionSense = Spec.Sense;
            Definition->DungeonTrap.DetectionDifficulty = Spec.DetectionDC;
            Definition->DungeonTrap.MaxDetectionRangeCells = 4;
            Definition->DungeonTrap.ActivationPolicy =
                EGridDungeonTrapActivationPolicy::OnEnterCell;
            Definition->DungeonTrap.bOneShot = true;
            Definition->DungeonTrap.bAffectsEntireParty = true;
            Definition->DungeonTrap.DamageType = Spec.DamageType;
            Definition->DungeonTrap.PhysicalSubtype = Spec.PhysicalSubtype;
            Definition->DungeonTrap.MinimumDamage = Spec.MinDamage;
            Definition->DungeonTrap.MaximumDamage = Spec.MaxDamage;
            Definition->DungeonTrap.RequiredToolTag = FName(TEXT("Tool.Disarm"));
            Definition->RuntimeActorClass = AGridGenericObjectActor::StaticClass();

            UStaticMesh* AuthoredMesh = FindStaticMeshByAssetName(
                FName(Spec.CandidateMeshName));
            Definition->StaticPart.Mesh = AuthoredMesh ? AuthoredMesh : PlaceholderMesh;
            Definition->StaticPart.LocalTransform = FTransform::Identity;
            if (!AuthoredMesh)
            {
                Definition->StaticPart.LocalTransform.SetScale3D(Spec.PlaceholderScale);
            }
            Definition->MovingParts.Reset();

            Definition->MarkPackageDirty();
            PackagesToSave.AddUnique(Definition->GetOutermost());
        }
        else if (Definition->DefinitionId != DefinitionId ||
                 Definition->SupportedType != EGridLevelObjectType::Trigger ||
                 !Definition->DungeonTrap.bIsDungeonTrap)
        {
            // Never silently overwrite an asset, including an invalid or legacy one.
            OutError = FString::Printf(
                TEXT("Existing trap asset %s has an incompatible contract. Repair it manually."),
                *PackagePath);
            return false;
        }

        FGridObjectPaletteEntry* Entry = ObjectPalette->Entries.FindByPredicate(
            [DefinitionId](const FGridObjectPaletteEntry& E)
            {
                return E.EntryId == DefinitionId;
            });
        if (!Entry || Entry->DefaultWorldObjectDefinition != Definition ||
            Entry->PaletteCategory != FName(TEXT("Hazards")))
        {
            if (!bPaletteChanged)
            {
                ObjectPalette->Modify();
                bPaletteChanged = true;
            }
            if (!Entry)
            {
                Entry = &ObjectPalette->Entries.AddDefaulted_GetRef();
                Entry->EntryId = DefinitionId;
            }
            Entry->DefaultWorldObjectDefinition = Definition;
            Entry->PaletteCategory = FName(TEXT("Hazards"));
        }
    }

    if (bPaletteChanged)
    {
        ObjectPalette->MarkPackageDirty();
        PackagesToSave.AddUnique(ObjectPalette->GetOutermost());
    }
    SyncPreviewRuntimeWorldObjectDefinitionsFromPalette();
    if (!PackagesToSave.IsEmpty())
    {
        UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
    }
    UE_LOG(LogGridEditor, Log,
        TEXT("[TRAP01.5] Three hidden dungeon trap definitions available in Hazards; assetsCreatedOrUpdated=%d."),
        PackagesToSave.Num());
    return true;
#else
    OutError = TEXT("TRAP01.5 provisioning is editor-only.");
    return false;
#endif
}
