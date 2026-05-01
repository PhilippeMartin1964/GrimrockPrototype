#include "Runtime/GridEditorPreviewComponent.h"

#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "EngineUtils.h"

UGridEditorPreviewComponent::UGridEditorPreviewComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridEditorPreviewComponent::Initialize (AGridLevelRuntimeActor* InRuntimeActor)
{
    RuntimeActor = InRuntimeActor;
}

void UGridEditorPreviewComponent::ClearPreviewObjects ()
{
    UWorld* World = GetWorld ();
    if (World)
    {
        for (TActorIterator<AGridEditorPreviewObjectActor> It (World); It; ++It)
        {
            AGridEditorPreviewObjectActor* PreviewActor = *It;
            if (!IsValid (PreviewActor))
            {
                continue;
            }
            if (PreviewActor->GetOwner () == RuntimeActor || SpawnedPreviewObjects.Contains (PreviewActor))
            {
                PreviewActor->Destroy ();
            }
        }
    } else
    {
        for (AGridEditorPreviewObjectActor* Actor : SpawnedPreviewObjects)
        {
            if (IsValid (Actor))
            {
                Actor->Destroy ();
            }
        }
    }
    SpawnedPreviewObjects.Empty ();
    CurrentSelectedObjectId.Invalidate ();
    CurrentHoveredObjectId.Invalidate ();
}

void UGridEditorPreviewComponent::RebuildPreviewObjects ()
{
    ClearPreviewObjects ();
    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return;
    }
    for (const FGridLevelObjectData& ObjectData : RuntimeActor->LevelAsset->Objects)
    {
        if (!IsPreviewableObject (ObjectData))
        {
            continue;
        }
        AddPreviewObject (ObjectData);
    }
}

void UGridEditorPreviewComponent::AddPreviewObject (const FGridLevelObjectData& ObjectData)
{
    TSubclassOf<AGridEditorPreviewObjectActor> PreviewClass;

    if (RuntimeActor->EditorPreviewObjectActorClass)
    {
        PreviewClass = RuntimeActor->EditorPreviewObjectActorClass;
    } else
    {
        PreviewClass = AGridEditorPreviewObjectActor::StaticClass ();
    }
    UStaticMesh* Mesh = RuntimeActor->GetObjectMesh (ObjectData);
    UMaterialInterface* Material = RuntimeActor->GetObjectMaterial (ObjectData);
    if (!Mesh)
    {
        return;
    }
    UWorld* World = GetWorld ();
    if (!World)
    {
        return;
    }
    FTransform PlacementTransform;
    if (!RuntimeActor->GetObjectPlacementTransform (ObjectData, PlacementTransform))
    {
        return;
    }
    const FVector Location = PlacementTransform.GetLocation ();
    const FRotator Rotation = PlacementTransform.GetRotation ().Rotator ();

    FActorSpawnParameters Params;
    Params.Owner = RuntimeActor;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AGridEditorPreviewObjectActor* PreviewActor =
        World->SpawnActor<AGridEditorPreviewObjectActor> (PreviewClass, Location, Rotation, Params);

    if (!PreviewActor)
    {
        return;
    }
    PreviewActor->InitializePreviewObject (ObjectData, Mesh, Material);
    SpawnedPreviewObjects.Add (PreviewActor);
}

void UGridEditorPreviewComponent::SetHoveredObject (FGuid ObjectId)
{
    CurrentHoveredObjectId = ObjectId;
    for (AGridEditorPreviewObjectActor* Actor : SpawnedPreviewObjects)
    {
        if (!IsValid (Actor))
        {
            continue;
        }
        Actor->SetHovered (ObjectId.IsValid () && Actor->ObjectId == ObjectId);
    }
}

void UGridEditorPreviewComponent::SetSelectedObject (FGuid ObjectId)
{
    CurrentSelectedObjectId = ObjectId;
    for (AGridEditorPreviewObjectActor* Actor : SpawnedPreviewObjects)
    {
        if (!IsValid (Actor))
        {
            continue;
        }
        Actor->SetSelected (ObjectId.IsValid () && Actor->ObjectId == ObjectId);
    }
}

bool UGridEditorPreviewComponent::IsPreviewableObject (const FGridLevelObjectData& ObjectData) const
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return false;
    }
    if (!ObjectData.bInitiallyEnabled)
    {
        return false;
    }
    if (!RuntimeActor->LevelAsset->IsValidCoord (ObjectData.CellX, ObjectData.CellY))
    {
        return false;
    }
    if (!RuntimeActor->GetObjectMesh (ObjectData))
    {
        return false;
    }
    return true;
}
