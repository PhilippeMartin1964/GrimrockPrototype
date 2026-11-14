#include "Runtime/GridEditorPreviewComponent.h"

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/Combat/GridWorldItemVisualResolver.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridEditorLooseItemPreviewReadModel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPlacementTransformResolver.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridRuntimeObject, Log, All);

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

const UGridItemSubsystem* UGridEditorPreviewComponent::ResolvePreviewItemRegistry()
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UGridItemSubsystem>();
		}
	}

	// Editor preview worlds do not have a GameInstance. Build a transient,
	// GC-owned subsystem from the SAME configured ItemSystem, not from a
	// second per-weapon catalogue. Reuse it for successive preview rebuilds.
	if (!IsValid(EditorPreviewItemRegistry))
	{
		UGridItemSystemAsset* System =
			GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
		if (!IsValid(System))
		{
			UE_LOG(LogGridRuntimeObject, Warning,
				TEXT("[ARM-CLEAN06.3Z] Editor preview has no configured DA_ItemSystem."));
			return nullptr;
		}
		EditorPreviewGameInstance = NewObject<UGameInstance>(this);
		EditorPreviewItemRegistry = NewObject<UGridItemSubsystem>(EditorPreviewGameInstance.Get());
		TArray<FString> Errors;
		if (!EditorPreviewItemRegistry->RegisterItemSystem(System, Errors))
		{
			for (const FString& Error : Errors)
			{
				UE_LOG(LogGridRuntimeObject, Error,
					TEXT("[ARM-CLEAN06.3Z] Preview ItemSystem: %s"), *Error);
			}
			EditorPreviewItemRegistry = nullptr;
			EditorPreviewGameInstance = nullptr;
			return nullptr;
		}
	}
	return EditorPreviewItemRegistry.Get();
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
	const FGridEditorLooseItemPreviewReadView Canonical =
		FGridEditorLooseItemPreviewReadModel::Resolve(ResolvePreviewItemRegistry(), Instance);
	UStaticMesh* PreviewMesh = nullptr;
	FTransform RelativeVisualTransform = FTransform::Identity;
	if (Canonical.bCanonicalIdentity)
	{
		if (!Canonical.bResolved)
		{
			return; // Registered identity: never draw an obsolete DA_Weapon mesh.
		}
		PreviewMesh = Canonical.WorldMesh.LoadSynchronous();
		RelativeVisualTransform = Canonical.WorldTransform;
	}
	else
	{
		const FGridResolvedWorldItemMesh Legacy =
			FGridWorldItemVisualResolver::Resolve(*Instance.ItemDefinition);
		PreviewMesh = Legacy.Mesh;
		RelativeVisualTransform = Legacy.RelativeTransform;
	}
	if (!PreviewMesh)
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

	PreviewActor->InitializePreviewObject(Instance.InstanceId, EGridLevelObjectType::Item, PreviewMesh);
	if (PreviewActor->MeshComponent)
	{
		PreviewActor->MeshComponent->SetRelativeTransform(RelativeVisualTransform);
	}
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
		UE_LOG(LogGridRuntimeObject, Warning, TEXT("[GridMonsterSpawn] Preview skipped SpawnId=%s Reason=%s"), *SpawnData.SpawnId.ToString(),
			MonsterSpawnError.IsEmpty() ? TEXT("InvalidTransform") : *MonsterSpawnError);
		return;
	}
	if (!MonsterDefinition || MonsterDefinition->SkeletalMesh.IsNull())
	{
		UE_LOG(LogGridRuntimeObject, Warning, TEXT("[GridMonsterSpawn] Preview skipped SpawnId=%s Definition=%s Reason=MissingSkeletalMesh"),
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
	if (!RuntimeActor || !RuntimeActor->LevelAsset || !RuntimeActor->LevelAsset->IsValidCoord(Instance.CellX, Instance.CellY))
	{
		return false;
	}
	const UGridWorldObjectDefinitionAsset* Definition = RuntimeActor->FindWorldObjectDefinition(Instance.WorldObjectDefinitionId);
	return Definition && Definition->HasAnyVisualPart();
}

bool UGridEditorPreviewComponent::IsPreviewableLooseItem(const FGridLooseItemInstance& Instance)
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset ||
		!RuntimeActor->LevelAsset->IsValidCoord(Instance.CellX, Instance.CellY) ||
		!IsValid(Instance.ItemDefinition.Get()))
	{
		return false;
	}
	const FGridEditorLooseItemPreviewReadView Canonical =
		FGridEditorLooseItemPreviewReadModel::Resolve(ResolvePreviewItemRegistry(), Instance);
	if (Canonical.bCanonicalIdentity)
	{
		return Canonical.bResolved && !Canonical.WorldMesh.IsNull();
	}
	return !Instance.ItemDefinition->WorldMesh.IsNull() ||
		(Instance.ItemDefinition->bUseWeaponVisualCatalogue &&
			!Instance.ItemDefinition->WeaponVisualCatalogue.IsNull());
}

bool UGridEditorPreviewComponent::IsPreviewableMonsterSpawn(const FGridMonsterSpawnInstance& SpawnData) const
{
	return RuntimeActor && RuntimeActor->LevelAsset && SpawnData.bSpawnAtStart && RuntimeActor->LevelAsset->IsValidCoord(SpawnData.CellX, SpawnData.CellY);
}
