#include "EditorTools/GridLevelEditorActor.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Components/TextRenderComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#endif

namespace
{
    struct FExpectedConcreteArchetypeSpec
    {
        const TCHAR* ArchetypeId;
        EGridLevelObjectType ExpectedType;
    };

    // Visual variants are concrete archetypes/palette entries, not EGridLevelObjectType values.
    static const FExpectedConcreteArchetypeSpec ExpectedConcreteArchetypes[] =
    {
        {TEXT ("Button_Normal"), EGridLevelObjectType::Button},
        {TEXT ("Button_Secret"), EGridLevelObjectType::Button},
        {TEXT ("Button_Wall"), EGridLevelObjectType::Button},
        {TEXT ("Door_Stone"), EGridLevelObjectType::Door},
        {TEXT ("Door_Secret"), EGridLevelObjectType::Door},
        {TEXT ("Receptacle_Alcove"), EGridLevelObjectType::Receptacle},
        {TEXT ("Receptacle_Alcove_Stone"), EGridLevelObjectType::Receptacle},
        {TEXT ("Decoration_Wall_Stone_Cracked"), EGridLevelObjectType::Decoration},
        {TEXT ("Decoration_Wall_Stone_SewerDrain"), EGridLevelObjectType::Decoration},
        {TEXT ("Receptacle_TorchHolder"), EGridLevelObjectType::Receptacle},
        {TEXT ("Receptacle_Altar"), EGridLevelObjectType::Receptacle},
        {TEXT ("Receptacle_OfferingBowl"), EGridLevelObjectType::Receptacle}
    };

    EGridWallType GetWallTypeForEdge (const FGridLevelCellData& CellData, EGridEdge Edge)
    {
        switch (Edge)
        {
            case EGridEdge::North: return CellData.NorthWall;
            case EGridEdge::East:  return CellData.EastWall;
            case EGridEdge::South: return CellData.SouthWall;
            case EGridEdge::West:  return CellData.WestWall;
            default:               return EGridWallType::None;
        }
    }

    float GetYawForOrientation (EGridEdge Orientation)
    {
        switch (Orientation)
        {
            case EGridEdge::North: return 0.f;
            case EGridEdge::East:  return 90.f;
            case EGridEdge::South: return 180.f;
            case EGridEdge::West:  return 270.f;
            default:               return 0.f;
        }
    }

    EGridLevelValidationSeverity ConvertArchetypeValidationSeverity (EGridArchetypeValidationSeverity Severity)
    {
        switch (Severity)
        {
            case EGridArchetypeValidationSeverity::Error:
                return EGridLevelValidationSeverity::Error;

            case EGridArchetypeValidationSeverity::Warning:
                return EGridLevelValidationSeverity::Warning;

            case EGridArchetypeValidationSeverity::Info:
            default:
                return EGridLevelValidationSeverity::Info;
        }
    }

    FString ToGridObjectTypeText (EGridLevelObjectType ObjectType)
    {
        if (const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ())
        {
            return TypeEnum->GetNameStringByValue (static_cast<int64> (ObjectType));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (ObjectType));
    }

    FString ToGridObjectEventText (EGridObjectEvent Event)
    {
        if (const UEnum* EventEnum = StaticEnum<EGridObjectEvent> ())
        {
            return EventEnum->GetNameStringByValue (static_cast<int64> (Event));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (Event));
    }

    FString ToGridObjectCommandText (EGridObjectCommand Command)
    {
        if (const UEnum* CommandEnum = StaticEnum<EGridObjectCommand> ())
        {
            return CommandEnum->GetNameStringByValue (static_cast<int64> (Command));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (Command));
    }

    FString ToGridObjectConditionText (EGridObjectCondition Condition)
    {
        if (const UEnum* ConditionEnum = StaticEnum<EGridObjectCondition> ())
        {
            return ConditionEnum->GetNameStringByValue (static_cast<int64> (Condition));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (Condition));
    }

    bool IsReceptacleCommand (EGridObjectCommand Command)
    {
        switch (Command)
        {
            case EGridObjectCommand::ReceptacleConsumeItem:
            case EGridObjectCommand::ReceptacleConsumeAllItems:
            case EGridObjectCommand::ReceptacleLock:
            case EGridObjectCommand::ReceptacleUnlock:
            case EGridObjectCommand::ReceptacleEnableRemoval:
            case EGridObjectCommand::ReceptacleDisableRemoval:
                return true;

            default:
                return false;
        }
    }

    bool IsEventEmittedByCurrentRuntime (EGridLevelObjectType SourceType, EGridObjectEvent Event)
    {
        switch (SourceType)
        {
            case EGridLevelObjectType::Button:
                return Event == EGridObjectEvent::Activated;

            case EGridLevelObjectType::Lever:
            case EGridLevelObjectType::PressurePlate:
            case EGridLevelObjectType::Trigger:
                return Event == EGridObjectEvent::Activated || Event == EGridObjectEvent::Deactivated;

            case EGridLevelObjectType::Receptacle:
                return Event == EGridObjectEvent::ItemInserted ||
                    Event == EGridObjectEvent::ItemRemoved ||
                    Event == EGridObjectEvent::ItemChanged;

            default:
                return false;
        }
    }

    bool IsCommandSupportedByCurrentRuntime (EGridLevelObjectType TargetType, EGridObjectCommand Command)
    {
        if (IsReceptacleCommand (Command))
        {
            return TargetType == EGridLevelObjectType::Receptacle;
        }

        const bool bStateCommand =
            Command == EGridObjectCommand::Toggle ||
            Command == EGridObjectCommand::Open ||
            Command == EGridObjectCommand::Close ||
            Command == EGridObjectCommand::Activate ||
            Command == EGridObjectCommand::Deactivate;
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
            case EGridLevelObjectType::MonsterSpawn:
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

    FString GetLevelAssetStatsText (const UGridLevelAsset* Asset)
    {
        if (!Asset)
        {
            return TEXT ("Cells=0 Objects=0 Links=0");
        }

        return FString::Printf (
            TEXT ("Cells=%d Objects=%d Links=%d"),
            Asset->Cells.Num (),
            Asset->Objects.Num (),
            Asset->Links.Num ());
    }

    FString GetGridEdgeText (EGridEdge Edge)
    {
        if (const UEnum* EdgeEnum = StaticEnum<EGridEdge> ())
        {
            return EdgeEnum->GetNameStringByValue (static_cast<int64> (Edge));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (Edge));
    }

    FString GetLevelStartText (const UGridLevelAsset* Asset)
    {
        if (!Asset)
        {
            return TEXT ("Cell=None Facing=None Valid=false");
        }

        return FString::Printf (
            TEXT ("Cell=(%d,%d) Facing=%s Valid=%s"),
            Asset->StartCellX,
            Asset->StartCellY,
            *GetGridEdgeText (Asset->StartFacing),
            Asset->IsStartCellValid () ? TEXT ("true") : TEXT ("false"));
    }

    FString GetObjectWorkflowAssetName (const UObject* Object)
    {
        return Object ? Object->GetName () : TEXT ("None");
    }

    FString GetItemPlacementWorkflowStatus (const FGridLevelObjectData& Object)
    {
        if (Object.ItemDefinitionAsset)
        {
            const FName AssetId = Object.ItemDefinitionAsset->ItemDefinitionId;
            if (!Object.ItemDefinitionId.IsNone () && Object.ItemDefinitionId != AssetId)
            {
                return TEXT ("ERROR_CONFLICTING_DEFINITIONS");
            }
            if (!Object.ArchetypeId.IsNone () && !AssetId.IsNone () && Object.ArchetypeId != AssetId)
            {
                return TEXT ("ERROR_CONFLICTING_DEFINITIONS");
            }
            return TEXT ("OK_ITEM_DEFINITION_ASSET");
        }

        if (!Object.ItemDefinitionId.IsNone ())
        {
            if (!Object.ArchetypeId.IsNone () && Object.ArchetypeId != Object.ItemDefinitionId)
            {
                return TEXT ("ERROR_CONFLICTING_DEFINITIONS");
            }
            return TEXT ("OK_ITEM_DEFINITION_ID");
        }

        if (!Object.ArchetypeId.IsNone ())
        {
            return TEXT ("LEGACY_ARCHETYPE_FALLBACK");
        }

        return TEXT ("ERROR_NO_ITEM_DEFINITION");
    }

    FString GetReceptacleWorkflowStatus (const FGridReceptacleBehaviorParams& Receptacle)
    {
        const bool bHasDefinitionAsset = Receptacle.InitialContainedItemDefinition != nullptr;
        const bool bHasDefinitionId = !Receptacle.InitialContainedItemDefinitionId.IsNone ();
        const bool bHasLegacyArchetype = !Receptacle.InitialContainedItemArchetypeId.IsNone ();

        if (bHasDefinitionAsset)
        {
            const FName AssetId = Receptacle.InitialContainedItemDefinition->ItemDefinitionId;
            if ((bHasDefinitionId && Receptacle.InitialContainedItemDefinitionId != AssetId) ||
                (bHasLegacyArchetype && !AssetId.IsNone () && Receptacle.InitialContainedItemArchetypeId != AssetId))
            {
                return TEXT ("ERROR_CONFLICTING_DEFINITIONS");
            }
            return TEXT ("OK_INITIAL_ITEM_DEFINITION_ASSET");
        }

        if (bHasDefinitionId)
        {
            if (bHasLegacyArchetype && Receptacle.InitialContainedItemArchetypeId != Receptacle.InitialContainedItemDefinitionId)
            {
                return TEXT ("ERROR_CONFLICTING_DEFINITIONS");
            }
            return TEXT ("OK_INITIAL_ITEM_DEFINITION_ID");
        }

        if (bHasLegacyArchetype)
        {
            return TEXT ("LEGACY_CONTAINED_ARCHETYPE_FALLBACK");
        }

        return TEXT ("EMPTY_RECEPTACLE");
    }

    void AppendItemWorkflowDiagnosticsForLevel (FString& Result, const UGridLevelAsset* Asset, const FString& LevelLabel)
    {
        if (!Asset)
        {
            Result += FString::Printf (TEXT ("Level=%s Status=ERROR_MISSING_LEVEL_ASSET\n"), *LevelLabel);
            return;
        }

        int32 ItemPlacements = 0;
        int32 ItemPlacementsUsingDefinitionAsset = 0;
        int32 ItemPlacementsUsingDefinitionId = 0;
        int32 ItemPlacementsUsingLegacyFallback = 0;
        int32 Receptacles = 0;
        int32 ReceptaclesUsingInitialDefinition = 0;
        int32 ReceptaclesUsingLegacyContainedItem = 0;

        Result += FString::Printf (TEXT ("Level=%s Asset=%s\n"), *LevelLabel, *Asset->GetName ());

        for (const FGridLevelObjectData& Object : Asset->Objects)
        {
            if (Object.Type == EGridLevelObjectType::Item)
            {
                ++ItemPlacements;
                const FString Status = GetItemPlacementWorkflowStatus (Object);
                if (Status == TEXT ("OK_ITEM_DEFINITION_ASSET"))
                {
                    ++ItemPlacementsUsingDefinitionAsset;
                }
                else if (Status == TEXT ("OK_ITEM_DEFINITION_ID"))
                {
                    ++ItemPlacementsUsingDefinitionId;
                }
                else if (Status == TEXT ("LEGACY_ARCHETYPE_FALLBACK"))
                {
                    ++ItemPlacementsUsingLegacyFallback;
                }

                Result += FString::Printf (
                    TEXT ("  Item ObjectId=%s Cell=(%d,%d) ArchetypeId=%s ItemDefinitionAsset=%s ItemDefinitionId=%s Status=%s\n"),
                    *Object.ObjectId.ToString (),
                    Object.CellX,
                    Object.CellY,
                    *Object.ArchetypeId.ToString (),
                    *GetObjectWorkflowAssetName (Object.ItemDefinitionAsset),
                    *Object.ItemDefinitionId.ToString (),
                    *Status);
            }

            if (Object.Type == EGridLevelObjectType::Receptacle)
            {
                ++Receptacles;
                const FGridReceptacleBehaviorParams& Receptacle = Object.Behavior.Receptacle;
                const FString Status = GetReceptacleWorkflowStatus (Receptacle);
                if (Status == TEXT ("OK_INITIAL_ITEM_DEFINITION_ASSET") || Status == TEXT ("OK_INITIAL_ITEM_DEFINITION_ID"))
                {
                    ++ReceptaclesUsingInitialDefinition;
                }
                else if (Status == TEXT ("LEGACY_CONTAINED_ARCHETYPE_FALLBACK"))
                {
                    ++ReceptaclesUsingLegacyContainedItem;
                }

                Result += FString::Printf (
                    TEXT ("  Receptacle ObjectId=%s ArchetypeId=%s InitialContainedItemDefinition=%s InitialContainedItemDefinitionId=%s LegacyInitialContainedItemArchetypeId=%s Status=%s\n"),
                    *Object.ObjectId.ToString (),
                    *Object.ArchetypeId.ToString (),
                    *GetObjectWorkflowAssetName (Receptacle.InitialContainedItemDefinition),
                    *Receptacle.InitialContainedItemDefinitionId.ToString (),
                    *Receptacle.InitialContainedItemArchetypeId.ToString (),
                    *Status);
            }
        }

        Result += FString::Printf (
            TEXT ("  ItemDefinitionWorkflow: ItemPlacements=%d ItemPlacementsUsingDefinitionAsset=%d ItemPlacementsUsingDefinitionId=%d ItemPlacementsUsingLegacyFallback=%d Receptacles=%d ReceptaclesUsingInitialDefinition=%d ReceptaclesUsingLegacyContainedItem=%d\n"),
            ItemPlacements,
            ItemPlacementsUsingDefinitionAsset,
            ItemPlacementsUsingDefinitionId,
            ItemPlacementsUsingLegacyFallback,
            Receptacles,
            ReceptaclesUsingInitialDefinition,
            ReceptaclesUsingLegacyContainedItem);
    }

#if WITH_EDITOR
    FString SanitizeAssetNameToken (const FString& RawName)
    {
        FString Sanitized;
        Sanitized.Reserve (RawName.Len ());

        for (const TCHAR Character : RawName)
        {
            if (FChar::IsAlnum (Character) || Character == TEXT ('_'))
            {
                Sanitized.AppendChar (Character);
            }
            else if (FChar::IsWhitespace (Character) || Character == TEXT ('-'))
            {
                Sanitized.AppendChar (TEXT ('_'));
            }
        }

        while (Sanitized.Contains (TEXT ("__")))
        {
            Sanitized.ReplaceInline (TEXT ("__"), TEXT ("_"));
        }

        Sanitized.TrimStartAndEndInline ();
        while (Sanitized.StartsWith (TEXT ("_")))
        {
            Sanitized.RightChopInline (1);
        }
        while (Sanitized.EndsWith (TEXT ("_")))
        {
            Sanitized.LeftChopInline (1);
        }

        return Sanitized.IsEmpty () ? FString (TEXT ("New_Level")) : Sanitized;
    }

    FString MakeUniqueGridLevelPackageName (const FString& FolderPath, const FString& BaseAssetName, FString& OutAssetName)
    {
        FString CandidateAssetName = BaseAssetName;
        FString CandidatePackageName = FolderPath / CandidateAssetName;
        int32 Suffix = 1;

        while (FPackageName::DoesPackageExist (CandidatePackageName) || FindPackage (nullptr, *CandidatePackageName))
        {
            CandidateAssetName = FString::Printf (TEXT ("%s_%02d"), *BaseAssetName, Suffix);
            CandidatePackageName = FolderPath / CandidateAssetName;
            ++Suffix;
        }

        OutAssetName = CandidateAssetName;
        return CandidatePackageName;
    }

    UStaticMesh* FindStaticMeshByAssetName (FName AssetName)
    {
        if (AssetName.IsNone ())
        {
            return nullptr;
        }

        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule> (TEXT ("AssetRegistry"));

        FARFilter Filter;
        Filter.PackagePaths.Add (FName (TEXT ("/Game")));
        Filter.ClassPaths.Add (UStaticMesh::StaticClass ()->GetClassPathName ());
        Filter.bRecursivePaths = true;

        TArray<FAssetData> MeshAssets;
        AssetRegistryModule.Get ().GetAssets (Filter, MeshAssets);

        for (const FAssetData& MeshAsset : MeshAssets)
        {
            if (MeshAsset.AssetName == AssetName)
            {
                return Cast<UStaticMesh> (MeshAsset.GetAsset ());
            }
        }

        return nullptr;
    }

    UGridObjectArchetypeAsset* LoadOrCreateObjectArchetypeAsset (const TCHAR* PackageName, const TCHAR* AssetName, bool& bOutCreated)
    {
        bOutCreated = false;

        const FString ObjectPath = FString::Printf (TEXT ("%s.%s"), PackageName, AssetName);
        if (UGridObjectArchetypeAsset* ExistingArchetype = LoadObject<UGridObjectArchetypeAsset> (nullptr, *ObjectPath))
        {
            return ExistingArchetype;
        }

        UPackage* Package = CreatePackage (PackageName);
        if (!Package)
        {
            return nullptr;
        }

        UGridObjectArchetypeAsset* NewArchetype = NewObject<UGridObjectArchetypeAsset> (
            Package,
            UGridObjectArchetypeAsset::StaticClass (),
            AssetName,
            RF_Public | RF_Standalone | RF_Transactional);

        if (NewArchetype)
        {
            FAssetRegistryModule::AssetCreated (NewArchetype);
            Package->MarkPackageDirty ();
            bOutCreated = true;
        }

        return NewArchetype;
    }

    void ConfigureStairsTransitionArchetype (
        UGridObjectArchetypeAsset& Archetype,
        FName ArchetypeId,
        const TCHAR* DisplayName,
        UStaticMesh* Mesh,
        bool bHideCellFloor)
    {
        Archetype.Modify ();
        Archetype.ArchetypeId = ArchetypeId;
        Archetype.DisplayName = FText::FromString (DisplayName);
        Archetype.SupportedType = EGridLevelObjectType::Decoration;
        Archetype.Description = FText::FromString (TEXT ("Dungeon transition stair object."));
        Archetype.bDefaultInitiallyEnabled = true;
        Archetype.bDefaultInitiallyActive = false;
        Archetype.DefaultTag = NAME_None;
        Archetype.DefaultBehavior = FGridObjectBehaviorParams ();
        Archetype.DefaultBehavior.Transition.bIsTransition = true;
        Archetype.DefaultBehavior.Transition.TargetLevelId = NAME_None;
        Archetype.DefaultBehavior.Transition.TargetCellX = 0;
        Archetype.DefaultBehavior.Transition.TargetCellY = 0;
        Archetype.DefaultBehavior.Transition.TargetFacing = EGridEdge::North;
        Archetype.DefaultBehavior.Transition.bRequireUseAction = false;
        Archetype.Category = FName (TEXT ("Transitions"));
        Archetype.ObjectCategory = EGridObjectCategory::Decoration;
        Archetype.PlacementKind = EGridObjectPlacementKind::Floor;
        Archetype.bPlaceOnEdge = false;
        Archetype.bPlaceAtCellCenter = true;
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
        Archetype.RuntimeActorClass = AGridGenericObjectActor::StaticClass ();
        Archetype.ItemActorClass = nullptr;
        Archetype.PlacementZOffset = 0.f;
        Archetype.WallInset = 6.f;
        Archetype.LocalOffsetAlongWall = 0.f;
        Archetype.LocalOffsetVertical = 0.f;
        Archetype.MarkPackageDirty ();
    }
#endif
}

AGridLevelEditorActor::AGridLevelEditorActor ()
{
    PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
    bIsEditorOnlyActor = true;
#endif
    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("SceneRoot"));
    SetRootComponent (SceneRoot);
    CoordinateGridPlane = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("CoordinateGridPlane"));
    CoordinateGridPlane->SetupAttachment (RootComponent);
    CoordinateGridPlane->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    CoordinateGridPlane->SetMobility (EComponentMobility::Movable);
    CoordinateGridPlane->SetHiddenInGame (true);
}

void AGridLevelEditorActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);

    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor && LevelAsset && PreviewRuntimeActor->LevelAsset != LevelAsset)
    {
        PreviewRuntimeActor->LevelAsset = LevelAsset;
    }
    UpdateCoordinateGridPlane ();
    UpdateCoordinateHoverLabel ();
}

void AGridLevelEditorActor::BeginPlay ()
{
    Super::BeginPlay ();

    UWorld* World = GetWorld ();
    if (!World || !World->IsGameWorld () || !bHideEditorActorDuringPIE)
    {
        return;
    }

    SetActorHiddenInGame (true);
    SetActorEnableCollision (false);

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    GetComponents<UPrimitiveComponent> (PrimitiveComponents);
    for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
    {
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
            PrimitiveComponent->SetVisibility (false, true);
        }
    }

    if (CoordinateGridPlane)
    {
        CoordinateGridPlane->SetHiddenInGame (true, true);
        CoordinateGridPlane->SetVisibility (false, true);
    }
}

#if WITH_EDITOR
void AGridLevelEditorActor::PostEditChangeProperty (FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty (PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName () : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateGridPlaneMesh)
        || PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateGridMaterial)
        || PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, bShowCoordinateGrid)
        || PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateGridZOffset))
    {
        UpdateCoordinateGridPlane ();
        return;
    }
    if (PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, bShowCoordinateLabels)
        || PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateLabelWorldSize))
    {
        UpdateCoordinateHoverLabel ();
        return;
    }
}
#endif

bool AGridLevelEditorActor::HasValidLevelAsset () const
{
    return LevelAsset != nullptr;
}

bool AGridLevelEditorActor::IsValidSelectedCell () const
{
    return LevelAsset && LevelAsset->IsValidCoord (SelectedCellX, SelectedCellY);
}

bool AGridLevelEditorActor::RequiresEdge (EGridLevelObjectType ObjectType) const
{
    switch (ObjectType)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
        case EGridLevelObjectType::Receptacle:
            return true;
        default:
            return false;
    }
}

bool AGridLevelEditorActor::IsEdgePlacedObject (const FGridLevelObjectData& ObjectData) const
{
    return IsEdgePlacedObject (ObjectData.Type, ObjectData.ArchetypeId);
}

bool AGridLevelEditorActor::IsEdgePlacedObject (EGridLevelObjectType ObjectType, FName ArchetypeId) const
{
    if (ObjectType == EGridLevelObjectType::Item && ArchetypeId == FName (TEXT ("Item_Torch")))
    {
        return true;
    }

    if (const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (ArchetypeId))
    {
        return Archetype->IsEdgePlaced () || Archetype->IsWallPlaced ();
    }

    return RequiresEdge (ObjectType);
}

bool AGridLevelEditorActor::IsCellCenteredObject (EGridLevelObjectType ObjectType) const
{
    switch (ObjectType)
    {
        case EGridLevelObjectType::PressurePlate:
        case EGridLevelObjectType::MonsterSpawn:
        case EGridLevelObjectType::ItemSpawn:
        case EGridLevelObjectType::Item:
        case EGridLevelObjectType::Light:
        case EGridLevelObjectType::Teleporter:
        case EGridLevelObjectType::Trigger:
        case EGridLevelObjectType::Decoration:
            return true;
        default:
            return false;
    }
}

const UGridObjectArchetypeAsset* AGridLevelEditorActor::FindObjectArchetypeById (FName ArchetypeId) const
{
    if (ArchetypeId.IsNone () || !ObjectPalette)
    {
        return nullptr;
    }

    for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
    {
        if (Entry.DefaultArchetype && Entry.DefaultArchetype->ArchetypeId == ArchetypeId)
        {
            return Entry.DefaultArchetype;
        }
    }

    return nullptr;
}

bool AGridLevelEditorActor::SetSelectedObjectOrientation (EGridEdge Orientation)
{
    if (Orientation == EGridEdge::None || !LevelAsset || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

    FGridLevelObjectData* SelectedObject = FindSelectedObjectMutable ();
    if (!SelectedObject)
    {
        return false;
    }

    const bool bUsesEdge = IsEdgePlacedObject (*SelectedObject);
    if (bUsesEdge)
    {
        const FGuid SelectedObjectId = SelectedObject->ObjectId;
        const EGridLevelObjectType SelectedObjectType = SelectedObject->Type;
        const bool bDestinationOccupied = LevelAsset->Objects.ContainsByPredicate (
            [SelectedObjectId, SelectedObjectType, SelectedObject, Orientation] (const FGridLevelObjectData& Obj)
        {
            return Obj.ObjectId != SelectedObjectId &&
                Obj.CellX == SelectedObject->CellX &&
                Obj.CellY == SelectedObject->CellY &&
                Obj.Type == SelectedObjectType &&
                Obj.Edge == Orientation;
        });

        if (bDestinationOccupied)
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot orient selected object, destination edge is occupied."));
            return false;
        }
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    if (bUsesEdge)
    {
        SelectedObject->Edge = Orientation;
        SelectedEdge = Orientation;
    }
    else
    {
        SelectedObject->LocalYaw = GetYawForOrientation (Orientation);
    }

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

FGridLevelCellData* AGridLevelEditorActor::GetSelectedCellMutable ()
{
    if (!IsValidSelectedCell ())
    {
        return nullptr;
    }

    return &LevelAsset->GetCellMutable (SelectedCellX, SelectedCellY);
}

EGridWallType* AGridLevelEditorActor::GetSelectedWallMutable (FGridLevelCellData& CellData)
{
    switch (SelectedEdge)
    {
        case EGridEdge::North: return &CellData.NorthWall;
        case EGridEdge::East:  return &CellData.EastWall;
        case EGridEdge::South: return &CellData.SouthWall;
        case EGridEdge::West:  return &CellData.WestWall;
        default:               return nullptr;
    }
}

void AGridLevelEditorActor::ResolvePreviewRuntimeActor ()
{
    if (!PreviewRuntimeActor)
    {
        PreviewRuntimeActor = Cast<AGridLevelRuntimeActor> (
            UGameplayStatics::GetActorOfClass (GetWorld (), AGridLevelRuntimeActor::StaticClass ()));
    }
}

FString AGridLevelEditorActor::GetEditorRuntimeAssetConsistencyDiagnostics () const
{
    const UWorld* World = GetWorld ();
    const UGridLevelAsset* PreviewLevelAsset = PreviewRuntimeActor ? PreviewRuntimeActor->LevelAsset.Get () : nullptr;

    FString Result;
    Result += TEXT ("GridLevelEditorActor Asset Consistency\n");
    Result += FString::Printf (TEXT ("EditorActor: %s\n"), *GetName ());
    Result += FString::Printf (TEXT ("World: %s\n"), World ? *World->GetMapName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("DungeonAsset: %s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("CurrentDungeonLevelId: %s\n"), *CurrentDungeonLevelId.ToString ());
    Result += FString::Printf (TEXT ("Editor LevelAsset: %s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("Editor Asset Stats: %s\n"), *GetLevelAssetStatsText (LevelAsset));
    Result += FString::Printf (TEXT ("Editor Start: %s\n"), *GetLevelStartText (LevelAsset));
    Result += FString::Printf (TEXT ("PreviewRuntimeActor: %s\n"), PreviewRuntimeActor ? *PreviewRuntimeActor->GetName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("Preview Runtime LevelAsset: %s\n"), PreviewLevelAsset ? *PreviewLevelAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("Preview Asset Stats: %s\n"), *GetLevelAssetStatsText (PreviewLevelAsset));
    Result += FString::Printf (TEXT ("Preview Start: %s\n"), *GetLevelStartText (PreviewLevelAsset));

    if (!LevelAsset)
    {
        Result += TEXT ("Status: ERROR - EditorActor LevelAsset is null.");
    }
    else if (!PreviewRuntimeActor)
    {
        Result += TEXT ("Status: ERROR - PreviewRuntimeActor is null.");
    }
    else if (!PreviewLevelAsset)
    {
        Result += TEXT ("Status: ERROR - PreviewRuntimeActor LevelAsset is null.");
    }
    else if (LevelAsset == PreviewLevelAsset)
    {
        Result += TEXT ("Status: OK - Editor and PreviewRuntimeActor use the same LevelAsset.");
    }
    else
    {
        Result += TEXT ("Status: WARNING - Editor and PreviewRuntimeActor use different LevelAssets.");
    }

    return Result;
}

void AGridLevelEditorActor::LogEditorRuntimeAssetConsistency () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetEditorRuntimeAssetConsistencyDiagnostics ());
}

FString AGridLevelEditorActor::GetItemWorkflowDiagnostics () const
{
    FString Result;
    Result += TEXT ("Grid ItemDefinition Workflow Diagnostics\n");
    Result += FString::Printf (TEXT ("EditorActor=%s\n"), *GetName ());
    Result += FString::Printf (TEXT ("DungeonAsset=%s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("LevelAsset=%s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));

    if (DungeonAsset && DungeonAsset->Levels.Num () > 0)
    {
        for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
        {
            if (!Entry.bEnabled)
            {
                continue;
            }
            const FString LevelLabel = Entry.LevelId.IsNone ()
                ? FString (TEXT ("None"))
                : Entry.LevelId.ToString ();
            AppendItemWorkflowDiagnosticsForLevel (Result, Entry.LevelAsset, LevelLabel);
        }
        return Result;
    }

    AppendItemWorkflowDiagnosticsForLevel (Result, LevelAsset, TEXT ("CurrentLevel"));
    return Result;
}

void AGridLevelEditorActor::LogItemWorkflowDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetItemWorkflowDiagnostics ());
}

FString AGridLevelEditorActor::GetDungeonDiagnostics () const
{
    FString Result;
    Result += TEXT ("GridLevelEditorActor Dungeon Diagnostics\n");
    Result += FString::Printf (TEXT ("EditorActor: %s\n"), *GetName ());
    Result += FString::Printf (TEXT ("DungeonAsset: %s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("CurrentDungeonLevelId: %s\n"), *CurrentDungeonLevelId.ToString ());
    Result += FString::Printf (TEXT ("Current LevelAsset: %s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));

    if (!DungeonAsset)
    {
        Result += TEXT ("Status: WARNING - DungeonAsset is null. Editor is using LevelAsset directly.");
        return Result;
    }

    Result += DungeonAsset->GetDungeonDiagnostics ();

    const FGridDungeonLevelEntry* CurrentEntry = DungeonAsset->FindLevelEntry (CurrentDungeonLevelId);
    if (!CurrentEntry)
    {
        Result += TEXT ("\nCurrentSelectionStatus: WARNING - CurrentDungeonLevelId was not found.");
    }
    else if (!CurrentEntry->bEnabled)
    {
        Result += TEXT ("\nCurrentSelectionStatus: WARNING - Current dungeon level is disabled.");
    }
    else if (!CurrentEntry->LevelAsset)
    {
        Result += TEXT ("\nCurrentSelectionStatus: ERROR - Current dungeon level has no LevelAsset.");
    }
    else
    {
        Result += TEXT ("\nCurrentSelectionStatus: OK");
    }

    return Result;
}

void AGridLevelEditorActor::LogDungeonDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetDungeonDiagnostics ());
}

void AGridLevelEditorActor::LogDungeonTransitionDiagnostics () const
{
    if (!DungeonAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("LogDungeonTransitionDiagnostics failed: DungeonAsset is null."));
        return;
    }

    UE_LOG (LogTemp, Log, TEXT ("%s"), *DungeonAsset->GetTransitionDiagnostics ());
}

bool AGridLevelEditorActor::CreateAndAddDungeonLevel (
    FName NewLevelId,
    FText DisplayName,
    FIntVector LogicalPosition,
    FString& OutError)
{
    OutError.Reset ();

    if (!DungeonAsset)
    {
        OutError = TEXT ("DungeonAsset is null.");
        return false;
    }

    if (NewLevelId.IsNone ())
    {
        OutError = TEXT ("Level Id is empty.");
        return false;
    }

    for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
    {
        if (Entry.LevelId == NewLevelId)
        {
            OutError = FString::Printf (TEXT ("Level Id '%s' already exists."), *NewLevelId.ToString ());
            return false;
        }

        if (Entry.LogicalPosition == LogicalPosition)
        {
            OutError = FString::Printf (
                TEXT ("Logical Position (%d,%d,%d) is already used by LevelId '%s'."),
                LogicalPosition.X,
                LogicalPosition.Y,
                LogicalPosition.Z,
                *Entry.LevelId.ToString ());
            return false;
        }
    }

#if WITH_EDITOR
    const FString LevelAssetFolderPath = TEXT ("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels");
    const FString SanitizedLevelId = SanitizeAssetNameToken (NewLevelId.ToString ());
    const FString BaseAssetName = FString::Printf (TEXT ("DA_GridLevel_%s"), *SanitizedLevelId);

    FString AssetName;
    const FString PackageName = MakeUniqueGridLevelPackageName (LevelAssetFolderPath, BaseAssetName, AssetName);

    UPackage* Package = CreatePackage (*PackageName);
    if (!Package)
    {
        OutError = FString::Printf (TEXT ("Failed to create package '%s'."), *PackageName);
        return false;
    }

    UGridLevelAsset* NewLevelAsset = NewObject<UGridLevelAsset> (
        Package,
        UGridLevelAsset::StaticClass (),
        *AssetName,
        RF_Public | RF_Standalone | RF_Transactional);

    if (!NewLevelAsset)
    {
        OutError = FString::Printf (TEXT ("Failed to create UGridLevelAsset '%s'."), *AssetName);
        return false;
    }

    NewLevelAsset->Modify ();
    NewLevelAsset->Width = 32;
    NewLevelAsset->Height = 32;
    NewLevelAsset->CellSize = 200.f;
    NewLevelAsset->EnsureCellCount ();
    NewLevelAsset->Objects.Reset ();
    NewLevelAsset->Links.Reset ();
    NewLevelAsset->StartCellX = 1;
    NewLevelAsset->StartCellY = 1;
    NewLevelAsset->StartFacing = EGridEdge::North;

    if (NewLevelAsset->IsValidCoord (NewLevelAsset->StartCellX, NewLevelAsset->StartCellY))
    {
        FGridLevelCellData& StartCell = NewLevelAsset->GetCellMutable (NewLevelAsset->StartCellX, NewLevelAsset->StartCellY);
        StartCell.CellType = EGridCellType::Floor;
        StartCell.bHasCeiling = true;
        StartCell.bBlocksOccupancy = false;
    }

    DungeonAsset->Modify ();
    const FName PreviousDefaultLevelId = DungeonAsset->DefaultLevelId;
    const FName PreviousCurrentDungeonLevelId = CurrentDungeonLevelId;
    UGridLevelAsset* PreviousLevelAsset = LevelAsset;

    FGridDungeonLevelEntry NewEntry;
    NewEntry.LevelId = NewLevelId;
    NewEntry.DisplayName = DisplayName.IsEmpty () ? FText::FromName (NewLevelId) : DisplayName;
    NewEntry.LevelAsset = NewLevelAsset;
    NewEntry.LogicalPosition = LogicalPosition;
    NewEntry.bEnabled = true;
    DungeonAsset->Levels.Add (NewEntry);

    if (DungeonAsset->DefaultLevelId.IsNone ())
    {
        DungeonAsset->DefaultLevelId = NewLevelId;
    }

    Modify ();
    CurrentDungeonLevelId = NewLevelId;
    LevelAsset = NewLevelAsset;
    if (!ApplyCurrentDungeonLevel ())
    {
        DungeonAsset->Levels.RemoveAll ([NewLevelId] (const FGridDungeonLevelEntry& Entry)
        {
            return Entry.LevelId == NewLevelId;
        });
        DungeonAsset->DefaultLevelId = PreviousDefaultLevelId;
        CurrentDungeonLevelId = PreviousCurrentDungeonLevelId;
        LevelAsset = PreviousLevelAsset;
        NewLevelAsset->ClearFlags (RF_Public | RF_Standalone);

        OutError = FString::Printf (
            TEXT ("Level '%s' was created but could not be applied to the editor actor."),
            *NewLevelId.ToString ());
        UE_LOG (LogTemp, Error, TEXT ("%s"), *OutError);
        return false;
    }

    FAssetRegistryModule::AssetCreated (NewLevelAsset);
    Package->MarkPackageDirty ();
    DungeonAsset->MarkPackageDirty ();

    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add (Package);
    PackagesToSave.Add (DungeonAsset->GetOutermost ());
    UEditorLoadingAndSavingUtils::SavePackages (PackagesToSave, false);

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("Created dungeon level %s at LogicalPosition=(%d,%d,%d), Asset=%s."),
        *NewLevelId.ToString (),
        LogicalPosition.X,
        LogicalPosition.Y,
        LogicalPosition.Z,
        *NewLevelAsset->GetPathName ());

    return true;
#else
    OutError = TEXT ("CreateAndAddDungeonLevel is editor-only.");
    return false;
#endif
}

bool AGridLevelEditorActor::EnsureStairsTransitionArchetypes (FString& OutError)
{
    OutError.Reset ();

    if (!ObjectPalette)
    {
        OutError = TEXT ("ObjectPalette is null.");
        return false;
    }

#if WITH_EDITOR
    UStaticMesh* StairsUpMesh = FindStaticMeshByAssetName (FName (TEXT ("SM_Stairs_Up_01")));
    UStaticMesh* StairsDownMesh = FindStaticMeshByAssetName (FName (TEXT ("SM_Stairs_Down_01")));

    if (!StairsUpMesh || !StairsDownMesh)
    {
        OutError = FString::Printf (
            TEXT ("Missing stair mesh asset(s): SM_Stairs_Up_01=%s SM_Stairs_Down_01=%s."),
            StairsUpMesh ? TEXT ("OK") : TEXT ("Missing"),
            StairsDownMesh ? TEXT ("OK") : TEXT ("Missing"));
        UE_LOG (LogTemp, Error, TEXT ("%s"), *OutError);
        return false;
    }

    bool bCreatedUp = false;
    bool bCreatedDown = false;
    UGridObjectArchetypeAsset* StairsUpArchetype = LoadOrCreateObjectArchetypeAsset (
        TEXT ("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Up"),
        TEXT ("DA_Stairs_Up"),
        bCreatedUp);
    UGridObjectArchetypeAsset* StairsDownArchetype = LoadOrCreateObjectArchetypeAsset (
        TEXT ("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Down"),
        TEXT ("DA_Stairs_Down"),
        bCreatedDown);

    if (!StairsUpArchetype || !StairsDownArchetype)
    {
        OutError = TEXT ("Failed to load or create Stairs_Up / Stairs_Down archetype assets.");
        return false;
    }

    ConfigureStairsTransitionArchetype (*StairsUpArchetype, FName (TEXT ("Stairs_Up")), TEXT ("Stairs Up"), StairsUpMesh, false);
    ConfigureStairsTransitionArchetype (*StairsDownArchetype, FName (TEXT ("Stairs_Down")), TEXT ("Stairs Down"), StairsDownMesh, true);

    ObjectPalette->Modify ();

    const auto AddOrUpdatePaletteEntry = [this] (FName EntryId, const FText& DisplayName, UGridObjectArchetypeAsset* Archetype)
    {
        FGridObjectPaletteEntry* ExistingEntry = ObjectPalette->Entries.FindByPredicate (
            [EntryId] (const FGridObjectPaletteEntry& Entry)
        {
            return Entry.EntryId == EntryId;
        });

        if (!ExistingEntry)
        {
            ExistingEntry = &ObjectPalette->Entries.AddDefaulted_GetRef ();
        }

        ExistingEntry->EntryId = EntryId;
        ExistingEntry->DisplayNameOverride = DisplayName;
        ExistingEntry->CategoryOverride = FName (TEXT ("Transitions"));
        ExistingEntry->DefaultArchetype = Archetype;
    };

    AddOrUpdatePaletteEntry (FName (TEXT ("Stairs_Up")), FText::FromString (TEXT ("Stairs Up")), StairsUpArchetype);
    AddOrUpdatePaletteEntry (FName (TEXT ("Stairs_Down")), FText::FromString (TEXT ("Stairs Down")), StairsDownArchetype);
    ObjectPalette->MarkPackageDirty ();

    ResolvePreviewRuntimeActor ();
    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->Modify ();
        PreviewRuntimeActor->ObjectArchetypes.AddUnique (StairsUpArchetype);
        PreviewRuntimeActor->ObjectArchetypes.AddUnique (StairsDownArchetype);
    }

    TArray<UPackage*> PackagesToSave;
    PackagesToSave.AddUnique (StairsUpArchetype->GetOutermost ());
    PackagesToSave.AddUnique (StairsDownArchetype->GetOutermost ());
    PackagesToSave.AddUnique (ObjectPalette->GetOutermost ());
    UEditorLoadingAndSavingUtils::SavePackages (PackagesToSave, false);

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("Stairs transition archetypes ensured: Stairs_Up=%s Stairs_Down=%s Palette=%s CreatedUp=%s CreatedDown=%s."),
        *StairsUpArchetype->GetPathName (),
        *StairsDownArchetype->GetPathName (),
        *ObjectPalette->GetPathName (),
        bCreatedUp ? TEXT ("true") : TEXT ("false"),
        bCreatedDown ? TEXT ("true") : TEXT ("false"));

    return true;
#else
    OutError = TEXT ("EnsureStairsTransitionArchetypes is editor-only.");
    return false;
#endif
}

