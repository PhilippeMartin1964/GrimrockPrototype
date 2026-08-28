			ItemPlacementsUsingDefinitionId,
            ItemPlacementsUsingLegacyFallback,
            Receptacles,
            ReceptaclesUsingInitialDefinition);
			}

#if WITH_EDITOR
			FString SanitizeAssetNameToken(const FString& RawName)
			{
				FString Sanitized;
				Sanitized.Reserve(RawName.Len());

				for (const TCHAR Character : RawName)
				{
					if (FChar::IsAlnum(Character) || Character == TEXT('_'))
					{
						Sanitized.AppendChar(Character);
					}
					else if (FChar::IsWhitespace(Character) || Character == TEXT('-'))
					{
						Sanitized.AppendChar(TEXT('_'));
					}
				}

				while (Sanitized.Contains(TEXT("__")))
				{
					Sanitized.ReplaceInline(TEXT("__"), TEXT("_"));
				}

				Sanitized.TrimStartAndEndInline();
				while (Sanitized.StartsWith(TEXT("_")))
				{
					Sanitized.RightChopInline(1);
				}
				while (Sanitized.EndsWith(TEXT("_")))
				{
					Sanitized.LeftChopInline(1);
				}

				return Sanitized.IsEmpty() ? FString(TEXT("New_Level")) : Sanitized;
			}

			FString MakeUniqueGridLevelPackageName(const FString& FolderPath, const FString& BaseAssetName, FString& OutAssetName)
			{
				FString CandidateAssetName = BaseAssetName;
				FString CandidatePackageName = FolderPath / CandidateAssetName;
				int32 Suffix = 1;

				while (FPackageName::DoesPackageExist(CandidatePackageName) || FindPackage(nullptr, *CandidatePackageName))
				{
					CandidateAssetName = FString::Printf(TEXT("%s_%02d"), *BaseAssetName, Suffix);
					CandidatePackageName = FolderPath / CandidateAssetName;
					++Suffix;
				}

				OutAssetName = CandidateAssetName;
				return CandidatePackageName;
			}

			UStaticMesh* FindStaticMeshByAssetName(FName AssetName)
			{
				if (AssetName.IsNone())
				{
					return nullptr;
				}

				FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

				FARFilter Filter;
				Filter.PackagePaths.Add(FName(TEXT("/Game")));
				Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
				Filter.bRecursivePaths = true;

				TArray<FAssetData> MeshAssets;
				AssetRegistryModule.Get().GetAssets(Filter, MeshAssets);

				for (const FAssetData& MeshAsset : MeshAssets)
				{
					if (MeshAsset.AssetName == AssetName)
					{
						return Cast<UStaticMesh>(MeshAsset.GetAsset());
					}
				}

				return nullptr;
			}

			UGridObjectArchetypeAsset* LoadOrCreateObjectArchetypeAsset(const TCHAR* PackageName, const TCHAR* AssetName, bool& bOutCreated)
			{
				bOutCreated = false;

				const FString ObjectPath = FString::Printf(TEXT("%s.%s"), PackageName, AssetName);
				if (UGridObjectArchetypeAsset* ExistingArchetype = LoadObject<UGridObjectArchetypeAsset>(nullptr, *ObjectPath))
				{
					return ExistingArchetype;
				}

				UPackage* Package = CreatePackage(PackageName);
				if (!Package)
				{
					return nullptr;
				}

				UGridObjectArchetypeAsset* NewArchetype = NewObject<UGridObjectArchetypeAsset>(
					Package, UGridObjectArchetypeAsset::StaticClass(), AssetName, RF_Public | RF_Standalone | RF_Transactional);

				if (NewArchetype)
				{
					FAssetRegistryModule::AssetCreated(NewArchetype);
					Package->MarkPackageDirty();
					bOutCreated = true;
				}

				return NewArchetype;
			}

			void ConfigureStairsTransitionArchetype(
				UGridObjectArchetypeAsset& Archetype, FName ArchetypeId, const TCHAR* DisplayName, UStaticMesh* Mesh, bool bHideCellFloor)
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
				Archetype.PlacementKind = EGridObjectPlacementKind::Floor;
				Archetype.bCanShareCell = true;
				Archetype.bCanShareAnchor = true;
				Archetype.bReplacesStandardWall = false;
				Archetype.bBlocksMovement = false;
				Archetype.bHideCellFloor = bHideCellFloor;
				Archetype.bIsInteractable = false;
				Archetype.bIsReadable = false;
				Archetype.bIsLightSource = false;
				Archetype.PreviewMesh = Mesh;
				Archetype.PreviewMaterial = nullptr;
				Archetype.FixedMesh = nullptr;
				Archetype.MovingMesh = nullptr;
				Archetype.FixedMaterial = nullptr;
				Archetype.MovingMaterial = nullptr;
				Archetype.RuntimeActorClass = AGridGenericObjectActor::StaticClass();
				Archetype.ItemActorClass = nullptr;
				Archetype.PlacementZOffset = 0.f;
				Archetype.WallInset = 6.f;
				Archetype.LocalOffsetAlongWall = 0.f;
				Archetype.LocalOffsetVertical = 0.f;
				Archetype.MarkPackageDirty();
			}
#endif
			}

			AGridLevelEditorActor::AGridLevelEditorActor()
			{
				PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
				bIsEditorOnlyActor = true;
#endif
				SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
				SetRootComponent(SceneRoot);
				CoordinateGridPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoordinateGridPlane"));
				CoordinateGridPlane->SetupAttachment(RootComponent);
				CoordinateGridPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				CoordinateGridPlane->SetMobility(EComponentMobility::Movable);
				CoordinateGridPlane->SetHiddenInGame(true);
			}

			void AGridLevelEditorActor::OnConstruction(const FTransform& Transform)
			{
				Super::OnConstruction(Transform);

				ResolvePreviewRuntimeActor();

				if (PreviewRuntimeActor && LevelAsset && PreviewRuntimeActor->LevelAsset != LevelAsset)
				{
					PreviewRuntimeActor->LevelAsset = LevelAsset;
				}
				UpdateCoordinateGridPlane();
				UpdateCoordinateHoverLabel();
			}

			void AGridLevelEditorActor::BeginPlay()
			{
				Super::BeginPlay();

				UWorld* World = GetWorld();
				if (!World || !World->IsGameWorld() || !bHideEditorActorDuringPIE)
				{
					return;
				}

				SetActorHiddenInGame(true);
				SetActorEnableCollision(false);

				TArray<UPrimitiveComponent*> PrimitiveComponents;
				GetComponents<UPrimitiveComponent>(PrimitiveComponents);
				for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
				{
					if (PrimitiveComponent)
					{
						PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						PrimitiveComponent->SetVisibility(false, true);
