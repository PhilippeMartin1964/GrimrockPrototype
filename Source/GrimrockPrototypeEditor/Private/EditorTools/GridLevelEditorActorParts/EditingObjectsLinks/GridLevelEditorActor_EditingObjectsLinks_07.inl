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
	FGridLooseItemInstance* Loose = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId);
	FGridItemSpawnInstance* Spawn = Loose ? nullptr : LevelAsset->FindItemSpawnInstanceById(LastSelectedObjectId);
	if (!Loose && !Spawn) return false;
	// Reflected compatibility entrypoint: no old definition may be attached.
	return NewItemDefinitionAsset == nullptr;
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
	if (!Loose && !Spawn)
	{
		return false;
	}
	const FName CanonicalId = Loose ? Loose->CanonicalItemId : Spawn->CanonicalItemId;
	const bool bAllowed = (Loose ? Loose->Quantity : Spawn->Quantity) > 0 &&
		FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
			ResolveEditorItemRegistry(), CanonicalId, NewItemLevel);
	if (!bAllowed)
	{
		return false; // Persisted ARM N0 / non-ARM N1 cannot be authored.
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
	if (!LevelAsset || NewItemDefinitionId.IsNone()) return false;
	FGridLooseItemInstance* Loose = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId);
	FGridItemSpawnInstance* Spawn = Loose ? nullptr : LevelAsset->FindItemSpawnInstanceById(LastSelectedObjectId);
	if (!Loose && !Spawn) return false;

	const FName PreviousCanonicalId = Loose ? Loose->CanonicalItemId : Spawn->CanonicalItemId;
	if (PreviousCanonicalId == NewItemDefinitionId)
		return (Loose ? Loose->Quantity : Spawn->Quantity) > 0 &&
			FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
				ResolveEditorItemRegistry(), NewItemDefinitionId, Loose ? Loose->ItemLevel : Spawn->ItemLevel);

	const UGridItemSubsystem* Registry = ResolveEditorItemRegistry();
	if (Registry && Registry->HasCanonicalItem(NewItemDefinitionId))
	{
		const int32 CurrentLevel = Loose ? Loose->ItemLevel : Spawn->ItemLevel;
		int32 AuthoredLevel = CurrentLevel;
		if (!FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
			Registry, NewItemDefinitionId, AuthoredLevel))
		{
			// Changing an item's identity is a NEW authoring operation, not
			// restoring a persisted item: explicitly select physical N0 for
			// unlevelled families, otherwise N1. Never persist ARM N0.
			AuthoredLevel = FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
				Registry, NewItemDefinitionId, 0) ? 0 : 1;
			if (!FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
				Registry, NewItemDefinitionId, AuthoredLevel))
			{
				return false;
			}
		}
		FGridResolvedItem Resolved;
		const int32 Quantity = Loose ? Loose->Quantity : Spawn->Quantity;
		if (!Registry->ResolveItem(NewItemDefinitionId, AuthoredLevel, Resolved) ||
			!Resolved.IsValid() || Quantity < 1 ||
			Quantity > FMath::Max(1, Resolved.MaxStack))
		{
			return false; // Changing ID must not silently discard stack units.
		}
		LevelAsset->Modify();
		if (Loose)
		{
			Loose->CanonicalItemId = NewItemDefinitionId;
			Loose->ItemLevel = AuthoredLevel;
		}
		else
		{
			Spawn->CanonicalItemId = NewItemDefinitionId;
			Spawn->ItemLevel = AuthoredLevel;
		}
		ObjectBehavior.Item.ItemDefinitionAsset = nullptr;
		LevelAsset->MarkPackageDirty();
		RebuildPreview();
		return true;
	}

	return false;
}

bool AGridLevelEditorActor::SyncSelectedItemDefinitionIdFromAsset()
{
	return false; // Historic asset synchronization is no longer an authoring operation.
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