bool AGridLevelEditorActor::ApplyCurrentDungeonLevel ()
{
    if (!DungeonAsset)
    {
        UE_LOG (LogTemp, Warning, TEXT ("ApplyCurrentDungeonLevel failed: DungeonAsset is null."));
        return false;
    }

    const FName RequestedLevelId = CurrentDungeonLevelId.IsNone ()
        ? DungeonAsset->DefaultLevelId
        : CurrentDungeonLevelId;
    if (RequestedLevelId.IsNone ())
    {
        UE_LOG (LogTemp, Error,
            TEXT ("ApplyCurrentDungeonLevel failed: CurrentDungeonLevelId and DefaultLevelId are both None."));
        return false;
    }

    const FGridDungeonLevelEntry* Entry = DungeonAsset->FindLevelEntry (RequestedLevelId);
    if (!Entry)
    {
        UE_LOG (LogTemp, Error, TEXT ("ApplyCurrentDungeonLevel failed: LevelId %s was not found."), *RequestedLevelId.ToString ());
        return false;
    }

    if (!Entry->bEnabled)
    {
        UE_LOG (LogTemp, Error, TEXT ("ApplyCurrentDungeonLevel failed: LevelId %s is disabled."), *RequestedLevelId.ToString ());
        return false;
    }

    if (!Entry->LevelAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("ApplyCurrentDungeonLevel failed: LevelId %s has no LevelAsset."), *RequestedLevelId.ToString ());
        return false;
    }

#if WITH_EDITOR
    Modify ();
#endif

    CurrentDungeonLevelId = RequestedLevelId;
    LevelAsset = Entry->LevelAsset;

    SyncPreviewRuntimeLevelAsset ();

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("ApplyCurrentDungeonLevel OK: LevelId=%s LevelAsset=%s."),
        *CurrentDungeonLevelId.ToString (),
        *GetNameSafe (LevelAsset));
    return true;
}

void AGridLevelEditorActor::ApplyCurrentDungeonLevelInEditor ()
{
    ApplyCurrentDungeonLevel ();
}

void AGridLevelEditorActor::LoadDefaultDungeonLevelInEditor ()
{
    if (!DungeonAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("LoadDefaultDungeonLevel failed: DungeonAsset is null."));
        return;
    }

    if (DungeonAsset->IsValidLevelId (DungeonAsset->DefaultLevelId))
    {
#if WITH_EDITOR
        Modify ();
#endif
        CurrentDungeonLevelId = DungeonAsset->DefaultLevelId;
        UE_LOG (
            LogTemp,
            Log,
            TEXT ("LoadDefaultDungeonLevel: loading DefaultLevelId %s."),
            *CurrentDungeonLevelId.ToString ());
        ApplyCurrentDungeonLevel ();
        return;
    }

    for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
    {
        if (Entry.bEnabled && !Entry.LevelId.IsNone () && Entry.LevelAsset)
        {
#if WITH_EDITOR
            Modify ();
#endif
            CurrentDungeonLevelId = Entry.LevelId;
            UE_LOG (
                LogTemp,
                Warning,
                TEXT ("LoadDefaultDungeonLevel: DefaultLevelId %s is not valid; loading first enabled level %s."),
                *DungeonAsset->DefaultLevelId.ToString (),
                *CurrentDungeonLevelId.ToString ());
            ApplyCurrentDungeonLevel ();
            return;
        }
    }

    UE_LOG (
        LogTemp,
        Error,
        TEXT ("LoadDefaultDungeonLevel failed: DungeonAsset %s has no enabled level with a LevelAsset."),
        *DungeonAsset->GetPathName ());
}

void AGridLevelEditorActor::SyncPreviewRuntimeLevelAsset ()
{
    ResolvePreviewRuntimeActor ();

    if (!LevelAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("GridLevelEditorActor: cannot sync PreviewRuntimeActor because LevelAsset is null."));
        return;
    }

    if (!PreviewRuntimeActor)
    {
        UE_LOG (LogTemp, Error, TEXT ("GridLevelEditorActor: cannot sync LevelAsset because PreviewRuntimeActor is null."));
        return;
    }

#if WITH_EDITOR
    PreviewRuntimeActor->Modify ();
#endif
    PreviewRuntimeActor->LevelAsset = LevelAsset;
    PreviewRuntimeActor->DungeonAsset = DungeonAsset;
    PreviewRuntimeActor->CurrentDungeonLevelId = CurrentDungeonLevelId;
    SyncPreviewRuntimeObjectArchetypesFromPalette ();
    PreviewRuntimeActor->RebuildLevel ();

    LogEditorRuntimeAssetConsistency ();
}

void AGridLevelEditorActor::PreparePIETestFromStart ()
{
    FString Error;
    if (!PreparePIETestFromStartInternal (Error))
    {
        UE_LOG (LogTemp, Error, TEXT ("PreparePIETestFromStart failed: %s"), *Error);
    }
}

bool AGridLevelEditorActor::PreparePIETestFromStartInternal (FString& OutError)
{
    OutError.Reset ();

    if (!LevelAsset)
    {
        OutError = TEXT ("LevelAsset is null.");
        return false;
    }

    if (!LevelAsset->IsStartCellValid ())
    {
        OutError = FString::Printf (
            TEXT ("Start cell is invalid: X=%d Y=%d Facing=%s."),
            LevelAsset->StartCellX,
            LevelAsset->StartCellY,
            *GetGridEdgeText (LevelAsset->StartFacing));
        return false;
    }

    if (DungeonAsset)
    {
        if (CurrentDungeonLevelId.IsNone ())
        {
            OutError = TEXT ("CurrentDungeonLevelId is None while DungeonAsset is assigned.");
            return false;
        }

        if (!DungeonAsset->IsValidLevelId (CurrentDungeonLevelId))
        {
            OutError = FString::Printf (
                TEXT ("CurrentDungeonLevelId '%s' is not an enabled level with a LevelAsset in DungeonAsset %s."),
                *CurrentDungeonLevelId.ToString (),
                *DungeonAsset->GetPathName ());
            return false;
        }

        UGridLevelAsset* DungeonLevelAsset = DungeonAsset->GetLevelAssetById (CurrentDungeonLevelId);
        if (DungeonLevelAsset != LevelAsset)
        {
            OutError = FString::Printf (
                TEXT ("CurrentDungeonLevelId '%s' resolves to %s but EditorActor.LevelAsset is %s. Apply Current Dungeon Level before PIE."),
                *CurrentDungeonLevelId.ToString (),
                DungeonLevelAsset ? *DungeonLevelAsset->GetPathName () : TEXT ("None"),
                *LevelAsset->GetPathName ());
            return false;
        }
    }

    ResolvePreviewRuntimeActor ();

    if (!PreviewRuntimeActor)
    {
        OutError = TEXT ("PreviewRuntimeActor is missing.");
        return false;
    }

#if WITH_EDITOR
    PreviewRuntimeActor->Modify ();
#endif
    PreviewRuntimeActor->bApplyLevelStartOnBeginPlay = true;
    PreviewRuntimeActor->LevelAsset = LevelAsset;
    PreviewRuntimeActor->DungeonAsset = DungeonAsset;
    PreviewRuntimeActor->CurrentDungeonLevelId = CurrentDungeonLevelId;
    PreviewRuntimeActor->RebuildLevel ();
    PreviewRuntimeActor->LogPIEReadinessDiagnostics ();

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("PreparePIETestFromStart OK: %s is ready to test LevelAsset %s from StartCell X=%d Y=%d Facing=%s."),
        *GetNameSafe (PreviewRuntimeActor),
        *GetNameSafe (LevelAsset),
        LevelAsset->StartCellX,
        LevelAsset->StartCellY,
        *GetGridEdgeText (LevelAsset->StartFacing));

    return true;
}

