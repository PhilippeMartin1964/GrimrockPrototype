#include "Runtime/GridEditorPreviewComponent.h"

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPlacementTransformResolver.h"
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

	for (const FGridWorldObjectInstance& Instance : RuntimeActor->LevelAsset->WorldObjectInstances)
	{
		if (IsPreviewableWorldObject(Instance))
		{
			AddWorldObjectPreview(Instance);
		}
	}
	for (const FGridLooseItemInstance& Instance : RuntimeActor->LevelAsset->LooseItemInstances)
	{
		if (IsPreviewableLooseItem(Instance))
		{
			AddLooseItemPreview(Instance);
		}
	}
	for (const FGridMonsterSpawnInstance& Spawn : RuntimeActor->LevelAsset->MonsterSpawns)
	{
		if (IsPreviewableMonsterSpawn(Spawn))
		{
			AddMonsterPreviewObject(Spawn);
		}
	}
}

void UGridEditorPreviewComponent::AddWorldObjectPreview(const FGridWorldObjectInstance& Instance)
{
	if (!RuntimeActor)
	{
		return;
	}

	const UGridWorldObjectDefinitionAsset* Definition = RuntimeActor->FindWorldObjectDefinition(Instance.WorldObjectDefinitionId);
	FTransform PlacementTransform;
	if (!Definition || !GridPlacementTransformResolver::ResolveWorldObject(*RuntimeActor, Instance, PlacementTransform))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<AGridEditorPreviewObjectActor> PreviewClass = RuntimeActor->EditorPreviewObjectActorClass;
	if (!PreviewClass)
	{
		PreviewClass = AGridEditorPreviewObjectActor::StaticClass();
	}
	FActorSpawnParameters Params;
	Params.Owner = RuntimeActor;
	Params.ObjectFlags = RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGridEditorPreviewObjectActor* PreviewActor = World->SpawnActor<AGridEditorPreviewObjectActor>(
		PreviewClass, PlacementTransform.GetLocation(), PlacementTransform.GetRotation().Rotator(), Params);
	if (!PreviewActor)
	{
		return;
	}

	PreviewActor->InitializePreviewObjectFromDefinition(Instance.InstanceId, Instance.Type, Definition, &Instance.InstanceConfig);
	SpawnedPreviewObjects.Add(PreviewActor);
}

void UGridEditorPreviewComponent::AddLooseItemPreview(const FGridLooseItemInstance& Instance)
{
	if (!RuntimeActor || !Instance.ItemDefinition)
	{
		return;
	}

	FTransform PlacementTransform;
	if (!GridPlacementTransformResolver::ResolveLooseItem(*RuntimeActor, Instance, PlacementTransform))
	{
		return;
	}
	UStaticMesh* ItemMesh = Instance.ItemDefinition->WorldMesh.LoadSynchronous();
	if (!ItemMesh)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<AGridEditorPreviewObjectActor> PreviewClass = RuntimeActor->EditorPreviewObjectActorClass;
	if (!PreviewClass)
	{
		PreviewClass = AGridEditorPreviewObjectActor::StaticClass();
	}
	FActorSpawnParameters Params;
	Params.Owner = RuntimeActor;
	Params.ObjectFlags = RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGridEditorPreviewObjectActor* PreviewActor = World->SpawnActor<AGridEditorPreviewObjectActor>(
		PreviewClass, PlacementTransform.GetLocation(), PlacementTransform.GetRotation().Rotator(), Params);
	if (!PreviewActor)
	{
		return;
	}

	PreviewActor->InitializePreviewObject(Instance.InstanceId, EGridLevelObjectType::Item, ItemMesh);
	SpawnedPreviewObjects.Add(PreviewActor);
}

void UGridEditorPreviewComponent::AddMonsterPreviewObject(const FGridMonsterSpawnInstance& SpawnData)
{
	if (!RuntimeActor)
	{
		return;
	}

	TSubclassOf<AGridEditorPreviewObjectActor> PreviewClass = RuntimeActor->EditorPreviewObjectActorClass;
	if (!PreviewClass)
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
	if (!RuntimeActor->ResolveMonsterSpawn(SpawnData, MonsterDefinition, MonsterActorClass, MonsterSpawnError) ||
		!RuntimeActor->GetMonsterSpawnTransform(SpawnData, PlacementTransform))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GridMonsterSpawn] Preview skipped SpawnId=%s Reason=%s"), *SpawnData.SpawnId.ToString(),
			MonsterSpawnError.IsEmpty() ? TEXT("InvalidTransform") : *MonsterSpawnError);
		return;
	}
	if (!MonsterDefinition || MonsterDefinition->SkeletalMesh.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GridMonsterSpawn] Preview skipped SpawnId=%s Definition=%s Reason=MissingSkeletalMesh"),
			*SpawnData.SpawnId.ToString(), MonsterDefinition ? *MonsterDefinition->MonsterId.ToString() : TEXT("None"));
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = RuntimeActor;
	Params.ObjectFlags = RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGridEditorPreviewObjectActor* PreviewActor = World->SpawnActor<AGridEditorPreviewObjectActor>(
		PreviewClass, PlacementTransform.GetLocation(), PlacementTransform.GetRotation().Rotator(), Params);
	if (!PreviewActor)
	{
		return;
	}

	PreviewActor->InitializeMonsterPreviewObject(SpawnData, MonsterDefinition);
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

bool UGridEditorPreviewComponent::IsPreviewableWorldObject(const FGridWorldObjectInstance& Instance) const
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset || !Instance.bInitiallyEnabled ||
		!RuntimeActor->LevelAsset->IsValidCoord(Instance.CellX, Instance.CellY))
	{
		return false;
	}
	const UGridWorldObjectDefinitionAsset* Definition = RuntimeActor->FindWorldObjectDefinition(Instance.WorldObjectDefinitionId);
	return Definition && Definition->HasAnyVisualPart();
}

bool UGridEditorPreviewComponent::IsPreviewableLooseItem(const FGridLooseItemInstance& Instance) const
{
	return RuntimeActor && RuntimeActor->LevelAsset && Instance.bInitiallyEnabled && RuntimeActor->LevelAsset->IsValidCoord(Instance.CellX, Instance.CellY) &&
		Instance.ItemDefinition && !Instance.ItemDefinition->WorldMesh.IsNull();
}

bool UGridEditorPreviewComponent::IsPreviewableMonsterSpawn(const FGridMonsterSpawnInstance& SpawnData) const
{
	return RuntimeActor && RuntimeActor->LevelAsset && SpawnData.bInitiallyEnabled && RuntimeActor->LevelAsset->IsValidCoord(SpawnData.CellX, SpawnData.CellY);
}
