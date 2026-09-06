#include "EditorTools/GridWorldObjectMIG08MigrationService.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	void AddUniqueId(const FGuid& Id, const FString& Context, TSet<FGuid>& SeenIds, FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (!Id.IsValid())
		{
			Result.Errors.Add(FString::Printf(TEXT("%s has an invalid persistent id."), *Context));
			return;
		}
		if (SeenIds.Contains(Id))
		{
			Result.Errors.Add(FString::Printf(TEXT("%s reuses persistent id %s."), *Context, *Id.ToString(EGuidFormats::DigitsWithHyphens)));
			return;
		}
		SeenIds.Add(Id);
	}

	void MigratePaletteIconToItem(FGridObjectPaletteEntry& Entry, UGridItemDefinitionAsset& ItemDefinition, FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (!Entry.Icon)
		{
			return;
		}

		if (ItemDefinition.Icon.IsNull())
		{
#if WITH_EDITOR
			ItemDefinition.Modify();
#endif
			ItemDefinition.Icon = Entry.Icon.Get();
#if WITH_EDITOR
			ItemDefinition.MarkPackageDirty();
#endif
			Result.bChanged = true;
			Result.Changes.Add(FString::Printf(TEXT("Palette entry '%s': promoted palette icon into ItemDefinition '%s'."), *Entry.EntryId.ToString(),
				*ItemDefinition.GetPathName()));
		}
		else if (ItemDefinition.Icon.ToSoftObjectPath().ToString() != Entry.Icon->GetPathName())
		{
			Result.Warnings.Add(FString::Printf(TEXT("Palette entry '%s': ItemDefinition '%s' already owns a different icon; existing ItemDefinition icon kept."),
				*Entry.EntryId.ToString(), *ItemDefinition.GetPathName()));
		}

		Entry.Icon = nullptr;
		Result.bChanged = true;
	}

	void MigrateLegacyItemWorldMesh(
		const FGridObjectPaletteEntry& Entry, const UGridObjectArchetypeAsset& LegacyArchetype, UGridItemDefinitionAsset& ItemDefinition,
		FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (!ItemDefinition.WorldMesh.IsNull())
		{
			if (LegacyArchetype.StaticPart.Mesh && ItemDefinition.WorldMesh.ToSoftObjectPath().ToString() != LegacyArchetype.StaticPart.Mesh->GetPathName())
			{
				Result.Warnings.Add(FString::Printf(TEXT("Palette entry '%s': ItemDefinition '%s' already owns a different WorldMesh; existing ItemDefinition mesh kept."),
					*Entry.EntryId.ToString(), *ItemDefinition.GetPathName()));
			}
			return;
		}

		if (!LegacyArchetype.StaticPart.Mesh)
		{
			Result.Warnings.Add(FString::Printf(TEXT("Palette entry '%s': legacy item archetype '%s' has no StaticPart mesh to promote into ItemDefinition '%s'."),
				*Entry.EntryId.ToString(), *LegacyArchetype.GetPathName(), *ItemDefinition.GetPathName()));
			return;
		}

#if WITH_EDITOR
		ItemDefinition.Modify();
#endif
		ItemDefinition.WorldMesh = LegacyArchetype.StaticPart.Mesh.Get();
#if WITH_EDITOR
		ItemDefinition.MarkPackageDirty();
#endif
		Result.bChanged = true;
		Result.Changes.Add(FString::Printf(TEXT("Palette entry '%s': promoted StaticPart mesh from legacy item archetype '%s' into ItemDefinition '%s'."),
			*Entry.EntryId.ToString(), *LegacyArchetype.GetPathName(), *ItemDefinition.GetPathName()));
	}
}