void AGridLevelEditorActor::SetStartFromSelection ()
{
    if (!LevelAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("GridLevelEditorActor: cannot set start from selection because LevelAsset is null."));
        return;
    }

    if (!LevelAsset->IsValidCoord (SelectedCellX, SelectedCellY))
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("GridLevelEditorActor: cannot set start from invalid selection X=%d Y=%d."),
            SelectedCellX,
            SelectedCellY);
        return;
    }

    const FGridLevelCellData& SelectedCell = LevelAsset->GetCell (SelectedCellX, SelectedCellY);
    if (SelectedCell.CellType == EGridCellType::Empty || SelectedCell.bBlocksOccupancy)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("GridLevelEditorActor: cannot set start from non-walkable selection X=%d Y=%d."),
            SelectedCellX,
            SelectedCellY);
        return;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    LevelAsset->StartCellX = SelectedCellX;
    LevelAsset->StartCellY = SelectedCellY;
    LevelAsset->StartFacing = SelectedEdge == EGridEdge::None ? EGridEdge::North : SelectedEdge;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("GridLevelEditorActor: level start set to X=%d Y=%d Facing=%s."),
        LevelAsset->StartCellX,
        LevelAsset->StartCellY,
        *GetGridEdgeText (LevelAsset->StartFacing));

    LogEditorRuntimeAssetConsistency ();
}

FVector AGridLevelEditorActor::GetSelectedCellWorldCenter (float ZOffset) const
{
    if (PreviewRuntimeActor)
    {
        return PreviewRuntimeActor->GetCellCenterWorld (SelectedCellX, SelectedCellY, ZOffset);
    }
    const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
    return GetActorLocation () + FVector::ZeroVector +
        FVector ((SelectedCellX * CellSize) + (CellSize * 0.5f), (SelectedCellY * CellSize) + (CellSize * 0.5f), ZOffset);
}

void AGridLevelEditorActor::EnsureLevelReady ()
{
    if (!HasValidLevelAsset ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: LevelAsset is null."));
        return;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    LevelAsset->EnsureCellCount ();
    LevelAsset->EnsureObjectIds ();

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
}

void AGridLevelEditorActor::RebuildPreview ()
{
    ResolvePreviewRuntimeActor ();
    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->LevelAsset = LevelAsset;
        SyncPreviewRuntimeObjectArchetypesFromPalette ();
        PreviewRuntimeActor->RebuildLevel ();
    }
}

void AGridLevelEditorActor::SyncPreviewRuntimeObjectArchetypesFromPalette ()
{
    if (!PreviewRuntimeActor || !ObjectPalette)
    {
        return;
    }

#if WITH_EDITOR
    PreviewRuntimeActor->Modify ();
#endif

    for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
    {
        if (Entry.DefaultArchetype)
        {
            PreviewRuntimeActor->ObjectArchetypes.AddUnique (Entry.DefaultArchetype);
        }
    }
}

void AGridLevelEditorActor::ClearSelectedCell ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid selected cell."));
        return;
    }
#if WITH_EDITOR
    LevelAsset->Modify ();
#endif
    * CellData = FGridLevelCellData ();
    RemoveObjectsAtSelectionInternal (false);
#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif
    RebuildGeometryPreview ();
}

void AGridLevelEditorActor::PaintSelectedWall ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid selected cell."));
        return;
    }
    if (CellData->CellType == EGridCellType::Empty)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot paint wall on empty cell."));
        return;
    }
    EGridWallType* WallPtr = GetSelectedWallMutable (*CellData);
    if (!WallPtr)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: SelectedEdge must be North/East/South/West."));
        return;
    }
#if WITH_EDITOR
    if (*WallPtr == PaintWallType)
    {
        return;
    }
    LevelAsset->Modify ();
#endif
    // Shared walls are stored per cell. Do not mirror to the neighboring opposite edge.
    * WallPtr = PaintWallType;
#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif
    RebuildGeometryPreview ();
}

void AGridLevelEditorActor::ClearSelectedWall ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid selected cell."));
        return;
    }

    EGridWallType* WallPtr = GetSelectedWallMutable (*CellData);
    if (!WallPtr)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: SelectedEdge must be North/East/South/West."));
        return;
    }

#if WITH_EDITOR
    if (*WallPtr == EGridWallType::None)
    {
        return;
    }
    LevelAsset->Modify ();
#endif

    // Keep the directional wall rule consistent with painting, rendering and movement.
    * WallPtr = EGridWallType::None;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildGeometryPreview ();
}

int32 AGridLevelEditorActor::RemoveObjectsAtSelectionInternal (bool bSameTypeOnly)
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return 0;
    }
#if WITH_EDITOR
    LevelAsset->Modify ();
#endif
    TArray<FGuid> RemovedIds;
    const EGridLevelObjectType FilterType = PaintObjectType;
    for (int32 Index = LevelAsset->Objects.Num () - 1; Index >= 0; --Index)
    {
        const FGridLevelObjectData& Obj = LevelAsset->Objects[Index];
        if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
        {
            continue;
        }
        if (bSameTypeOnly && Obj.Type != FilterType)
        {
            continue;
        }
        bool bRemove = false;
        if (IsEdgePlacedObject (Obj))
        {
            bRemove = (Obj.Edge == SelectedEdge);
        } else
        {
            bRemove = true;
        }
        if (bRemove)
        {
            RemovedIds.Add (Obj.ObjectId);
            LevelAsset->Objects.RemoveAt (Index);
        }
    }
    if (RemovedIds.Num () > 0)
    {
        LevelAsset->Links.RemoveAll ([&] (const FGridObjectLink& Link)
        {
            return RemovedIds.Contains (Link.SourceObjectId) || RemovedIds.Contains (Link.TargetObjectId);
        });
    }

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    return RemovedIds.Num ();
}

int32 AGridLevelEditorActor::RemoveObjectsConflictingWithPlacementInternal (
    EGridLevelObjectType NewObjectType,
    FName NewArchetypeId,
    bool bNewObjectOnEdge)
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return 0;
    }
    const UGridObjectArchetypeAsset* NewArchetype = FindObjectArchetypeById (NewArchetypeId);
    const bool bNewCanShareCell = NewArchetype ? NewArchetype->bCanShareCell : true;
    const bool bNewCanShareAnchor = NewArchetype ? NewArchetype->bCanShareAnchor : true;
    TArray<int32> IndicesToRemove;
    TArray<FGuid> RemovedIds;
    for (int32 Index = LevelAsset->Objects.Num () - 1; Index >= 0; --Index)
    {
        const FGridLevelObjectData& ExistingObject = LevelAsset->Objects[Index];
        if (ExistingObject.CellX != SelectedCellX || ExistingObject.CellY != SelectedCellY)
        {
            continue;
        }
        const UGridObjectArchetypeAsset* ExistingArchetype = FindObjectArchetypeById (ExistingObject.ArchetypeId);
        const bool bExistingCanShareCell = ExistingArchetype ? ExistingArchetype->bCanShareCell : true;
        const bool bExistingCanShareAnchor = ExistingArchetype ? ExistingArchetype->bCanShareAnchor : true;
        const bool bExistingObjectOnEdge = IsEdgePlacedObject (ExistingObject);
        const bool bSameAnchor = bNewObjectOnEdge && bExistingObjectOnEdge ? ExistingObject.Edge == SelectedEdge : !bNewObjectOnEdge && !bExistingObjectOnEdge;
        bool bShouldRemove = false;
        if (!bNewCanShareCell || !bExistingCanShareCell)
        {
            bShouldRemove = true;
        } else if (bSameAnchor && (!bNewCanShareAnchor || !bExistingCanShareAnchor))
        {
            bShouldRemove = true;
        }
        if (bShouldRemove)
        {
            RemovedIds.Add (ExistingObject.ObjectId);
            IndicesToRemove.Add (Index);
        }
    }
    if (IndicesToRemove.Num () == 0)
    {
        return 0;
    }
#if WITH_EDITOR
    LevelAsset->Modify ();
#endif
    for (int32 IndexToRemove : IndicesToRemove)
    {
        LevelAsset->Objects.RemoveAt (IndexToRemove);
    }
    LevelAsset->Links.RemoveAll ([&] (const FGridObjectLink& Link)
    {
        return RemovedIds.Contains (Link.SourceObjectId) || RemovedIds.Contains (Link.TargetObjectId);
    });
#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif
    return RemovedIds.Num ();
}

void AGridLevelEditorActor::PlaceSelectedObject ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid LevelAsset or selected cell."));
        return;
    }
    if (PaintObjectType == EGridLevelObjectType::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: PaintObjectType is None."));
        return;
    }
    const UGridObjectArchetypeAsset* ObjectArchetype = FindObjectArchetypeById (ObjectArchetypeId);
    const bool bIsWallReplacingObject = ObjectArchetype && ObjectArchetype->bReplacesStandardWall;
    const bool bIsStoneAlcoveReceptacle = ObjectArchetypeId == FName (TEXT ("Receptacle_Alcove_Stone"));
    const bool bPlaceObjectOnEdge = bIsWallReplacingObject || IsEdgePlacedObject (PaintObjectType, ObjectArchetypeId);
    if (bPlaceObjectOnEdge && SelectedEdge == EGridEdge::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: this object type requires a valid edge."));
        return;
    }
    if (ObjectArchetype)
    {
        RemoveObjectsConflictingWithPlacementInternal (PaintObjectType, ObjectArchetypeId, bPlaceObjectOnEdge);
    } else
    {
        if (PlacementPolicy == EGridEditorObjectPlacementPolicy::ReplaceSameSlotOnly)
        {
            RemoveObjectsAtSelectionInternal (true);
        } else
        {
            RemoveObjectsAtSelectionInternal (false);
        }
    }
    FGridLevelObjectData NewObject;
    NewObject.Type = PaintObjectType;
    NewObject.CellX = SelectedCellX;
    NewObject.CellY = SelectedCellY;
    NewObject.Edge = bPlaceObjectOnEdge ? SelectedEdge : EGridEdge::None;
    NewObject.LocalYaw = 0.f;
    NewObject.ArchetypeId = ObjectArchetypeId;
    NewObject.bInitiallyEnabled = bObjectInitiallyEnabled;
    NewObject.bInitiallyActive = bObjectInitiallyActive;
    NewObject.Tag = ObjectTag;
    NewObject.Notes = ObjectNotes;
    NewObject.PaletteEntryId = SelectedPaletteEntryId;
    NewObject.Behavior = ObjectBehavior;

    if (bIsStoneAlcoveReceptacle)
    {
        NewObject.Type = EGridLevelObjectType::Receptacle;
        NewObject.bInitiallyEnabled = true;
        NewObject.bInitiallyActive = !NewObject.Behavior.Receptacle.InitialContainedItemArchetypeId.IsNone ();
    }

    if (bIsWallReplacingObject)
    {
        if (FGridLevelCellData* CellData = GetSelectedCellMutable ())
        {
            if (EGridWallType* WallPtr = GetSelectedWallMutable (*CellData))
            {
#if WITH_EDITOR
                LevelAsset->Modify ();
#endif
                *WallPtr = EGridWallType::Solid;
#if WITH_EDITOR
                LevelAsset->MarkPackageDirty ();
#endif
            }
        }
    }

    const FGuid NewId = LevelAsset->AddObject (NewObject);
    LastSelectedObjectId = NewId;
    RebuildPreview ();
}

void AGridLevelEditorActor::RemoveObjectsAtSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid LevelAsset or selected cell."));
        return;
    }

    RemoveObjectsAtSelectionInternal (false);
    LastSelectedObjectId.Invalidate ();
    RebuildPreview ();
}

void AGridLevelEditorActor::SelectObjectAtSelection ()
{
    ClearSelectedObjectState ();

    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid LevelAsset or selected cell."));
        return;
    }

    for (int32 Index = LevelAsset->Objects.Num () - 1; Index >= 0; --Index)
    {
        const FGridLevelObjectData& Obj = LevelAsset->Objects[Index];

        if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
        {
            continue;
        }

        if (IsEdgePlacedObject (Obj) && Obj.Edge != SelectedEdge)
        {
            continue;
        }
        LastSelectedObjectId = Obj.ObjectId;
        PaintObjectType = Obj.Type;
        SelectedEdge = Obj.Edge;
        bObjectInitiallyEnabled = Obj.bInitiallyEnabled;
        bObjectInitiallyActive = Obj.bInitiallyActive;
        ObjectArchetypeId = Obj.ArchetypeId;
        ObjectTag = Obj.Tag;
        ObjectNotes = Obj.Notes;
        SelectedPaletteEntryId = Obj.PaletteEntryId;
        ObjectBehavior = Obj.Behavior;
        return;
    }

    ClearSelectedObjectState ();
    UE_LOG (LogTemp, Log, TEXT ("GridLevelEditorActor: no object found at current selection."));
}

bool AGridLevelEditorActor::TryConvertWorldHitToSelection (const FVector& WorldHitLocation, const FVector& /*HitNormal*/)
{
    return ApplyGridHoverFromWorldPoint (WorldHitLocation) && CommitHoveredCellSelection ();
}

bool AGridLevelEditorActor::ApplyViewportHitSelection (const FVector& WorldHitLocation, const FVector& HitNormal)
{
    return TryConvertWorldHitToSelection (WorldHitLocation, HitNormal);
}

bool AGridLevelEditorActor::IsSelectionValidForEditing () const
{
    return HasValidLevelAsset () && IsValidSelectedCell ();
}

bool AGridLevelEditorActor::SelectCellFromOverview (int32 CellX, int32 CellY)
{
    if (!HasValidLevelAsset () || !LevelAsset->IsValidCoord (CellX, CellY))
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("GridLevelEditorActor: overview cell selection is outside grid bounds X=%d Y=%d."),
            CellX,
            CellY);
        return false;
    }

    Modify ();
    SelectedCellX = CellX;
    SelectedCellY = CellY;
    SelectedEdge = EGridEdge::None;
    HoveredCellX = CellX;
    HoveredCellY = CellY;
    HoveredEdge = EGridEdge::None;
    UpdateCoordinateHoverLabel ();
    return true;
}

EGridEdge AGridLevelEditorActor::GetEdgeFromPointInCell (const FVector2D& LocalInCell, float CellSize) const
{
    const float DistNorth = FMath::Abs (CellSize - LocalInCell.Y);
    const float DistEast = FMath::Abs (CellSize - LocalInCell.X);
    const float DistSouth = FMath::Abs (LocalInCell.Y);
    const float DistWest = FMath::Abs (LocalInCell.X);

    float BestDist = DistNorth;
    EGridEdge BestEdge = EGridEdge::North;

    if (DistEast < BestDist)
    {
        BestDist = DistEast;
        BestEdge = EGridEdge::East;
    }

    if (DistSouth < BestDist)
    {
        BestDist = DistSouth;
        BestEdge = EGridEdge::South;
    }

    if (DistWest < BestDist)
    {
        BestDist = DistWest;
        BestEdge = EGridEdge::West;
    }

    return BestEdge;
}

bool AGridLevelEditorActor::ApplyGridHoverFromWorldPoint (const FVector& WorldPoint)
{
    if (!HasValidLevelAsset ())
    {
        HoveredCellX = INDEX_NONE;
        HoveredCellY = INDEX_NONE;
        HoveredEdge = EGridEdge::None;
        UpdateCoordinateHoverLabel ();
        return false;
    }

    ResolvePreviewRuntimeActor ();

    const float CellSize = LevelAsset->CellSize;
    if (CellSize <= KINDA_SMALL_NUMBER)
    {
        HoveredCellX = INDEX_NONE;
        HoveredCellY = INDEX_NONE;
        HoveredEdge = EGridEdge::None;
        UpdateCoordinateHoverLabel ();
        return false;
    }

    FVector GridWorldOrigin = FVector::ZeroVector;
    if (PreviewRuntimeActor)
    {
        GridWorldOrigin = PreviewRuntimeActor->GetActorLocation () + PreviewRuntimeActor->GridOrigin;
    }

    const FVector Local = WorldPoint - GridWorldOrigin;

    const int32 NewCellX = FMath::FloorToInt (Local.X / CellSize);
    const int32 NewCellY = FMath::FloorToInt (Local.Y / CellSize);

    if (!LevelAsset->IsValidCoord (NewCellX, NewCellY))
    {
        HoveredCellX = INDEX_NONE;
        HoveredCellY = INDEX_NONE;
        HoveredEdge = EGridEdge::None;
        UpdateCoordinateHoverLabel ();
        return false;
    }

    const float LocalInCellX = Local.X - (static_cast<float>(NewCellX) * CellSize);
    const float LocalInCellY = Local.Y - (static_cast<float>(NewCellY) * CellSize);

    HoveredCellX = NewCellX;
    HoveredCellY = NewCellY;
    HoveredEdge = GetEdgeFromPointInCell (FVector2D (LocalInCellX, LocalInCellY), CellSize);
    UpdateCoordinateHoverLabel ();
    return true;
}

