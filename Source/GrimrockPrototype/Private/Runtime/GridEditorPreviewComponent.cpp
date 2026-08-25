#include "Runtime/GridEditorPreviewComponent.h"

#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "EngineUtils.h"

UGridEditorPreviewComponent::UGridEditorPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridEditorPreviewComponent::Initialize(AGridLevelRuntimeActor* InRuntimeActor)
{
	RuntimeActor = InRuntimeActor;
}

void UGridEditorPreviewComponent::ClearPreviewObjects()
{
	for (AGridEditorPreviewObjectActor* Actor : SpawnedPreviewObjects)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedPreviewObjects.Empty();
	CurrentSelectedObjectId.Invalidate();
	CurrentHoveredObjectId.Invalidate();
}

void UGridEditorPreviewComponent::RebuildPreviewObjects()
{
	CleanupOrphanPreviewObjects();
	ClearPreviewObjects();
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return;
	}
	for (const FGridLevelObjectData& ObjectData : RuntimeActor->LevelAsset->Objects)
	{
		if (!IsPreviewableObject(ObjectData))
		{
			continue;
		}
		AddPreviewObject(ObjectData);
	}
}

void UGridEditorPreviewComponent::AddPreviewObject(const FGridLevelObjectData& ObjectData)
{
	TSubclassOf<AGridEditorPreviewObjectActor> PreviewClass;

	if (RuntimeActor->EditorPreviewObjectActorClass)
	{
		PreviewClass = RuntimeActor->EditorPreviewObjectActorClass;
	}
	else
	{
		PreviewClass = AGridEditorPreviewObjectActor::StaticClass();
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTransform PlacementTransform;
	UGridMonsterDefinitionAsset* MonsterDefinition = nullptr;
	TSubclassOf<AGridMonsterActor> MonsterActorClass;
	FString MonsterSpawnError;
	const bool bMonsterSpawn = ObjectData.Type == EGridLevelObjectType::MonsterSpawn;
	if (bMonsterSpawn)
	{
		if (!RuntimeActor->ResolveMonsterSpawn(ObjectData, MonsterDefinition, MonsterActorClass, MonsterSpawnError) ||
			!RuntimeActor->GetMonsterSpawnTransform(ObjectData, PlacementTransform))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GridMonsterSpawn] Preview skipped SpawnId=%s Reason=%s"), *ObjectData.ObjectId.ToString(),
				MonsterSpawnError.IsEmpty() ? TEXT("InvalidTransform") : *MonsterSpawnError);
			return;
		}
		if (MonsterDefinition->SkeletalMesh.IsNull())
		{
			UE_LOG(LogTemp, Warning, TEXT("[GridMonsterSpawn] Preview skipped SpawnId=%s Definition=%s Reason=MissingSkeletalMesh"),
				*ObjectData.ObjectId.ToString(), *MonsterDefinition->MonsterId.ToString());
			return;
		}
	}
	else if (!RuntimeActor->GetObjectPlacementTransform(ObjectData, PlacementTransform))
	{
		return;
	}

	UStaticMesh* Mesh = bMonsterSpawn ? nullptr : RuntimeActor->GetObjectMesh(ObjectData);
	UMaterialInterface* Material = bMonsterSpawn ? nullptr : RuntimeActor->GetObjectMaterial(ObjectData);
	if (!bMonsterSpawn && !Mesh)
	{
		return;
	}
	const FVector Location = PlacementTransform.GetLocation();
	const FRotator Rotation = PlacementTransform.GetRotation().Rotator();

	FActorSpawnParameters Params;
	Params.Owner = RuntimeActor;
	Params.ObjectFlags = RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGridEditorPreviewObjectActor* PreviewActor = World->SpawnActor<AGridEditorPreviewObjectActor>(PreviewClass, Location, Rotation, Params);

	if (!PreviewActor)
	{
		return;
	}
	if (bMonsterSpawn)
	{
		PreviewActor->InitializeMonsterPreviewObject(ObjectData, MonsterDefinition);
	}
	else
	{
		PreviewActor->InitializePreviewObject(ObjectData, Mesh, Material);
	}
	SpawnedPreviewObjects.Add(PreviewActor);
}

void UGridEditorPreviewComponent::CleanupOrphanPreviewObjects()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AGridEditorPreviewObjectActor> It(World); It; ++It)
	{
		AGridEditorPreviewObjectActor* PreviewActor = *It;

		if (!IsValid(PreviewActor))
		{
			continue;
		}

		if (PreviewActor->GetOwner() == RuntimeActor)
		{
			PreviewActor->Destroy();
		}
	}
	SpawnedPreviewObjects.RemoveAll(
		[](const TObjectPtr<AGridEditorPreviewObjectActor>& Actor)
		{
			return !IsValid(Actor);
		});
}

void UGridEditorPreviewComponent::SetHoveredObject(FGuid ObjectId)
{
	CurrentHoveredObjectId = ObjectId;
	for (AGridEditorPreviewObjectActor* Actor : SpawnedPreviewObjects)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		Actor->SetHovered(ObjectId.IsValid() && Actor->ObjectId == ObjectId);
	}
}

void UGridEditorPreviewComponent::SetSelectedObject(FGuid ObjectId)
{
	CurrentSelectedObjectId = ObjectId;
	for (AGridEditorPreviewObjectActor* Actor : SpawnedPreviewObjects)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		Actor->SetSelected(ObjectId.IsValid() && Actor->ObjectId == ObjectId);
	}
}

bool UGridEditorPreviewComponent::IsPreviewableObject(const FGridLevelObjectData& ObjectData) const
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return false;
	}
	if (!ObjectData.bInitiallyEnabled)
	{
		return false;
	}
	if (!RuntimeActor->LevelAsset->IsValidCoord(ObjectData.CellX, ObjectData.CellY))
	{
		return false;
	}
	if (ObjectData.Type == EGridLevelObjectType::MonsterSpawn)
	{
		return true;
	}
	if (!RuntimeActor->GetObjectMesh(ObjectData))
	{
		return false;
	}
	return true;
}
