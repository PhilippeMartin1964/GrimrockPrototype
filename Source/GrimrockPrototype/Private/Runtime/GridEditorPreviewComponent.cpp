#include "Runtime/GridEditorPreviewComponent.h"

#include "Core/GridLevelPlacementCompatibility.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
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

	auto TryAddPreview = [this](const FGridLevelObjectData& ObjectData)
	{
		if (IsPreviewableObject(ObjectData))
		{
			AddPreviewObject(ObjectData);
		}
	};

	// WORLDOBJ-MIG09-E2B: typed level placements are the only read source.
	for (const FGridWorldObjectInstance& Instance : RuntimeActor->LevelAsset->WorldObjectInstances)
	{
		TryAddPreview(GridLevelPlacementCompatibility::ToLegacyWorldObject(Instance));
	}
	for (const FGridLooseItemInstance& Instance : RuntimeActor->LevelAsset->LooseItemInstances)
	{
		TryAddPreview(GridLevelPlacementCompatibility::ToLegacyLooseItem(Instance));
	}
	for (const FGridMonsterSpawnInstance& Spawn : RuntimeActor->LevelAsset->MonsterSpawns)
	{
		if (IsPreviewableMonsterSpawn(Spawn))
		{
			AddMonsterPreviewObject(Spawn);
		}
	}
	for (const FGridItemSpawnInstance& Spawn : RuntimeActor->LevelAsset->ItemSpawns)
	{
		TryAddPreview(GridLevelPlacementCompatibility::ToLegacyItemSpawn(Spawn));
	}
	for (const FGridLogicObjectInstance& Instance : RuntimeActor->LevelAsset->LogicObjects)
	{
		TryAddPreview(GridLevelPlacementCompatibility::ToLegacyLogicObject(Instance));
	}
}

void UGridEditorPreviewComponent::AddMonsterPreviewObject(const FGridMonsterSpawnInstance& SpawnData)
{
	if (!RuntimeActor)
	{
		return;
	}

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
	if (!RuntimeActor->GetObjectPlacementTransform(ObjectData, PlacementTransform))
	{
		return;
	}

	const UGridObjectArchetypeAsset* Archetype = RuntimeActor->FindObjectArchetype(ObjectData.ArchetypeId);
	UStaticMesh* DirectItemMesh = nullptr;
	if (ObjectData.Type == EGridLevelObjectType::Item && ObjectData.ItemDefinitionAsset)
	{
		DirectItemMesh = ObjectData.ItemDefinitionAsset->WorldMesh.LoadSynchronous();
	}

	const bool bHasTargetComposition = Archetype && Archetype->HasAnyVisualPart();
	if (!DirectItemMesh && !bHasTargetComposition)
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
	if (DirectItemMesh)
	{
		// WORLDOBJ-MIG05: preview the same WorldMesh used by the runtime item actor.
		PreviewActor->InitializePreviewObject(ObjectData, DirectItemMesh);
	}
	else
	{
		PreviewActor->InitializePreviewObjectFromArchetype(ObjectData, Archetype);
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

bool UGridEditorPreviewComponent::IsPreviewableMonsterSpawn(const FGridMonsterSpawnInstance& SpawnData) const
{
	return RuntimeActor && RuntimeActor->LevelAsset && SpawnData.bInitiallyEnabled && RuntimeActor->LevelAsset->IsValidCoord(SpawnData.CellX, SpawnData.CellY);
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

	// WORLDOBJ-MIG05: a direct collectible is previewable without any world-object archetype.
	if (ObjectData.Type == EGridLevelObjectType::Item && ObjectData.ItemDefinitionAsset && !ObjectData.ItemDefinitionAsset->WorldMesh.IsNull())
	{
		return true;
	}

	const UGridObjectArchetypeAsset* Archetype = RuntimeActor->FindObjectArchetype(ObjectData.ArchetypeId);
	return Archetype && Archetype->HasAnyVisualPart();
}