bool AGridLevelEditorActor::CommitHoveredCellSelection ()
{
    if (!HasValidLevelAsset () || !LevelAsset->IsValidCoord (HoveredCellX, HoveredCellY))
    {
        return false;
    }

    const bool bSelectionChanged =
        SelectedCellX != HoveredCellX ||
        SelectedCellY != HoveredCellY ||
        SelectedEdge != HoveredEdge;

    if (bSelectionChanged)
    {
        Modify ();
        SelectedCellX = HoveredCellX;
        SelectedCellY = HoveredCellY;
        SelectedEdge = HoveredEdge;
    }

    return true;
}

FVector AGridLevelEditorActor::GetSelectionPreviewCenter (float ZOffset) const
{
    return GetSelectedCellWorldCenter (ZOffset);
}

void AGridLevelEditorActor::ApplyPrimaryToolAction ()
{
    switch (ActiveTool)
    {
        case EGridEditorTool::Select:
            if (!SelectHoveredObject ())
            {
                SelectObjectAtSelection ();
            }
            break;

        case EGridEditorTool::PaintCell:
            PaintSelectedCell ();
            break;

        case EGridEditorTool::PaintWall:
            PaintSelectedWall ();
            break;

        case EGridEditorTool::PaintObject:
            PlaceSelectedObject ();
            break;

        case EGridEditorTool::Erase:
            EraseAtSelection ();
            break;

        case EGridEditorTool::Link:
            SelectHoveredObject ();
            BeginOrCompleteLinkAtSelection ();
            break;

        default:
            break;
    }
}

void AGridLevelEditorActor::ApplySecondaryToolAction ()
{
    switch (ActiveTool)
    {
        case EGridEditorTool::PaintCell:
            ClearSelectedCell ();
            break;

        case EGridEditorTool::PaintWall:
            ClearSelectedWall ();
            break;

        case EGridEditorTool::PaintObject:
            RemoveObjectsAtSelection ();
            break;

        case EGridEditorTool::Link:
            ClearPendingLinkSource ();
            break;

        case EGridEditorTool::Select:
        case EGridEditorTool::Erase:
        default:
            break;
    }
}

const FGridLevelObjectData* AGridLevelEditorActor::FindObjectAtSelection () const
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return nullptr;
    }
    for (int32 Index = LevelAsset->Objects.Num () - 1; Index >= 0; --Index)
    {
        const FGridLevelObjectData& Obj = LevelAsset->Objects[Index];

        if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
        {
            continue;
        }

        if (IsEdgePlacedObject (Obj) && Obj.Edge != SelectedEdge)
        {
            continue;
        }

        return &Obj;
    }
    return nullptr;
}

const FGridLevelObjectData* AGridLevelEditorActor::FindObjectById (const FGuid& ObjectId) const
{
    if (!HasValidLevelAsset () || !ObjectId.IsValid ())
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.ObjectId == ObjectId)
        {
            return &Obj;
        }
    }
    return nullptr;
}

FGridLevelObjectData* AGridLevelEditorActor::FindSelectedObjectMutable ()
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return nullptr;
    }

    return LevelAsset->Objects.FindByPredicate (
        [this] (const FGridLevelObjectData& Obj)
    {
        return Obj.ObjectId == LastSelectedObjectId;
    });
}

bool AGridLevelEditorActor::TryGetObjectWorldLocation (
    const FGridLevelObjectData& ObjectData,
    FVector& OutWorldLocation) const
{
    if (!HasValidLevelAsset ())
    {
        return false;
    }
    const float CellSize = LevelAsset->CellSize;
    FVector GridWorldOrigin = FVector::ZeroVector;
    if (PreviewRuntimeActor)
    {
        GridWorldOrigin = PreviewRuntimeActor->GetActorLocation () + PreviewRuntimeActor->GridOrigin;
    }

    const FVector CellCenter = GridWorldOrigin + FVector (
        (ObjectData.CellX * CellSize) + (CellSize * 0.5f),
        (ObjectData.CellY * CellSize) + (CellSize * 0.5f),
        12.f);

    if (IsEdgePlacedObject (ObjectData))
    {
        switch (ObjectData.Edge)
        {
            case EGridEdge::North:
                OutWorldLocation = CellCenter + FVector (0.f, CellSize * 0.5f, 0.f);
                return true;

            case EGridEdge::East:
                OutWorldLocation = CellCenter + FVector (CellSize * 0.5f, 0.f, 0.f);
                return true;

            case EGridEdge::South:
                OutWorldLocation = CellCenter + FVector (0.f, -CellSize * 0.5f, 0.f);
                return true;

            case EGridEdge::West:
                OutWorldLocation = CellCenter + FVector (-CellSize * 0.5f, 0.f, 0.f);
                return true;

            default:
                return false;
        }
    }

    OutWorldLocation = CellCenter;
    return true;
}

bool AGridLevelEditorActor::TryGetSelectedObjectWorldLocation (FVector& OutWorldLocation) const
{
    const FGridLevelObjectData* Obj = FindObjectAtSelection ();
    return Obj ? TryGetObjectWorldLocation (*Obj, OutWorldLocation) : false;
}

bool AGridLevelEditorActor::TryGetPendingLinkSourceLocation (FVector& OutWorldLocation) const
{
    if (!bHasPendingLinkSource || !PendingLinkSourceObjectId.IsValid ())
    {
        return false;
    }

    const FGridLevelObjectData* Obj = FindObjectById (PendingLinkSourceObjectId);
    return Obj ? TryGetObjectWorldLocation (*Obj, OutWorldLocation) : false;
}

bool AGridLevelEditorActor::HasPendingLinkSource () const
{
    return bHasPendingLinkSource && PendingLinkSourceObjectId.IsValid ();
}

void AGridLevelEditorActor::ClearPendingLinkSource ()
{
    bHasPendingLinkSource = false;
    PendingLinkSourceObjectId.Invalidate ();
}

bool AGridLevelEditorActor::BeginOrCompleteLinkAtSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return false;
    }

    const FGridLevelObjectData* SelectedObject = FindObjectAtSelection ();
    if (!SelectedObject)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: no object at selection for link mode."));
        return false;
    }

    if (!bHasPendingLinkSource)
    {
        PendingLinkSourceObjectId = SelectedObject->ObjectId;
        bHasPendingLinkSource = true;
        LastSelectedObjectId = SelectedObject->ObjectId;

        UE_LOG (
            LogTemp,
            Log,
            TEXT ("GridLevelEditorActor: link source set to %s"),
            *SelectedObject->ObjectId.ToString ());

        return true;
    }

    if (PendingLinkSourceObjectId == SelectedObject->ObjectId)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: source and target are identical."));
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const bool bAlreadyExists = LevelAsset->Links.ContainsByPredicate (
        [&] (const FGridObjectLink& Link)
    {
        return Link.SourceObjectId == PendingLinkSourceObjectId &&
            Link.TargetObjectId == SelectedObject->ObjectId &&
            Link.SourceEvent == LinkSourceEvent &&
            Link.Command == LinkCommand;
    });

    if (!bAlreadyExists)
    {
        FGridObjectLink NewLink;
        NewLink.SourceObjectId = PendingLinkSourceObjectId;
        NewLink.TargetObjectId = SelectedObject->ObjectId;
        NewLink.SourceEvent = LinkSourceEvent;
        NewLink.Command = LinkCommand;
        LevelAsset->Links.Add (NewLink);

#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif

        UE_LOG (
            LogTemp,
            Log,
            TEXT ("GridLevelEditorActor: link created %s -> %s"),
            *PendingLinkSourceObjectId.ToString (),
            *SelectedObject->ObjectId.ToString ());
    }

    LastSelectedObjectId = SelectedObject->ObjectId;
    ClearPendingLinkSource ();
    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::RemoveLinksAtSelection ()
{
    if (!HasValidLevelAsset ())
    {
        return false;
    }

    const FGridLevelObjectData* SelectedObject = FindObjectAtSelection ();
    if (!SelectedObject)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const int32 RemovedCount = LevelAsset->Links.RemoveAll (
        [&] (const FGridObjectLink& Link)
    {
        return Link.SourceObjectId == SelectedObject->ObjectId ||
            Link.TargetObjectId == SelectedObject->ObjectId;
    });

    if (RemovedCount > 0)
    {
#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif
        RebuildPreview ();
        return true;
    }
    return false;
}

bool AGridLevelEditorActor::ApplyPaletteEntry (FName EntryId)
{
    if (!ObjectPalette)
    {
        return false;
    }

    const FGridObjectPaletteEntry* Entry = ObjectPalette->FindEntryById (EntryId);
    if (!Entry || !Entry->DefaultArchetype)
    {
        return false;
    }

    SelectedPaletteEntryId = Entry->EntryId;
    PaintObjectType = Entry->DefaultArchetype->SupportedType;
    ObjectArchetypeId = Entry->DefaultArchetype->ArchetypeId;
    SelectedArchetypeId = Entry->DefaultArchetype->ArchetypeId;
    bObjectInitiallyEnabled = Entry->DefaultArchetype->bDefaultInitiallyEnabled;
    bObjectInitiallyActive = Entry->DefaultArchetype->bDefaultInitiallyActive;
    ObjectTag = Entry->DefaultArchetype->DefaultTag;
    ObjectBehavior = Entry->DefaultArchetype->DefaultBehavior;

    return true;
}

void AGridLevelEditorActor::ApplySelectedPaletteEntry ()
{
    ApplyPaletteEntry (SelectedPaletteEntryId);
}

bool AGridLevelEditorActor::ApplyEditedSelectedObject ()
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    for (FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.ObjectId != LastSelectedObjectId)
        {
            continue;
        }

        Obj.Type = PaintObjectType;
        Obj.Edge = IsEdgePlacedObject (PaintObjectType, ObjectArchetypeId) ? SelectedEdge : EGridEdge::None;
        Obj.ArchetypeId = ObjectArchetypeId;
        Obj.PaletteEntryId = SelectedPaletteEntryId;
        Obj.bInitiallyEnabled = bObjectInitiallyEnabled;
        Obj.bInitiallyActive = bObjectInitiallyActive;
        Obj.Tag = ObjectTag;
        Obj.Notes = ObjectNotes;
        Obj.Behavior = ObjectBehavior;

#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif

        RebuildPreview ();
        return true;
    }

    return false;
}

bool AGridLevelEditorActor::RemoveLinkByIndexForSelectedObject (int32 LinkIndex)
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

    int32 CurrentIndex = 0;

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    for (int32 Index = 0; Index < LevelAsset->Links.Num (); ++Index)
    {
        const FGridObjectLink& Link = LevelAsset->Links[Index];

        if (Link.SourceObjectId != LastSelectedObjectId &&
            Link.TargetObjectId != LastSelectedObjectId)
        {
            continue;
        }

        if (CurrentIndex == LinkIndex)
        {
            LevelAsset->Links.RemoveAt (Index);

#if WITH_EDITOR
            LevelAsset->MarkPackageDirty ();
#endif

            RebuildPreview ();
            return true;
        }

        ++CurrentIndex;
    }
    return false;
}

bool AGridLevelEditorActor::RemoveAllLinksForSelectedObject ()
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const int32 RemovedCount = LevelAsset->Links.RemoveAll (
        [this] (const FGridObjectLink& Link)
    {
        return Link.SourceObjectId == LastSelectedObjectId ||
            Link.TargetObjectId == LastSelectedObjectId;
    });

    if (RemovedCount <= 0)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::CreateLink (
    FGuid SourceObjectId,
    FGuid TargetObjectId,
    EGridObjectEvent SourceEvent,
    EGridObjectCommand Command)
{
    if (!HasValidLevelAsset () || !SourceObjectId.IsValid () || !TargetObjectId.IsValid ())
    {
        return false;
    }

    if (!FindObjectById (SourceObjectId) || !FindObjectById (TargetObjectId))
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const bool bAlreadyExists = LevelAsset->Links.ContainsByPredicate (
        [&] (const FGridObjectLink& Link)
    {
        return Link.SourceObjectId == SourceObjectId &&
            Link.TargetObjectId == TargetObjectId &&
            Link.SourceEvent == SourceEvent &&
            Link.Command == Command;
    });

    if (bAlreadyExists)
    {
        return false;
    }

    FGridObjectLink NewLink;
    NewLink.SourceObjectId = SourceObjectId;
    NewLink.TargetObjectId = TargetObjectId;
    NewLink.SourceEvent = SourceEvent;
    NewLink.Command = Command;
    LevelAsset->Links.Add (NewLink);

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    LastSelectedObjectId = SourceObjectId;
    RebuildPreview ();
    return true;
}

void AGridLevelEditorActor::ClearSelectedObjectState ()
{
    LastSelectedObjectId.Invalidate ();

    PaintObjectType = EGridLevelObjectType::None;
    ObjectArchetypeId = NAME_None;
    SelectedArchetypeId = NAME_None;
    SelectedPaletteEntryId = NAME_None;

    bObjectInitiallyEnabled = true;
    bObjectInitiallyActive = false;

    ObjectTag = NAME_None;
    ObjectNotes.Empty ();
    ObjectBehavior = FGridObjectBehaviorParams ();
    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->SetEditorSelectedObject (FGuid ());
    }
}

bool AGridLevelEditorActor::RemoveExactLink (
    FGuid SourceObjectId,
    FGuid TargetObjectId,
    EGridObjectEvent SourceEvent,
    EGridObjectCommand Command)
{
    if (!HasValidLevelAsset () || !SourceObjectId.IsValid () || !TargetObjectId.IsValid ())
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const int32 RemovedCount = LevelAsset->Links.RemoveAll (
        [&] (const FGridObjectLink& Link)
    {
        return Link.SourceObjectId == SourceObjectId &&
            Link.TargetObjectId == TargetObjectId &&
            Link.SourceEvent == SourceEvent &&
            Link.Command == Command;
    });

    if (RemovedCount <= 0)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

const FGridLevelObjectData* AGridLevelEditorActor::GetSelectedObjectData () const
{
    return FindObjectById (LastSelectedObjectId);
}

bool AGridLevelEditorActor::SelectObjectById (FGuid ObjectId)
{
    if (!HasValidLevelAsset () || !ObjectId.IsValid ())
    {
        ClearSelectedObjectState ();
        return false;
    }

    const FGridLevelObjectData* Obj = FindObjectById (ObjectId);
    if (!Obj)
    {
        ClearSelectedObjectState ();
        return false;
    }

    LastSelectedObjectId = Obj->ObjectId;

    SelectedCellX = Obj->CellX;
    SelectedCellY = Obj->CellY;
    SelectedEdge = Obj->Edge;

    PaintObjectType = Obj->Type;
    ObjectArchetypeId = Obj->ArchetypeId;
    SelectedArchetypeId = Obj->ArchetypeId;
    SelectedPaletteEntryId = Obj->PaletteEntryId;

    bObjectInitiallyEnabled = Obj->bInitiallyEnabled;
    bObjectInitiallyActive = Obj->bInitiallyActive;

    ObjectTag = Obj->Tag;
    ObjectNotes = Obj->Notes;
    ObjectBehavior = Obj->Behavior;

    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->SetEditorSelectedObject (LastSelectedObjectId);
    }
    return true;
}

bool AGridLevelEditorActor::TryGetObjectWorldLocationById (
    FGuid ObjectId,
    FVector& OutWorldLocation) const
{
    const FGridLevelObjectData* Obj = FindObjectById (ObjectId);
    return Obj ? TryGetObjectWorldLocation (*Obj, OutWorldLocation) : false;
}