FGridWorldObjectMIG08MigrationResult FGridWorldObjectMIG08MigrationService::MigrateLevelAsset(UGridLevelAsset& LevelAsset)
{
	FGridWorldObjectMIG08MigrationResult Result;
	const bool bWasTyped = LevelAsset.bTypedPlacementStorageAuthoritative;
	const int32 LegacyCountBefore = LevelAsset.Objects.Num();
	TSet<FGuid> LegacyIdsBefore;

	if (!bWasTyped)
	{
		for (const FGridLevelObjectData& Object : LevelAsset.Objects)
		{
			if (Object.ObjectId.IsValid())
			{
				LegacyIdsBefore.Add(Object.ObjectId);
			}
		}

#if WITH_EDITOR
		LevelAsset.Modify();
#endif
		LevelAsset.EnableTypedPlacementStorageFromLegacy();
#if WITH_EDITOR
		LevelAsset.MarkPackageDirty();
#endif
		Result.bChanged = true;
		Result.Changes.Add(FString::Printf(TEXT("Level '%s': migrated %d legacy placements into typed storage."), *LevelAsset.GetPathName(), LegacyCountBefore));
	}
	else
	{
		LevelAsset.RefreshLegacyObjectMirrorFromTyped();
	}

	const TArray<FGridLevelObjectData>& CompatibilityView = LevelAsset.GetObjectCompatibilityView();
	if (LevelAsset.GetTypedPlacementCount() != CompatibilityView.Num())
	{
		Result.Errors.Add(FString::Printf(TEXT("Level '%s': typed placement count %d does not match compatibility mirror count %d."),
			*LevelAsset.GetPathName(), LevelAsset.GetTypedPlacementCount(), CompatibilityView.Num()));
	}

	if (!bWasTyped && LegacyCountBefore != CompatibilityView.Num())
	{
		Result.Errors.Add(FString::Printf(TEXT("Level '%s': migration changed placement count from %d to %d."), *LevelAsset.GetPathName(), LegacyCountBefore,
			CompatibilityView.Num()));
	}

	if (!bWasTyped)
	{
		for (const FGuid& LegacyId : LegacyIdsBefore)
		{
			if (!CompatibilityView.ContainsByPredicate(
					[&LegacyId](const FGridLevelObjectData& Object)
					{
						return Object.ObjectId == LegacyId;
					}))
			{
				Result.Errors.Add(FString::Printf(TEXT("Level '%s': migration lost legacy ObjectId %s."), *LevelAsset.GetPathName(),
					*LegacyId.ToString(EGuidFormats::DigitsWithHyphens)));
			}
		}
	}

	TSet<FGuid> SeenIds;
	for (const FGridWorldObjectInstance& Instance : LevelAsset.WorldObjectInstances)
	{
		AddUniqueId(Instance.InstanceId, TEXT("WorldObjectInstance"), SeenIds, Result);
		if (Instance.WorldObjectDefinitionId.IsNone())
		{
			Result.Errors.Add(FString::Printf(TEXT("Level '%s': WorldObjectInstance %s has no WorldObjectDefinitionId."), *LevelAsset.GetPathName(),
				*Instance.InstanceId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
	}
	for (const FGridLooseItemInstance& Instance : LevelAsset.LooseItemInstances)
	{
		AddUniqueId(Instance.InstanceId, TEXT("LooseItemInstance"), SeenIds, Result);
		if (!Instance.ItemDefinition)
		{
			Result.Errors.Add(FString::Printf(TEXT("Level '%s': LooseItemInstance %s has no ItemDefinition."), *LevelAsset.GetPathName(),
				*Instance.InstanceId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
	}
	for (const FGridMonsterSpawnInstance& Spawn : LevelAsset.MonsterSpawns)
	{
		AddUniqueId(Spawn.SpawnId, TEXT("MonsterSpawn"), SeenIds, Result);
		if (!Spawn.MonsterDefinition)
		{
			Result.Errors.Add(FString::Printf(TEXT("Level '%s': MonsterSpawn %s has no MonsterDefinition."), *LevelAsset.GetPathName(),
				*Spawn.SpawnId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
	}
	for (const FGridItemSpawnInstance& Spawn : LevelAsset.ItemSpawns)
	{
		AddUniqueId(Spawn.SpawnId, TEXT("ItemSpawn"), SeenIds, Result);
		if (!Spawn.ItemDefinition)
		{
			Result.Errors.Add(FString::Printf(TEXT("Level '%s': ItemSpawn %s has no ItemDefinition."), *LevelAsset.GetPathName(),
				*Spawn.SpawnId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
	}
	for (const FGridLogicObjectInstance& Instance : LevelAsset.LogicObjects)
	{
		AddUniqueId(Instance.InstanceId, TEXT("LogicObject"), SeenIds, Result);
	}

	return Result;
}

FGridWorldObjectMIG08MigrationResult FGridWorldObjectMIG08MigrationService::MigratePaletteAsset(UGridObjectPaletteAsset& PaletteAsset)
{
	FGridWorldObjectMIG08MigrationResult Result;

	for (FGridObjectPaletteEntry& Entry : PaletteAsset.Entries)
	{
		UGridObjectArchetypeAsset* LegacyArchetype = Entry.DefaultArchetype.Get();
		UGridItemDefinitionAsset* ItemDefinition = Entry.DefaultItemDefinition.Get();

		if (ItemDefinition && LegacyArchetype && LegacyArchetype->SupportedType != EGridLevelObjectType::Item)
		{
			Result.Errors.Add(FString::Printf(TEXT("Palette '%s' entry '%s' mixes ItemDefinition '%s' with non-item archetype '%s'."),
				*PaletteAsset.GetPathName(), *Entry.EntryId.ToString(), *ItemDefinition->GetPathName(), *LegacyArchetype->GetPathName()));
			continue;
		}

		if (!ItemDefinition && LegacyArchetype && LegacyArchetype->SupportedType == EGridLevelObjectType::Item)
		{
			ItemDefinition = LegacyArchetype->DefaultBehavior.Item.ItemDefinitionAsset.Get();
			if (!ItemDefinition)
			{
				Result.Errors.Add(FString::Printf(TEXT("Palette '%s' entry '%s' uses legacy item archetype '%s' but no ItemDefinition can be resolved."),
					*PaletteAsset.GetPathName(), *Entry.EntryId.ToString(), *LegacyArchetype->GetPathName()));
				continue;
			}
		}

		if (!ItemDefinition)
		{
			continue;
		}

		if (LegacyArchetype)
		{
			MigrateLegacyItemWorldMesh(Entry, *LegacyArchetype, *ItemDefinition, Result);
		}
		MigratePaletteIconToItem(Entry, *ItemDefinition, Result);

		const bool bNeedsDirectItemContract = Entry.DefaultItemDefinition != ItemDefinition || Entry.DefaultArchetype != nullptr;
		if (bNeedsDirectItemContract)
		{
			Entry.DefaultItemDefinition = ItemDefinition;
			Entry.DefaultArchetype = nullptr;
			Result.bChanged = true;
			Result.Changes.Add(FString::Printf(TEXT("Palette '%s' entry '%s': converted to direct ItemDefinition '%s'."), *PaletteAsset.GetPathName(),
				*Entry.EntryId.ToString(), *ItemDefinition->GetPathName()));
		}
	}

	TMap<const UGridItemDefinitionAsset*, int32> ItemUsageCounts;
	for (const FGridObjectPaletteEntry& Entry : PaletteAsset.Entries)
	{
		if (Entry.DefaultItemDefinition)
		{
			++ItemUsageCounts.FindOrAdd(Entry.DefaultItemDefinition.Get());
		}
		if (!Entry.IsValidEntry())
		{
			Result.Errors.Add(FString::Printf(TEXT("Palette '%s' entry '%s' is invalid after MIG08 migration."), *PaletteAsset.GetPathName(),
				*Entry.EntryId.ToString()));
		}
	}

	for (const TPair<const UGridItemDefinitionAsset*, int32>& Pair : ItemUsageCounts)
	{
		if (Pair.Value > 1 && Pair.Key)
		{
			Result.Warnings.Add(FString::Printf(TEXT("Palette '%s': ItemDefinition '%s' is exposed by %d palette entries; review whether these entries are intentionally distinct."),
				*PaletteAsset.GetPathName(), *Pair.Key->GetPathName(), Pair.Value));
		}
	}

	if (Result.bChanged)
	{
#if WITH_EDITOR
		PaletteAsset.Modify();
		PaletteAsset.MarkPackageDirty();
#endif
	}

	return Result;
}
