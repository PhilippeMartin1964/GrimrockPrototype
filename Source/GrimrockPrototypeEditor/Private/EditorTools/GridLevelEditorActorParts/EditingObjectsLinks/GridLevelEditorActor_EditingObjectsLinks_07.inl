#include "Core/GridObjectInstanceBehavior.h"
#include "EditorTools/GridEditorCanonicalItemLevelPolicy.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"

bool AGridLevelEditorActor::FocusSelectedObject()
{
	if (!LevelAsset || !LevelAsset->TryGetTypedPlacementLocation(LastSelectedObjectId, SelectedCellX, SelectedCellY, SelectedEdge)) return false;
#if WITH_EDITOR
	if (GEditor)
	{
		FVector WorldLocation;
		if (TryGetObjectWorldLocationById(LastSelectedObjectId, WorldLocation))
		{
			const float FocusExtent = FMath::Max(50.f, LevelAsset->CellSize * 0.25f);
			GEditor->MoveViewportCamerasToBox(FBox(WorldLocation - FVector(FocusExtent), WorldLocation + FVector(FocusExtent)), false);
		}
	}
#endif
	return true;
}

bool AGridLevelEditorActor::ApplyBehaviorToSelectedObject(const FGridObjectBehaviorParams& NewBehavior)
{
	FGridWorldObjectInstance* WorldObjectInstance = LevelAsset ? LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId) : nullptr;
	if (!WorldObjectInstance) return false;
	LevelAsset->Modify();
	WorldObjectInstance->InstanceConfig.Relocation = NewBehavior.Relocation;
	WorldObjectInstance->InstanceConfig.Pit = NewBehavior.Pit;
	WorldObjectInstance->InstanceConfig.ReceptacleInitialContent = NewBehavior.Receptacle.InitialContent;
	WorldObjectInstance->InstanceConfig.bStartsUnlocked = NewBehavior.Lock.bStartsUnlocked;
	ObjectBehavior = GridObjectInstanceBehavior::Resolve(*WorldObjectInstance, FindWorldObjectDefinitionById(WorldObjectInstance->WorldObjectDefinitionId));
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::ResetSelectedObjectBehaviorFromDefinition()
{
	const FGridWorldObjectInstance* WorldObjectInstance = LevelAsset ? LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId) : nullptr;
	const UGridWorldObjectDefinitionAsset* Definition = WorldObjectInstance ? FindWorldObjectDefinitionById(WorldObjectInstance->WorldObjectDefinitionId) : nullptr;
	return Definition && ApplyBehaviorToSelectedObject(Definition->DefaultBehavior);
}