bool AGridLevelEditorActor::GetObjectEditorWorldCenter (
    const FGridLevelObjectData& Obj,
    FVector& OutWorldCenter) const
{
    if (!HasValidLevelAsset ())
    {
        return false;
    }

    const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
    if (CellSize <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    constexpr float FallbackCellHeight = 300.f;
    constexpr float DoorCenterHeight = FallbackCellHeight * 0.5f;
    constexpr float CeilingObjectInset = 32.f;

    auto ApplyDoorCenterHeight = [DoorCenterHeight] (const FGridLevelObjectData& ObjectData, FVector& InOutLocation)
    {
        if (ObjectData.Type == EGridLevelObjectType::Door)
        {
            InOutLocation.Z += DoorCenterHeight;
        }
    };

    if (PreviewRuntimeActor)
    {
        FTransform PlacementTransform = FTransform::Identity;
        if (PreviewRuntimeActor->GetObjectPlacementTransform (Obj, PlacementTransform))
        {
            OutWorldCenter = PlacementTransform.GetLocation ();
            ApplyDoorCenterHeight (Obj, OutWorldCenter);
            return true;
        }
    }

    FVector GridWorldOrigin = GetActorLocation ();
    const FVector CellBase = GridWorldOrigin + FVector (Obj.CellX * CellSize, Obj.CellY * CellSize, 0.f);

    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (Obj.ArchetypeId);
    const EGridObjectPlacementKind PlacementKind = Archetype
        ? Archetype->PlacementKind
        : (IsEdgePlacedObject (Obj) ? EGridObjectPlacementKind::Edge : EGridObjectPlacementKind::Center);

    if (Obj.Type == EGridLevelObjectType::Door)
    {
        switch (Obj.Edge)
        {
            case EGridEdge::North:
                OutWorldCenter = CellBase + FVector (CellSize * 0.5f, CellSize, DoorCenterHeight);
                return true;

            case EGridEdge::East:
                OutWorldCenter = CellBase + FVector (CellSize, CellSize * 0.5f, DoorCenterHeight);
                return true;

            case EGridEdge::South:
                OutWorldCenter = CellBase + FVector (CellSize * 0.5f, 0.f, DoorCenterHeight);
                return true;

            case EGridEdge::West:
                OutWorldCenter = CellBase + FVector (0.f, CellSize * 0.5f, DoorCenterHeight);
                return true;

            default:
                return false;
        }
    }
    if (Obj.Type == EGridLevelObjectType::Item && Obj.Edge != EGridEdge::None)
    {
        const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : 12.f;
        const float EdgeInset = Archetype ? FMath::Max (Archetype->WallInset, 18.f) : 18.f;

        switch (Obj.Edge)
        {
            case EGridEdge::North:
                OutWorldCenter = CellBase + FVector (CellSize * 0.5f, CellSize - EdgeInset, PlacementZOffset);
                return true;

            case EGridEdge::South:
                OutWorldCenter = CellBase + FVector (CellSize * 0.5f, EdgeInset, PlacementZOffset);
                return true;

            case EGridEdge::East:
                OutWorldCenter = CellBase + FVector (CellSize - EdgeInset, CellSize * 0.5f, PlacementZOffset);
                return true;

            case EGridEdge::West:
                OutWorldCenter = CellBase + FVector (EdgeInset, CellSize * 0.5f, PlacementZOffset);
                return true;

            default:
                return false;
        }
    }

    switch (PlacementKind)
    {
        case EGridObjectPlacementKind::Wall:
        case EGridObjectPlacementKind::Edge:
            if (Obj.Edge == EGridEdge::None)
            {
                return false;
            }
        {
            const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : 12.f;
            const float WallInset = Archetype ? Archetype->WallInset : 6.f;
            const float LocalOffsetAlongWall = Archetype ? Archetype->LocalOffsetAlongWall : 0.f;
            const float LocalOffsetVertical = Archetype ? Archetype->LocalOffsetVertical : 0.f;
            const float FinalZ = PlacementZOffset + LocalOffsetVertical;

            switch (Obj.Edge)
            {
                case EGridEdge::North:
                    OutWorldCenter = CellBase + FVector ((CellSize * 0.5f) + LocalOffsetAlongWall, CellSize - WallInset, FinalZ);
                    return true;

                case EGridEdge::South:
                    OutWorldCenter = CellBase + FVector ((CellSize * 0.5f) - LocalOffsetAlongWall, WallInset, FinalZ);
                    return true;

                case EGridEdge::East:
                    OutWorldCenter = CellBase + FVector (CellSize - WallInset, (CellSize * 0.5f) - LocalOffsetAlongWall, FinalZ);
                    return true;

                case EGridEdge::West:
                    OutWorldCenter = CellBase + FVector (WallInset, (CellSize * 0.5f) + LocalOffsetAlongWall, FinalZ);
                    return true;

                default:
                    return false;
            }
        }

        case EGridObjectPlacementKind::Ceiling:
        {
            const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : FallbackCellHeight - CeilingObjectInset;
            OutWorldCenter = CellBase + FVector (CellSize * 0.5f, CellSize * 0.5f, PlacementZOffset);
            return true;
        }

        case EGridObjectPlacementKind::Center:
        case EGridObjectPlacementKind::Floor:
        default:
        {
            const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : 12.f;
            OutWorldCenter = CellBase + FVector (CellSize * 0.5f, CellSize * 0.5f, PlacementZOffset);
            return true;
        }
    }
}

bool AGridLevelEditorActor::FocusSelectedObject ()
{
    if (!LastSelectedObjectId.IsValid ())
    {
        return false;
    }

    const FGridLevelObjectData* Obj = FindObjectById (LastSelectedObjectId);
    if (!Obj)
    {
        return false;
    }

    SelectedCellX = Obj->CellX;
    SelectedCellY = Obj->CellY;
    SelectedEdge = Obj->Edge;

#if WITH_EDITOR
    if (GEditor)
    {
        FVector WorldLocation = FVector::ZeroVector;
        if (TryGetObjectWorldLocation (*Obj, WorldLocation))
        {
            GEditor->MoveViewportCamerasToActor (*this, false);
        }
    }
#endif

    return true;
}

bool AGridLevelEditorActor::ApplyBehaviorToSelectedObject (
    const FGridObjectBehaviorParams& NewBehavior)
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    for (FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.ObjectId != LastSelectedObjectId)
        {
            continue;
        }

        Obj.Behavior = NewBehavior;
        ObjectBehavior = NewBehavior;

#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif

        RebuildPreview ();
        return true;
    }

    return false;
}

bool AGridLevelEditorActor::ResetSelectedObjectBehaviorFromArchetype ()
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (Obj->ArchetypeId);
    if (!Archetype)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->Behavior = Archetype->DefaultBehavior;
    ObjectBehavior = Obj->Behavior;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectArchetypeId (FName NewArchetypeId)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->ArchetypeId = NewArchetypeId;
    ObjectArchetypeId = NewArchetypeId;
    SelectedArchetypeId = NewArchetypeId;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionAsset (UGridItemDefinitionAsset* NewItemDefinitionAsset)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj || Obj->Type != EGridLevelObjectType::Item)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->ItemDefinitionAsset = NewItemDefinitionAsset;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionId (FName NewItemDefinitionId)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj || Obj->Type != EGridLevelObjectType::Item)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->ItemDefinitionId = NewItemDefinitionId;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SyncSelectedItemDefinitionIdFromAsset ()
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj || Obj->Type != EGridLevelObjectType::Item || !Obj->ItemDefinitionAsset)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->ItemDefinitionId = Obj->ItemDefinitionAsset->ItemDefinitionId;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedReceptacleInitialContainedItemDefinition (UGridItemDefinitionAsset* NewItemDefinitionAsset)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj || Obj->Type != EGridLevelObjectType::Receptacle)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->Behavior.Receptacle.InitialContainedItemDefinition = NewItemDefinitionAsset;
    ObjectBehavior = Obj->Behavior;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedReceptacleInitialContainedItemDefinitionId (FName NewItemDefinitionId)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj || Obj->Type != EGridLevelObjectType::Receptacle)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->Behavior.Receptacle.InitialContainedItemDefinitionId = NewItemDefinitionId;
    ObjectBehavior = Obj->Behavior;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SyncSelectedReceptacleInitialItemDefinitionIdFromAsset ()
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj || Obj->Type != EGridLevelObjectType::Receptacle || !Obj->Behavior.Receptacle.InitialContainedItemDefinition)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->Behavior.Receptacle.InitialContainedItemDefinitionId =
        Obj->Behavior.Receptacle.InitialContainedItemDefinition->ItemDefinitionId;
    ObjectBehavior = Obj->Behavior;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectTag (FName NewTag)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->Tag = NewTag;
    ObjectTag = NewTag;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectNotes (const FString& NewNotes)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->Notes = NewNotes;
    ObjectNotes = NewNotes;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableText (const FText& NewReadableText)
{
    FGridLevelObjectData* SelectedObject = FindSelectedObjectMutable ();
    if (!SelectedObject || !LevelAsset)
    {
        return false;
    }

#if WITH_EDITOR
    Modify ();
    LevelAsset->Modify ();
#endif

    SelectedObject->OverrideReadableText = NewReadableText;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyEnabled (bool bNewInitiallyEnabled)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->bInitiallyEnabled = bNewInitiallyEnabled;
    bObjectInitiallyEnabled = bNewInitiallyEnabled;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyActive (bool bNewInitiallyActive)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->bInitiallyActive = bNewInitiallyActive;
    bObjectInitiallyActive = bNewInitiallyActive;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::MoveSelectedObjectToCurrentSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected object, destination cell is invalid."));
        return false;
    }

    FGridLevelObjectData* SelectedObject = FindSelectedObjectMutable ();
    if (!SelectedObject)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected object, no object is selected."));
        return false;
    }

    const bool bRequiresEdge = IsEdgePlacedObject (*SelectedObject);
    if (bRequiresEdge && SelectedEdge == EGridEdge::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected edge-based object to Edge=None."));
        return false;
    }

    const EGridEdge DestinationEdge = bRequiresEdge ? SelectedEdge : EGridEdge::None;
    const bool bAlreadyAtDestination = SelectedObject->CellX == SelectedCellX &&
        SelectedObject->CellY == SelectedCellY &&
        SelectedObject->Edge == DestinationEdge;

    if (bAlreadyAtDestination)
    {
        UE_LOG (LogTemp, Log, TEXT ("GridLevelEditorActor: selected object is already at the current selection."));
        return true;
    }

    const FGuid SelectedObjectId = SelectedObject->ObjectId;
    const EGridLevelObjectType SelectedObjectType = SelectedObject->Type;
    const bool bDestinationOccupied = LevelAsset->Objects.ContainsByPredicate (
        [this, SelectedObjectId, SelectedObjectType, bRequiresEdge, DestinationEdge] (const FGridLevelObjectData& Obj)
    {
        if (Obj.ObjectId == SelectedObjectId ||
            Obj.CellX != SelectedCellX ||
            Obj.CellY != SelectedCellY ||
            Obj.Type != SelectedObjectType)
        {
            return false;
        }

        if (bRequiresEdge)
        {
            return Obj.Edge == DestinationEdge;
        }

        return true;
    });

    if (bDestinationOccupied)
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("GridLevelEditorActor: cannot move selected object, destination already contains an object of the same type."));
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    SelectedObject->CellX = SelectedCellX;
    SelectedObject->CellY = SelectedCellY;
    SelectedObject->Edge = DestinationEdge;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    UE_LOG (
        LogTemp,
        Log,
        TEXT ("GridLevelEditorActor: moved selected object %s to X=%d Y=%d Edge=%d."),
        *SelectedObjectId.ToString (),
        SelectedCellX,
        SelectedCellY,
        static_cast<int32> (DestinationEdge));
    return true;
}

TArray<FGridLevelValidationMessage> AGridLevelEditorActor::ValidateCurrentLevel ()
{
    LastValidationMessages.Reset ();

    auto AddMessage = [this] (
        EGridLevelValidationSeverity Severity,
        const FString& Message,
        const FGuid& OptionalObjectId = FGuid ())
    {
        FGridLevelValidationMessage ValidationMessage;
        ValidationMessage.Severity = Severity;
        ValidationMessage.Message = Message;
        ValidationMessage.OptionalObjectId = OptionalObjectId;
        LastValidationMessages.Add (ValidationMessage);
    };

    auto AddArchetypeValidationMessages = [this, &AddMessage] ()
    {
        if (!ObjectPalette)
        {
            return;
        }

        TSet<const UGridObjectArchetypeAsset*> ValidatedArchetypes;
        TSet<const UGridObjectArchetypeAsset*> DirectPaintItemArchetypes;

        TArray<FGridArchetypeValidationMessage> PaletteMessages;
        ObjectPalette->ValidatePalette (PaletteMessages);
        for (const FGridArchetypeValidationMessage& PaletteMessage : PaletteMessages)
        {
            AddMessage (
                ConvertArchetypeValidationSeverity (PaletteMessage.Severity),
                FString::Printf (TEXT ("ObjectPalette: %s"), *PaletteMessage.Message));
        }

        for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
        {
            const UGridObjectArchetypeAsset* Archetype = Entry.DefaultArchetype.Get ();
            if (!Archetype)
            {
                continue;
            }

            const FString ArchetypeName = Archetype->ArchetypeId.IsNone ()
                ? Archetype->GetName ()
                : Archetype->ArchetypeId.ToString ();

            if (Archetype->SupportedType == EGridLevelObjectType::Item && !DirectPaintItemArchetypes.Contains (Archetype))
            {
                DirectPaintItemArchetypes.Add (Archetype);
                AddMessage (
                    EGridLevelValidationSeverity::Info,
                    FString::Printf (
                        TEXT ("Archetype %s: Item archetype is directly available in the paint palette as a placed pickup item."),
                        *ArchetypeName));
            }

            if (ValidatedArchetypes.Contains (Archetype))
            {
                continue;
            }

            ValidatedArchetypes.Add (Archetype);

            TArray<FGridArchetypeValidationMessage> ArchetypeMessages;
            Archetype->ValidateArchetype (ArchetypeMessages);

            for (const FGridArchetypeValidationMessage& ArchetypeMessage : ArchetypeMessages)
            {
                AddMessage (
                    ConvertArchetypeValidationSeverity (ArchetypeMessage.Severity),
                    FString::Printf (
                        TEXT ("Archetype %s: %s"),
                        *ArchetypeName,
                        *ArchetypeMessage.Message));
            }
        }
    };

    auto AddExpectedConcreteArchetypeMessages = [this, &AddMessage] ()
    {
        if (!ObjectPalette)
        {
            return;
        }

        for (const FExpectedConcreteArchetypeSpec& ExpectedSpec : ExpectedConcreteArchetypes)
        {
            const FName ExpectedArchetypeId (ExpectedSpec.ArchetypeId);
            const FGridObjectPaletteEntry* MatchingEntry = nullptr;

            for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
            {
                if (Entry.GetEffectiveArchetypeId () == ExpectedArchetypeId)
                {
                    MatchingEntry = &Entry;
                    break;
                }
            }

            if (!MatchingEntry)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("ObjectPalette should expose concrete archetype '%s'. Variants must be palette entries/archetypes, not new EGridLevelObjectType values."),
                        ExpectedSpec.ArchetypeId));
                continue;
            }

            const EGridLevelObjectType EffectiveType = MatchingEntry->GetEffectiveObjectType ();
            if (EffectiveType != ExpectedSpec.ExpectedType)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("ObjectPalette archetype '%s' should use Type=%s, but currently uses Type=%s."),
                        ExpectedSpec.ArchetypeId,
                        *ToGridObjectTypeText (ExpectedSpec.ExpectedType),
                        *ToGridObjectTypeText (EffectiveType)));
            }
        }
    };

    if (!DungeonAsset)
    {
        AddMessage (
            EGridLevelValidationSeverity::Warning,
            TEXT ("DungeonAsset is missing. The editor can use LevelAsset directly, but dungeon level ids and transitions cannot be fully validated."));
    }
    else
    {
        TSet<FName> SeenLevelIds;
        TSet<FIntVector> SeenLogicalPositions;
        bool bHasEnabledFallbackLevel = false;

        if (DungeonAsset->Levels.Num () == 0)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("DungeonAsset contains no level entries."));
        }

        for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
        {
            if (Entry.LevelId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("DungeonAsset contains a level entry with an empty LevelId."));
            }
            else if (SeenLevelIds.Contains (Entry.LevelId))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (TEXT ("DungeonAsset contains duplicate LevelId '%s'."), *Entry.LevelId.ToString ()));
            }
            SeenLevelIds.Add (Entry.LevelId);

            if (SeenLogicalPositions.Contains (Entry.LogicalPosition))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("DungeonAsset contains duplicate LogicalPosition (%d,%d,%d)."),
                        Entry.LogicalPosition.X,
                        Entry.LogicalPosition.Y,
                        Entry.LogicalPosition.Z));
            }
            SeenLogicalPositions.Add (Entry.LogicalPosition);

            if (!Entry.LevelAsset)
            {
                AddMessage (
                    Entry.bEnabled ? EGridLevelValidationSeverity::Error : EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Dungeon level '%s' has no LevelAsset."),
                        Entry.LevelId.IsNone () ? TEXT ("None") : *Entry.LevelId.ToString ()));
            }
            else if (Entry.bEnabled && !Entry.LevelId.IsNone ())
            {
                bHasEnabledFallbackLevel = true;
            }
        }

        if (!DungeonAsset->IsValidLevelId (DungeonAsset->DefaultLevelId))
        {
            AddMessage (
                bHasEnabledFallbackLevel ? EGridLevelValidationSeverity::Warning : EGridLevelValidationSeverity::Error,
                bHasEnabledFallbackLevel
                    ? TEXT ("DefaultLevelId is invalid; runtime/editor fallback will use the first enabled level with a LevelAsset.")
                    : TEXT ("DefaultLevelId is invalid and no enabled fallback level with a LevelAsset exists."));
        }
    }

    AddArchetypeValidationMessages ();
    AddExpectedConcreteArchetypeMessages ();

    if (!LevelAsset)
    {
        AddMessage (
            EGridLevelValidationSeverity::Error,
            TEXT ("LevelAsset is missing."));
        return LastValidationMessages;
    }

    if (LevelAsset->Width <= 0)
    {
        AddMessage (EGridLevelValidationSeverity::Error, TEXT ("LevelAsset Width must be greater than zero."));
    }
    if (LevelAsset->Height <= 0)
    {
        AddMessage (EGridLevelValidationSeverity::Error, TEXT ("LevelAsset Height must be greater than zero."));
    }
    if (LevelAsset->CellSize <= 0.f)
    {
        AddMessage (EGridLevelValidationSeverity::Error, TEXT ("LevelAsset CellSize must be greater than zero."));
    }

    const int32 ExpectedCellCount = FMath::Max (1, LevelAsset->Width) * FMath::Max (1, LevelAsset->Height);
    if (LevelAsset->Cells.Num () != ExpectedCellCount)
    {
        AddMessage (
            EGridLevelValidationSeverity::Error,
            FString::Printf (
                TEXT ("LevelAsset Cells.Num()=%d but expected %d for Width=%d Height=%d."),
                LevelAsset->Cells.Num (),
                ExpectedCellCount,
                LevelAsset->Width,
                LevelAsset->Height));
    }

    if (!LevelAsset->IsStartCellValid ())
    {
        AddMessage (
            EGridLevelValidationSeverity::Error,
            FString::Printf (
                TEXT ("Start cell X=%d Y=%d Facing=%s is invalid. It must be inside the grid, non-empty and not block occupancy."),
                LevelAsset->StartCellX,
                LevelAsset->StartCellY,
                *GetGridEdgeText (LevelAsset->StartFacing)));
    }

    if (LevelAsset->Width > 0 && LevelAsset->Height > 0 && LevelAsset->Cells.Num () == ExpectedCellCount)
    {
        int32 OverlappingSharedWallCount = 0;
        int32 DirectionalSharedWallCount = 0;
        FString FirstOverlappingSharedWall;
        FString FirstDirectionalSharedWall;

        for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
        {
            for (int32 X = 0; X < LevelAsset->Width; ++X)
            {
                const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);

                auto ValidateSharedEdge = [
                    &OverlappingSharedWallCount,
                    &DirectionalSharedWallCount,
                    &FirstOverlappingSharedWall,
                    &FirstDirectionalSharedWall,
                    X,
                    Y] (
                    const TCHAR* EdgeName,
                    EGridWallType LocalWall,
                    EGridWallType OppositeWall,
                    int32 NeighborX,
                    int32 NeighborY)
                {
                    if (LocalWall != EGridWallType::None && OppositeWall != EGridWallType::None)
                    {
                        ++OverlappingSharedWallCount;
                        if (FirstOverlappingSharedWall.IsEmpty ())
                        {
                            FirstOverlappingSharedWall = FString::Printf (
                                TEXT ("%s between (%d,%d) and (%d,%d)"),
                                EdgeName, X, Y, NeighborX, NeighborY);
                        }
                    }
                    else if (LocalWall != OppositeWall)
                    {
                        ++DirectionalSharedWallCount;
                        if (FirstDirectionalSharedWall.IsEmpty ())
                        {
                            FirstDirectionalSharedWall = FString::Printf (
                                TEXT ("%s between (%d,%d) and (%d,%d), %s vs %s"),
                                EdgeName,
                                X,
                                Y,
                                NeighborX,
                                NeighborY,
                                *UEnum::GetValueAsString (LocalWall),
                                *UEnum::GetValueAsString (OppositeWall));
                        }
                    }
                };

                if (X + 1 < LevelAsset->Width)
                {
                    ValidateSharedEdge (
                        TEXT ("East/West"),
                        Cell.EastWall,
                        LevelAsset->GetCell (X + 1, Y).WestWall,
                        X + 1,
                        Y);
                }
                if (Y + 1 < LevelAsset->Height)
                {
                    ValidateSharedEdge (
                        TEXT ("North/South"),
                        Cell.NorthWall,
                        LevelAsset->GetCell (X, Y + 1).SouthWall,
                        X,
                        Y + 1);
                }
            }
        }

        if (OverlappingSharedWallCount > 0)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                FString::Printf (
                    TEXT ("%d shared edges have walls on both sides; runtime rendering may create overlapping wall instances. First: %s."),
                    OverlappingSharedWallCount,
                    *FirstOverlappingSharedWall));
        }
        if (DirectionalSharedWallCount > 0)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                FString::Printf (
                    TEXT ("%d shared edges are directional; movement depends on the source cell. First: %s."),
                    DirectionalSharedWallCount,
                    *FirstDirectionalSharedWall));
        }
    }

    TSet<FGuid> SeenObjectIds;
    TMap<FGuid, const FGridLevelObjectData*> ObjectsById;
    TMap<FGuid, int32> OutgoingLinkCountBySourceId;
    TMap<FGuid, int32> ReceptacleItemInsertedLinkCountBySourceId;
    TMap<FGuid, int32> ReceptacleItemRemovedLinkCountBySourceId;
    TMap<FGuid, int32> ReceptacleItemChangedLinkCountBySourceId;

    auto IsEdgeOrWallPlacedObject = [this] (const FGridLevelObjectData& ObjectData) -> bool
    {
        return IsEdgePlacedObject (ObjectData);
    };

    auto GetValidationAnchorKey = [&IsEdgeOrWallPlacedObject] (const FGridLevelObjectData& ObjectData) -> FString
    {
        if (!IsEdgeOrWallPlacedObject (ObjectData))
        {
            return TEXT ("Center");
        }

        switch (ObjectData.Edge)
        {
            case EGridEdge::North:
                return TEXT ("North");

            case EGridEdge::East:
                return TEXT ("East");

            case EGridEdge::South:
                return TEXT ("South");

            case EGridEdge::West:
                return TEXT ("West");

            case EGridEdge::None:
            default:
                return TEXT ("Center");
        }
    };

    auto GetObjectValidationName = [] (const FGridLevelObjectData& ObjectData) -> FString
    {
        if (!ObjectData.Tag.IsNone ())
        {
            return ObjectData.Tag.ToString ();
        }

        if (!ObjectData.ArchetypeId.IsNone ())
        {
            return ObjectData.ArchetypeId.ToString ();
        }

        return ObjectData.ObjectId.IsValid ()
            ? ObjectData.ObjectId.ToString ().Left (8)
            : FString (TEXT ("InvalidObjectId"));
    };

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (!Obj.ObjectId.IsValid ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Object at X=%d Y=%d has an invalid ObjectId."),
                    Obj.CellX,
                    Obj.CellY));
        } else if (SeenObjectIds.Contains (Obj.ObjectId))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("Duplicate ObjectId found."),
                Obj.ObjectId);
        } else
        {
            SeenObjectIds.Add (Obj.ObjectId);
            ObjectsById.Add (Obj.ObjectId, &Obj);
        }

        if (!LevelAsset->IsValidCoord (Obj.CellX, Obj.CellY))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Object is outside grid bounds at X=%d Y=%d."),
                    Obj.CellX,
                    Obj.CellY),
                Obj.ObjectId);
            continue;
        }

        const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (Obj.ArchetypeId);
        if (Obj.ArchetypeId.IsNone ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("Placed object has no ArchetypeId. Preview and runtime archetype lookup cannot resolve it."),
                Obj.ObjectId);
        }
        else if (ObjectPalette && !Archetype)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Placed object ArchetypeId '%s' is not exposed by the assigned ObjectPalette."),
                    *Obj.ArchetypeId.ToString ()),
                Obj.ObjectId);
        }

        if (Archetype && Obj.Type != Archetype->SupportedType)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Placed object Type=%s does not match archetype '%s' SupportedType=%s."),
                    *ToGridObjectTypeText (Obj.Type),
                    *Obj.ArchetypeId.ToString (),
                    *ToGridObjectTypeText (Archetype->SupportedType)),
                Obj.ObjectId);
        }

        if (Archetype &&
            Obj.Type != EGridLevelObjectType::Item &&
            Archetype->IsCenterPlaced () &&
            Obj.Edge != EGridEdge::None)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                FString::Printf (
                    TEXT ("Center-placed object has a cardinal Edge=%s; runtime center placement ignores this edge."),
                    *GetGridEdgeText (Obj.Edge)),
                Obj.ObjectId);
        }

        if (ObjectPalette && !Obj.PaletteEntryId.IsNone ())
        {
            const FGridObjectPaletteEntry* PaletteEntry = ObjectPalette->FindEntryById (Obj.PaletteEntryId);
            if (!PaletteEntry)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Placed object PaletteEntryId '%s' no longer exists in the assigned ObjectPalette."),
                        *Obj.PaletteEntryId.ToString ()),
                    Obj.ObjectId);
            }
            else if (PaletteEntry->GetEffectiveArchetypeId () != Obj.ArchetypeId)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Placed object PaletteEntryId '%s' now resolves to archetype '%s', but the object stores ArchetypeId '%s'."),
                        *Obj.PaletteEntryId.ToString (),
                        *PaletteEntry->GetEffectiveArchetypeId ().ToString (),
                        *Obj.ArchetypeId.ToString ()),
                    Obj.ObjectId);
            }
        }

        if (IsEdgeOrWallPlacedObject (Obj) && Obj.Edge == EGridEdge::None)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                TEXT ("Edge or wall placed object has Edge=None."),
                Obj.ObjectId);
        }

        if (Archetype && Archetype->bBlocksMovement)
        {
            const FGridLevelCellData& CellData = LevelAsset->GetCell (Obj.CellX, Obj.CellY);
            if (CellData.bBlocksOccupancy)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Object blocks movement on a cell that already blocks occupancy."),
                    Obj.ObjectId);
            }
        }

        if (Obj.Type == EGridLevelObjectType::Door)
        {
            const FGridLevelCellData& CellData = LevelAsset->GetCell (Obj.CellX, Obj.CellY);
            const EGridWallType WallType = GetWallTypeForEdge (CellData, Obj.Edge);
            if (WallType == EGridWallType::Solid)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Door is placed on an edge whose wall is Solid. A door edge must use WallType=None."),
                Obj.ObjectId);
            }

            int32 NeighborX = Obj.CellX;
            int32 NeighborY = Obj.CellY;
            switch (Obj.Edge)
            {
                case EGridEdge::North: ++NeighborY; break;
                case EGridEdge::East:  ++NeighborX; break;
                case EGridEdge::South: --NeighborY; break;
                case EGridEdge::West:  --NeighborX; break;
                case EGridEdge::None:
                default:
                    break;
            }
            if (Obj.Edge != EGridEdge::None && !LevelAsset->IsValidCoord (NeighborX, NeighborY))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Door is placed on an outer grid edge with no neighboring cell to cross."),
                    Obj.ObjectId);
            }
        }

        if (Obj.Behavior.Transition.bIsTransition)
        {
            const FGridObjectTransitionParams& Transition = Obj.Behavior.Transition;
            if (Transition.TargetLevelId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Transition has no TargetLevelId."),
                    Obj.ObjectId);
            }

            if (Transition.TargetFacing == EGridEdge::None)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Transition TargetFacing cannot be None."),
                    Obj.ObjectId);
            }

            const UGridLevelAsset* TargetLevelAsset = nullptr;
            if (DungeonAsset)
            {
                TargetLevelAsset = DungeonAsset->GetLevelAssetById (Transition.TargetLevelId);
                if (!Transition.TargetLevelId.IsNone () && !TargetLevelAsset)
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("Transition target LevelId '%s' was not found as an enabled level with a LevelAsset in the DungeonAsset."),
                            *Transition.TargetLevelId.ToString ()),
                        Obj.ObjectId);
                }
            }
            else
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Transition cannot validate TargetLevelId because DungeonAsset is null."),
                    Obj.ObjectId);
            }

            if (TargetLevelAsset)
            {
                if (!TargetLevelAsset->IsValidCoord (Transition.TargetCellX, Transition.TargetCellY))
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("Transition target cell X=%d Y=%d is outside target level bounds."),
                            Transition.TargetCellX,
                            Transition.TargetCellY),
                        Obj.ObjectId);
                }
            }
            else if (!LevelAsset->IsValidCoord (Transition.TargetCellX, Transition.TargetCellY))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Transition target cell X=%d Y=%d is outside the current level bounds; target level bounds could not be validated."),
                        Transition.TargetCellX,
                        Transition.TargetCellY),
                    Obj.ObjectId);
            }
        }

        if (Obj.Type == EGridLevelObjectType::Receptacle)
        {
            const FGridObjectBehaviorParams& Behavior = Obj.Behavior;
            const FGridReceptacleBehaviorParams& Receptacle = Behavior.Receptacle;
            const FName InitialDefinitionId =
                Receptacle.InitialContainedItemDefinition &&
                !Receptacle.InitialContainedItemDefinition->ItemDefinitionId.IsNone ()
                    ? Receptacle.InitialContainedItemDefinition->ItemDefinitionId
                    : Receptacle.InitialContainedItemDefinitionId;

            for (const FName AcceptedId : Receptacle.AcceptedArchetypeIds)
            {
                if (!AcceptedId.IsNone () && Receptacle.RejectedItemArchetypeIds.Contains (AcceptedId))
                {
                    AddMessage (
                        EGridLevelValidationSeverity::Error,
                        FString::Printf (
                            TEXT ("Receptacle item id '%s' is present in both accepted and rejected lists."),
                            *AcceptedId.ToString ()),
                        Obj.ObjectId);
                }
            }

            if (Obj.bInitiallyActive &&
                InitialDefinitionId.IsNone () &&
                Receptacle.InitialContainedItemArchetypeId.IsNone ())
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Receptacle is initially active but has no initial item definition or legacy item id."),
                    Obj.ObjectId);
            }

            if (!InitialDefinitionId.IsNone () &&
                Receptacle.RejectedItemArchetypeIds.Contains (InitialDefinitionId))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Receptacle starts with item definition '%s' but the rejected list includes it."),
                        *InitialDefinitionId.ToString ()),
                    Obj.ObjectId);
            }

            if (!Behavior.Receptacle.InitialContainedItemArchetypeId.IsNone ()
                && !Behavior.Receptacle.bAcceptAnyItem
                && !Behavior.Receptacle.AcceptedArchetypeIds.Contains (Behavior.Receptacle.InitialContainedItemArchetypeId))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Receptacle starts with '%s' but AcceptedArchetypeIds does not include it."),
                        *Behavior.Receptacle.InitialContainedItemArchetypeId.ToString ()),
                    Obj.ObjectId);
            }

            if (!Behavior.Receptacle.InitialContainedItemArchetypeId.IsNone ()
                && Behavior.Receptacle.RejectedItemArchetypeIds.Contains (Behavior.Receptacle.InitialContainedItemArchetypeId))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Receptacle starts with '%s' but RejectedItemArchetypeIds includes it."),
                        *Behavior.Receptacle.InitialContainedItemArchetypeId.ToString ()),
                    Obj.ObjectId);
            }

            if (!Behavior.Receptacle.bAcceptAnyItem
                && Behavior.Receptacle.AcceptedItemTags.Num () == 0
                && Behavior.Receptacle.AcceptedArchetypeIds.Num () == 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Receptacle accepts no item: bAcceptAnyItem=false and accepted lists are empty."),
                Obj.ObjectId);
            }
        }
    }

    for (int32 ObjectIndex = 0; ObjectIndex < LevelAsset->Objects.Num (); ++ObjectIndex)
    {
        const FGridLevelObjectData& ObjectA = LevelAsset->Objects[ObjectIndex];
        const UGridObjectArchetypeAsset* ArchetypeA = FindObjectArchetypeById (ObjectA.ArchetypeId);
        if (!ArchetypeA || !LevelAsset->IsValidCoord (ObjectA.CellX, ObjectA.CellY))
        {
            continue;
        }

        const FString AnchorA = GetValidationAnchorKey (ObjectA);
        for (int32 OtherIndex = 0; OtherIndex < LevelAsset->Objects.Num (); ++OtherIndex)
        {
            if (ObjectIndex == OtherIndex)
            {
                continue;
            }

            const FGridLevelObjectData& ObjectB = LevelAsset->Objects[OtherIndex];
            if (ObjectA.CellX != ObjectB.CellX || ObjectA.CellY != ObjectB.CellY)
            {
                continue;
            }

            if (!ArchetypeA->bCanShareCell)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Object does not allow sharing its cell but another object is placed there."),
                    ObjectA.ObjectId);
                break;
            }

            if (!ArchetypeA->bCanShareAnchor && AnchorA == GetValidationAnchorKey (ObjectB))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (TEXT ("Object does not allow sharing anchor '%s' but another object uses it."), *AnchorA),
                    ObjectA.ObjectId);
                break;
            }
        }
    }

    TSet<FString> SeenLinkKeys;
    TMap<FString, uint8> DoorCommandDirectionsBySourceEvent;
    for (int32 LinkIndex = 0; LinkIndex < LevelAsset->Links.Num (); ++LinkIndex)
    {
        const FGridObjectLink& Link = LevelAsset->Links[LinkIndex];
        const FGridLevelObjectData* const* SourceObjectPtr = ObjectsById.Find (Link.SourceObjectId);
        const FGridLevelObjectData* const* TargetObjectPtr = ObjectsById.Find (Link.TargetObjectId);
        const FGridLevelObjectData* SourceObject = SourceObjectPtr ? *SourceObjectPtr : nullptr;
        const FGridLevelObjectData* TargetObject = TargetObjectPtr ? *TargetObjectPtr : nullptr;

        const FString LinkKey = FString::Printf (
            TEXT ("%s|%s|%d|%d|%d|%s|%s|%d|%d|%.9g|%d"),
            *Link.SourceObjectId.ToString (EGuidFormats::Digits),
            *Link.TargetObjectId.ToString (EGuidFormats::Digits),
            static_cast<int32> (Link.SourceEvent),
            static_cast<int32> (Link.Command),
            static_cast<int32> (Link.Condition),
            *Link.ConditionItemDefinitionId.ToString (),
            *Link.ConditionItemTag.ToString (),
            static_cast<int32> (Link.ConditionItemType),
            Link.ConditionCount,
            Link.ConditionWeight,
            Link.bInvertCondition ? 1 : 0);
        if (SeenLinkKeys.Contains (LinkKey))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d duplicates an identical link."), LinkIndex),
                Link.SourceObjectId);
        }
        else
        {
            SeenLinkKeys.Add (LinkKey);
        }

        if (!Link.SourceObjectId.IsValid ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d has an invalid SourceObjectId."), LinkIndex));
        }
        else if (!SourceObject)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d SourceObjectId was not found."), LinkIndex),
                Link.SourceObjectId);
        }
        else
        {
            int32& OutgoingCount = OutgoingLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
            ++OutgoingCount;

            if (!IsEventEmittedByCurrentRuntime (SourceObject->Type, Link.SourceEvent))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Link %d uses SourceEvent=%s, which is not emitted by the current C++ runtime for source type %s."),
                        LinkIndex,
                        *ToGridObjectEventText (Link.SourceEvent),
                        *ToGridObjectTypeText (SourceObject->Type)),
                    Link.SourceObjectId);
            }
            if (!SourceObject->bInitiallyEnabled)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (TEXT ("Link %d source object is initially disabled."), LinkIndex),
                    Link.SourceObjectId);
            }

            switch (Link.SourceEvent)
            {
                case EGridObjectEvent::ItemInserted:
                {
                    int32& EventCount = ReceptacleItemInsertedLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
                    ++EventCount;
                    break;
                }

                case EGridObjectEvent::ItemRemoved:
                {
                    int32& EventCount = ReceptacleItemRemovedLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
                    ++EventCount;
                    break;
                }

                case EGridObjectEvent::ItemChanged:
                {
                    int32& EventCount = ReceptacleItemChangedLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
                    ++EventCount;
                    break;
                }

                default:
                    break;
            }
        }

        if (!Link.TargetObjectId.IsValid ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d has an invalid TargetObjectId."), LinkIndex));
        }
        else if (!TargetObject)
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (TEXT ("Link %d TargetObjectId was not found."), LinkIndex),
                Link.TargetObjectId);
        }
        else
        {
            if (!IsCommandSupportedByCurrentRuntime (TargetObject->Type, Link.Command))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Link %d command %s is not supported by the current runtime for target type %s."),
                        LinkIndex,
                        *ToGridObjectCommandText (Link.Command),
                        *ToGridObjectTypeText (TargetObject->Type)),
                    Link.TargetObjectId);
            }
            if (!TargetObject->bInitiallyEnabled)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (TEXT ("Link %d target object is initially disabled and may have no spawned runtime actor."), LinkIndex),
                    Link.TargetObjectId);
            }

            if (TargetObject->Type == EGridLevelObjectType::Door)
            {
                const bool bOpensDoor =
                    Link.Command == EGridObjectCommand::Open ||
                    Link.Command == EGridObjectCommand::Activate;
                const bool bClosesDoor =
                    Link.Command == EGridObjectCommand::Close ||
                    Link.Command == EGridObjectCommand::Deactivate;
                if (bOpensDoor || bClosesDoor)
                {
                    const FString DoorCommandKey = FString::Printf (
                        TEXT ("%s|%s|%d"),
                        *Link.SourceObjectId.ToString (EGuidFormats::Digits),
                        *Link.TargetObjectId.ToString (EGuidFormats::Digits),
                        static_cast<int32> (Link.SourceEvent));
                    uint8& DirectionMask = DoorCommandDirectionsBySourceEvent.FindOrAdd (DoorCommandKey);
                    DirectionMask |= bOpensDoor ? 1 : 2;
                    if (DirectionMask == 3)
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Warning,
                            FString::Printf (
                                TEXT ("Link %d conflicts with another link: the same source event both opens and closes this door."),
                                LinkIndex),
                            Link.TargetObjectId);
                    }
                }
            }
        }

        if (Link.SourceObjectId.IsValid () && Link.SourceObjectId == Link.TargetObjectId)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                FString::Printf (TEXT ("Link %d targets its own source object."), LinkIndex),
                Link.SourceObjectId);
        }

        if (Link.Condition != EGridObjectCondition::None)
        {
            if (!TargetObject || TargetObject->Type != EGridLevelObjectType::Receptacle)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    FString::Printf (
                        TEXT ("Link %d condition %s requires a receptacle target."),
                        LinkIndex,
                        *ToGridObjectConditionText (Link.Condition)),
                    Link.TargetObjectId);
            }

            switch (Link.Condition)
            {
                case EGridObjectCondition::ReceptacleContainsItemDefinition:
                    if (Link.ConditionItemDefinitionId.IsNone ())
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires ConditionItemDefinitionId."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleContainsItemTag:
                    if (Link.ConditionItemTag.IsNone ())
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires ConditionItemTag."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleContainsItemType:
                    if (Link.ConditionItemType == EGridItemType::None)
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires a non-None ConditionItemType."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleItemCountAtLeast:
                    if (Link.ConditionCount <= 0)
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires ConditionCount > 0."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleWeightAtLeast:
                    if (Link.ConditionWeight <= 0.0f)
                    {
                        AddMessage (
                            EGridLevelValidationSeverity::Error,
                            FString::Printf (TEXT ("Link %d condition requires ConditionWeight > 0."), LinkIndex),
                            Link.SourceObjectId);
                    }
                    break;

                case EGridObjectCondition::ReceptacleIsEmpty:
                case EGridObjectCondition::ReceptacleHasAnyItem:
                case EGridObjectCondition::None:
                default:
                    break;
            }
        }
    }

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.Type == EGridLevelObjectType::Trigger && !OutgoingLinkCountBySourceId.Contains (Obj.ObjectId))
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                TEXT ("Trigger has no outgoing links."),
                Obj.ObjectId);
        }

        if (Obj.Type == EGridLevelObjectType::Receptacle)
        {
            const int32 ItemInsertedCount = ReceptacleItemInsertedLinkCountBySourceId.FindRef (Obj.ObjectId);
            const int32 ItemRemovedCount = ReceptacleItemRemovedLinkCountBySourceId.FindRef (Obj.ObjectId);
            const int32 ItemChangedCount = ReceptacleItemChangedLinkCountBySourceId.FindRef (Obj.ObjectId);

            if (ItemInsertedCount == 0 && ItemRemovedCount == 0 && ItemChangedCount > 0)
            {
                continue;
            }

            if (ItemRemovedCount > 0 && ItemInsertedCount == 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Receptacle '%s' has ItemRemoved links but no ItemInserted links. This may be intentional, but the puzzle will not reset when an item is inserted again."),
                        *GetObjectValidationName (Obj)),
                    Obj.ObjectId);
            }

            if (ItemInsertedCount > 0 && ItemRemovedCount == 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Receptacle '%s' has ItemInserted links but no ItemRemoved links. This may be intentional, but the puzzle will not react when the item is removed."),
                        *GetObjectValidationName (Obj)),
                    Obj.ObjectId);
            }
        }
    }

    if (LastValidationMessages.Num () == 0)
    {
        AddMessage (
            EGridLevelValidationSeverity::Info,
            TEXT ("Validation complete: no issues found."));
    }

    return LastValidationMessages;
}