bool AGridLevelEditorActor::SetSelectedWorldObjectDefinitionId(FName NewWorldObjectDefinitionId)
{
	FGridWorldObjectInstance* WorldObjectInstance = LevelAsset ? LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId) : nullptr;
	if (!WorldObjectInstance) return false;
	LevelAsset->Modify();
	WorldObjectInstance->WorldObjectDefinitionId = NewWorldObjectDefinitionId;
	WorldObjectDefinitionId = NewWorldObjectDefinitionId;
	SelectedWorldObjectDefinitionId = NewWorldObjectDefinitionId;
	ObjectBehavior = GridObjectInstanceBehavior::Resolve(*WorldObjectInstance, FindWorldObjectDefinitionById(NewWorldObjectDefinitionId));
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionAsset(UGridItemDefinitionAsset* NewItemDefinitionAsset)
{
	if (!LevelAsset) return false;
	if (FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		LooseItemInstance->ItemDefinition = NewItemDefinitionAsset;
	}
	else if (FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		ItemSpawn->ItemDefinition = NewItemDefinitionAsset;
	}
	else return false;
	ObjectBehavior.Item.ItemDefinitionAsset = NewItemDefinitionAsset;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

const UGridItemSubsystem* AGridLevelEditorActor::ResolveEditorItemRegistry()
{
	// During PIE, use the real game registry. An unconfigured runtime cannot
	// silently fall back to old authoring data.
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			const UGridItemSubsystem* RuntimeRegistry = GameInstance->GetSubsystem<UGridItemSubsystem>();
			return RuntimeRegistry && RuntimeRegistry->IsConfigured() ? RuntimeRegistry : nullptr;
		}
	}

	// EditorPreview has no GameInstance. Use the SAME DefaultGame.ini root
	// as the preview mesh reader; hold the transient context through GC.
	if (!IsValid(EditorAuthoringItemRegistry))
	{
		UGridItemSystemAsset* ItemSystem =
			GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
		if (!IsValid(ItemSystem))
		{
			return nullptr; // No unchecked historical authoring if registry is missing.
		}
		EditorAuthoringGameInstance = NewObject<UGameInstance>(this);
		EditorAuthoringItemRegistry = NewObject<UGridItemSubsystem>(EditorAuthoringGameInstance.Get());
		TArray<FString> Errors;
		if (!EditorAuthoringItemRegistry->RegisterItemSystem(ItemSystem, Errors))
		{
			for (const FString& Error : Errors)
			{
				UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.3AN] Editor ItemSystem: %s"), *Error);
			}
			EditorAuthoringItemRegistry = nullptr;
			EditorAuthoringGameInstance = nullptr;
			return nullptr;
		}
	}
	return EditorAuthoringItemRegistry.Get();
}

bool AGridLevelEditorActor::SetSelectedObjectItemLevel(int32 NewItemLevel)
{
	if (!LevelAsset || NewItemLevel < 0 || NewItemLevel > 20)
	{
		return false;
	}
	FGridLooseItemInstance* Loose = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId);
	FGridItemSpawnInstance* Spawn = Loose ? nullptr : LevelAsset->FindItemSpawnInstanceById(LastSelectedObjectId);
	UGridItemDefinitionAsset* Definition = Loose ? Loose->ItemDefinition.Get() : (Spawn ? Spawn->ItemDefinition.Get() : nullptr);
	if (!FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(
		ResolveEditorItemRegistry(), Definition, NewItemLevel))
	{
		return false; // Known canonical item: never consult obsolete opt-in/level.
	}
	int32& AuthoredLevel = Loose ? Loose->ItemLevel : Spawn->ItemLevel;
	if (AuthoredLevel == NewItemLevel)
	{
		return true;
	}
	LevelAsset->Modify();
	AuthoredLevel = NewItemLevel;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectItemDefinitionId(FName NewItemDefinitionId)
{
	// Compatibility authoring command: resolve a definition without storing a second identity.
	if (!LevelAsset) return false;
	if (const FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId))
	{
		if (LooseItemInstance->ItemDefinition && LooseItemInstance->ItemDefinition->ItemDefinitionId == NewItemDefinitionId) return true;
	}
	if (ObjectPalette)
	{
		for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
		{
			if (Entry.DefaultItemDefinition && Entry.DefaultItemDefinition->ItemDefinitionId == NewItemDefinitionId)
				return SetSelectedObjectItemDefinitionAsset(Entry.DefaultItemDefinition);
		}
	}
	return false;
}

bool AGridLevelEditorActor::SyncSelectedItemDefinitionIdFromAsset()
{
	const FGridLooseItemInstance* LooseItemInstance = LevelAsset ? LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId) : nullptr;
	return LooseItemInstance && LooseItemInstance->ItemDefinition;
}

bool AGridLevelEditorActor::SetSelectedObjectMonsterDefinitionAsset(UGridMonsterDefinitionAsset* NewMonsterDefinitionAsset)
{
	FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset ? LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId) : nullptr;
	if (!MonsterSpawn) return false;
	LevelAsset->Modify();
	MonsterSpawn->MonsterDefinition = NewMonsterDefinitionAsset;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}