bool AGridLevelEditorActor::HasAnyObjectInSelectedCell () const
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return false;
    }

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.CellX == SelectedCellX && Obj.CellY == SelectedCellY)
        {
            return true;
        }
    }

    return false;
}

bool AGridLevelEditorActor::HasAnyWallInSelectedCell () const
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return false;
    }

    const FGridLevelCellData& CellData = LevelAsset->GetCell (SelectedCellX, SelectedCellY);

    return CellData.NorthWall != EGridWallType::None ||
        CellData.EastWall != EGridWallType::None ||
        CellData.SouthWall != EGridWallType::None ||
        CellData.WestWall != EGridWallType::None;
}

void AGridLevelEditorActor::EraseAtSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return;
    }

    const int32 RemovedObjectCount = RemoveObjectsAtSelectionInternal (false);

    if (RemovedObjectCount > 0)
    {
        LastSelectedObjectId.Invalidate ();
        RebuildPreview ();
        return;
    }

    if (FGridLevelCellData* CellData = GetSelectedCellMutable ())
    {
        if (EGridWallType* WallPtr = GetSelectedWallMutable (*CellData))
        {
            if (*WallPtr != EGridWallType::None)
            {
                ClearSelectedWall ();
                return;
            }
        }

        if (CellData->CellType != EGridCellType::Empty &&
            !HasAnyObjectInSelectedCell () &&
            !HasAnyWallInSelectedCell ())
        {
            ClearSelectedCell ();
            return;
        }
    }
}

bool AGridLevelEditorActor::UpdateHoveredObjectFromWorldPoint (const FVector& WorldPoint)
{
    ResolvePreviewRuntimeActor ();

    HoveredObjectId.Invalidate ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->SetEditorHoveredObject (FGuid ());
    }

    if (!HasValidLevelAsset ())
    {
        return false;
    }

    float BestDistSq = FMath::Square (ObjectHoverPickRadius);
    const FGridLevelObjectData* BestObject = nullptr;

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        FVector ObjLocation = FVector::ZeroVector;

        if (!TryGetObjectWorldLocation (Obj, ObjLocation))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared2D (WorldPoint, ObjLocation);

        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            BestObject = &Obj;
        }
    }

    if (!BestObject)
    {
        return false;
    }
    HoveredObjectId = BestObject->ObjectId;

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->SetEditorHoveredObject (HoveredObjectId);
    }

    return true;
}

bool AGridLevelEditorActor::SelectHoveredObject ()
{
    if (!HoveredObjectId.IsValid ())
    {
        return false;
    }

    return SelectObjectById (HoveredObjectId);
}

bool AGridLevelEditorActor::TryGetHoveredObjectWorldLocation (FVector& OutWorldLocation) const
{
    if (!HoveredObjectId.IsValid ())
    {
        return false;
    }

    return TryGetObjectWorldLocationById (HoveredObjectId, OutWorldLocation);
}

void AGridLevelEditorActor::PaintSelectedCell ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        return;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    CellData->CellType = PaintCellType;
    CellData->bHasCeiling = bPaintCellHasCeiling;
    CellData->bBlocksOccupancy = bPaintCellBlocksOccupancy;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildGeometryPreview ();
}

void AGridLevelEditorActor::RebuildGeometryPreview ()
{
    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->LevelAsset = LevelAsset;
        PreviewRuntimeActor->RebuildLevel (EGridRuntimeRebuildMode::GeometryOnly);
    }
}

void AGridLevelEditorActor::EnsureCoordinateHoverLabel ()
{
    if (CoordinateHoverLabel || !SceneRoot)
    {
        return;
    }
    CoordinateHoverLabel = NewObject<UTextRenderComponent> (this, UTextRenderComponent::StaticClass (),
        TEXT ("CoordinateHoverLabel"), RF_Transactional);
    if (!CoordinateHoverLabel)
    {
        return;
    }
    CoordinateHoverLabel->CreationMethod = EComponentCreationMethod::Instance;
    AddInstanceComponent (CoordinateHoverLabel);
    CoordinateHoverLabel->AttachToComponent (SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);

    CoordinateHoverLabel->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    CoordinateHoverLabel->SetHiddenInGame (false);
    CoordinateHoverLabel->SetHorizontalAlignment (EHTA_Center);
    CoordinateHoverLabel->SetVerticalAlignment (EVRTA_TextCenter);
    CoordinateHoverLabel->SetTextRenderColor (FColor::White);
    CoordinateHoverLabel->SetRelativeRotation (FRotator (90.f, -90.f, 0.f));

    CoordinateHoverLabel->RegisterComponentWithWorld (GetWorld ());
}

void AGridLevelEditorActor::UpdateCoordinateHoverLabel ()
{
    if (!bShowCoordinateGrid || !bShowCoordinateLabels || !LevelAsset || !LevelAsset->IsValidCoord (HoveredCellX, HoveredCellY))
    {
        if (CoordinateHoverLabel)
        {
            CoordinateHoverLabel->SetVisibility (false, true);
        }
        return;
    }
    EnsureCoordinateHoverLabel ();
    if (!CoordinateHoverLabel)
    {
        return;
    }
    const TCHAR* EdgeText = TEXT (" ");

    switch (HoveredEdge)
    {
        case EGridEdge::North: EdgeText = TEXT ("N"); break;
        case EGridEdge::East:  EdgeText = TEXT ("E"); break;
        case EGridEdge::South: EdgeText = TEXT ("S"); break;
        case EGridEdge::West:  EdgeText = TEXT ("W"); break;
        default: break;
    }
    const float CellSize = LevelAsset->CellSize;
    CoordinateHoverLabel->SetWorldSize (CoordinateLabelWorldSize);
    CoordinateHoverLabel->SetText (
        FText::FromString (
            FString::Printf (TEXT ("X:%d   Y:%d  %s"),
                HoveredCellX, HoveredCellY, EdgeText)));
    CoordinateHoverLabel->SetRelativeLocation (
        FVector ((HoveredCellX + 0.5f) * CellSize, (HoveredCellY + 0.5f) * CellSize, CoordinateGridZOffset + 4.f));
    CoordinateHoverLabel->SetVisibility (true, true);
    CoordinateHoverLabel->MarkRenderStateDirty ();
}

void AGridLevelEditorActor::UpdateCoordinateGridPlane ()
{
    if (!CoordinateGridPlane)
    {
        return;
    }
    const bool bVisible = bShowCoordinateGrid && LevelAsset != nullptr && CoordinateGridPlaneMesh != nullptr;
    CoordinateGridPlane->SetVisibility (bVisible, true);
    if (!bVisible)
    {
        return;
    }
    const float CellSize = LevelAsset->CellSize;
    const int32 Width = LevelAsset->Width;
    const int32 Height = LevelAsset->Height;
    CoordinateGridPlane->SetStaticMesh (CoordinateGridPlaneMesh);
    if (CoordinateGridMaterial)
    {
        CoordinateGridPlane->SetMaterial (0, CoordinateGridMaterial);
    }
    CoordinateGridPlane->SetRelativeLocation (FVector (Width * CellSize * 0.5f, Height * CellSize * 0.5f, CoordinateGridZOffset));
    CoordinateGridPlane->SetRelativeScale3D (FVector (Width * CellSize / 100.f, Height * CellSize / 100.f, 1.f));
}
