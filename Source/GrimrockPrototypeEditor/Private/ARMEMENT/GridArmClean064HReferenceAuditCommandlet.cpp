#include "ARMEMENT/GridArmClean064HReferenceAuditCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* CanonicalRoot =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
	const FString LevelPrefix =
		TEXT("Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/");
	const FString PaletteRelative =
		TEXT("Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset");

	FString ObjectPathFromRelative(const FString& Relative)
	{
		FString Package = Relative;
		if (!Package.RemoveFromStart(TEXT("Content/")) ||
			!Package.RemoveFromEnd(TEXT(".uasset")))
		{
			return FString();
		}
		return FString(TEXT("/Game/")) + Package + TEXT(".") +
			FPackageName::GetShortName(FString(TEXT("/Game/")) + Package);
	}

	bool IsSaveProperty(const UStruct* Owner, FName PropertyName)
	{
		const FProperty* Property = FindFProperty<FProperty>(Owner, PropertyName);
		return Property && Property->HasAnyPropertyFlags(CPF_SaveGame);
	}

	struct FReferenceCounts
	{
		int32 Total = 0;
		int32 Candidate = 0;
		int32 Unmapped = 0;
		int32 EmptyId = 0;
		int32 Missing = 0;
		int32 InvalidLevelOrQuantity = 0;
		int32 ArchetypeMismatch = 0;
	};

	void AddCount(FReferenceCounts& Counts, const FString& Status)
	{
		++Counts.Total;
		if (Status == TEXT("ID_CANDIDATE_ONLY")) { ++Counts.Candidate; }
		else if (Status == TEXT("NO_CANONICAL_ID")) { ++Counts.Unmapped; }
		else if (Status == TEXT("EMPTY_LEGACY_ID")) { ++Counts.EmptyId; }
		else if (Status == TEXT("NO_LEGACY_DEFINITION")) { ++Counts.Missing; }
		else if (Status == TEXT("INVALID_LEVEL_OR_QUANTITY")) { ++Counts.InvalidLevelOrQuantity; }
		else if (Status == TEXT("ARCHETYPE_MISMATCH")) { ++Counts.ArchetypeMismatch; }
	}

	TSharedPtr<FJsonObject> ReadInput(const FString& Path)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			return nullptr;
		}
		TSharedPtr<FJsonObject> Parsed;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Parsed))
		{
			return nullptr;
		}
		return Parsed;
	}
}

UGridArmClean064HReferenceAuditCommandlet::UGridArmClean064HReferenceAuditCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean064HReferenceAuditCommandlet::Main(const FString& Params)
{
	FString Input, Output;
	if (!FParse::Value(*Params, TEXT("Input="), Input) ||
		!FParse::Value(*Params, TEXT("Output="), Output) ||
		Input.IsEmpty() || Output.IsEmpty() || FPaths::IsSamePath(Input, Output))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Distinct private Input/Output required."));
		return 1;
	}
	const TSharedPtr<FJsonObject> Source = ReadInput(Input);
	const TArray<TSharedPtr<FJsonValue>>* Files = nullptr;
	if (!Source.IsValid() ||
		Source->GetStringField(TEXT("schema")) != TEXT("ARM-CLEAN06.4G/private-binary-snapshot/v1") ||
		!Source->TryGetArrayField(TEXT("source_files"), Files) ||
		!Files || Files->Num() != 84 ||
		Source->GetIntegerField(TEXT("legacy_file_count")) != 56 ||
		Source->GetIntegerField(TEXT("canonical_file_count")) != 28)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Private 06.4G input invalid."));
		return 2;
	}
	TArray<FString> LevelFiles;
	bool bPalettePresent = false;
	for (const TSharedPtr<FJsonValue>& File : *Files)
	{
		const TSharedPtr<FJsonObject> Record = File.IsValid() ? File->AsObject() : nullptr;
		if (!Record.IsValid()) { return 2; }
		FString Relative;
		if (!Record->TryGetStringField(TEXT("relative"), Relative)) { return 2; }
		if (Relative.StartsWith(LevelPrefix) && Relative.EndsWith(TEXT(".uasset")))
		{
			LevelFiles.Add(Relative);
		}
		if (Relative == PaletteRelative) { bPalettePresent = true; }
	}
	if (LevelFiles.Num() != 4 || !bPalettePresent)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Level/palette set changed."));
		return 2;
	}

	UGridItemSystemAsset* ItemRoot =
		LoadObject<UGridItemSystemAsset>(nullptr, CanonicalRoot);
	if (!IsValid(ItemRoot))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Real canonical root missing."));
		return 3;
	}
	TMap<FName, const UGridCanonicalItemDefinitionAsset*> Canonical;
	int32 InvalidCanonical = 0;
	for (const UGridCanonicalItemDefinitionAsset* Definition : ItemRoot->ItemDefinitions)
	{
		if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
			Canonical.Contains(Definition->ItemId))
		{
			++InvalidCanonical;
			continue;
		}
		Canonical.Add(Definition->ItemId, Definition);
	}
	if (InvalidCanonical != 0 || Canonical.Num() != 23)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Canonical item registry incomplete."));
		return 3;
	}
	const bool bRootInitiallyDirty = ItemRoot->GetOutermost()->IsDirty();
	TMap<FString, FReferenceCounts> Counters;
	TArray<TSharedPtr<FJsonValue>> PrivateRows;
	TSet<FGuid> PlacementIds;
	int32 DuplicateOrInvalidPlacementIds = 0;
	int32 LevelCount = 0;
	int32 PaletteEntryCount = 0;
	int32 WorldDefinitions = 0;
	int32 DefinitionReceptacleEntries = 0;

	auto Observe = [&](const FString& Category, const FString& Owner,
		const UGridItemDefinitionAsset* Legacy, int32 Level, int32 Quantity,
		const FString& PlacementId)
	{
		const FName ItemId = IsValid(Legacy) ? Legacy->ItemDefinitionId : NAME_None;
		FString Status = TEXT("ID_CANDIDATE_ONLY");
		if (!IsValid(Legacy)) { Status = TEXT("NO_LEGACY_DEFINITION"); }
		else if (ItemId.IsNone()) { Status = TEXT("EMPTY_LEGACY_ID"); }
		else if (Level < 0 || Level > 20 || Quantity < 1)
		{
			Status = TEXT("INVALID_LEVEL_OR_QUANTITY");
		}
		else if (!Canonical.Contains(ItemId)) { Status = TEXT("NO_CANONICAL_ID"); }
		else if (!Legacy->WeaponArchetypeId.IsNone())
		{
			const UGridArmamentDefinitionAsset* Weapon =
				Cast<UGridArmamentDefinitionAsset>(Canonical[ItemId]);
			if (!Weapon || Weapon->ArchetypeId != Legacy->WeaponArchetypeId)
			{
				Status = TEXT("ARCHETYPE_MISMATCH");
			}
		}
		AddCount(Counters.FindOrAdd(Category), Status);
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("category"), Category);
		Row->SetStringField(TEXT("owner"), Owner);
		Row->SetStringField(TEXT("placement_guid"), PlacementId);
		Row->SetStringField(TEXT("source_definition"),
			IsValid(Legacy) ? Legacy->GetPathName() : FString());
		Row->SetStringField(TEXT("item_id"), ItemId.ToString());
		Row->SetNumberField(TEXT("item_level"), Level);
		Row->SetNumberField(TEXT("quantity"), Quantity);
		Row->SetStringField(TEXT("status"), Status);
		PrivateRows.Add(MakeShared<FJsonValueObject>(Row));
	};

	auto CheckGuid = [&](const FGuid& Guid)
	{
		if (!Guid.IsValid() || PlacementIds.Contains(Guid))
		{
			++DuplicateOrInvalidPlacementIds;
		}
		else
		{
			PlacementIds.Add(Guid);
		}
	};

	for (const FString& Relative : LevelFiles)
	{
		const FString Path = ObjectPathFromRelative(Relative);
		UGridLevelAsset* Level = LoadObject<UGridLevelAsset>(nullptr, *Path);
		if (!IsValid(Level))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Cannot load verified grid level."));
			return 4;
		}
		++LevelCount;
		const bool bWasDirty = Level->GetOutermost()->IsDirty();
		for (const FGridLooseItemInstance& Loose : Level->LooseItemInstances)
		{
			CheckGuid(Loose.InstanceId);
			Observe(TEXT("LEVEL_LOOSE"), Path, Loose.ItemDefinition, Loose.ItemLevel,
				Loose.Quantity, Loose.InstanceId.ToString());
		}
		for (const FGridItemSpawnInstance& Spawn : Level->ItemSpawns)
		{
			CheckGuid(Spawn.SpawnId);
			Observe(TEXT("LEVEL_SPAWN"), Path, Spawn.ItemDefinition, Spawn.ItemLevel,
				Spawn.Quantity, Spawn.SpawnId.ToString());
		}
		for (const FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
		{
			CheckGuid(Object.InstanceId);
			for (const FGridReceptacleInitialItemConfig& Item :
				Object.InstanceConfig.ReceptacleInitialContent)
			{
				Observe(TEXT("LEVEL_RECEPTACLE"), Path,
					Item.ItemDefinition, Item.ItemLevel, Item.Quantity,
					Object.InstanceId.ToString());
			}
		}
		if (Level->GetOutermost()->IsDirty() != bWasDirty)
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Grid level dirty state mutated."));
			return 5;
		}
	}

	const FString PalettePath = ObjectPathFromRelative(PaletteRelative);
	UGridObjectPaletteAsset* Palette = LoadObject<UGridObjectPaletteAsset>(
		nullptr, *PalettePath);
	if (!IsValid(Palette)) { return 4; }
	const bool bPaletteWasDirty = Palette->GetOutermost()->IsDirty();
	for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
	{
		++PaletteEntryCount;
		if (Entry.DefaultItemDefinition)
		{
			Observe(TEXT("PALETTE_DIRECT"), PalettePath,
				Entry.DefaultItemDefinition, 0, 1, Entry.EntryId.ToString());
		}
	}
	if (Palette->GetOutermost()->IsDirty() != bPaletteWasDirty) { return 5; }

	// Include default contents from real world-object definitions, not just
	// the sparse instance overrides. These are NOT among 06.4G's 84 files.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(
		TEXT("/Game/GrimrockPrototype/Core/DataAssets")));
	Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Definitions;
	Registry.GetAssets(Filter, Definitions);
	Definitions.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.LexicalLess(B.PackageName);
	});
	for (const FAssetData& Asset : Definitions)
	{
		const UGridWorldObjectDefinitionAsset* Definition =
			Cast<UGridWorldObjectDefinitionAsset>(Asset.GetAsset());
		if (!IsValid(Definition)) { return 4; }
		++WorldDefinitions;
		const bool bWasDirty = Definition->GetOutermost()->IsDirty();
		for (const FGridReceptacleInitialItemConfig& Item :
			Definition->DefaultBehavior.Receptacle.InitialContent)
		{
			++DefinitionReceptacleEntries;
			Observe(TEXT("DEFINITION_RECEPTACLE"), Definition->GetPathName(),
				Item.ItemDefinition, Item.ItemLevel, Item.Quantity, FString());
		}
		if (Definition->GetOutermost()->IsDirty() != bWasDirty) { return 5; }
	}

	// SaveGame schema only: never load or mutate private *.sav files.
	const bool bPersistentIdentityPresent =
		IsSaveProperty(FGridRuntimeItemState::StaticStruct(),
			GET_MEMBER_NAME_CHECKED(FGridRuntimeItemState, ObjectId)) &&
		IsSaveProperty(FGridRuntimeItemState::StaticStruct(),
			GET_MEMBER_NAME_CHECKED(FGridRuntimeItemState, ItemDefinitionId)) &&
		IsSaveProperty(FGridRuntimeItemState::StaticStruct(),
			GET_MEMBER_NAME_CHECKED(FGridRuntimeItemState, ItemLevel));
	const bool bLegacyProjectilePointer =
		IsSaveProperty(FGridRuntimeItemState::StaticStruct(),
			GET_MEMBER_NAME_CHECKED(FGridRuntimeItemState, ProjectileDefinitionAsset));
	const bool bLegacyPendingPointer =
		IsSaveProperty(FGridPendingWorldItemState::StaticStruct(),
			GET_MEMBER_NAME_CHECKED(FGridPendingWorldItemState, ItemDefinitionAsset));
	if (!bPersistentIdentityPresent || !bLegacyProjectilePointer ||
		!bLegacyPendingPointer || ItemRoot->GetOutermost()->IsDirty() != bRootInitiallyDirty)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Reflected SaveGame source schema changed."));
		return 5;
	}

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4H/real-references/v1"));
	Root->SetNumberField(TEXT("levels_scanned"), LevelCount);
	Root->SetNumberField(TEXT("palette_entries"), PaletteEntryCount);
	Root->SetNumberField(TEXT("world_definitions_scanned"), WorldDefinitions);
	Root->SetNumberField(TEXT("definition_receptacle_entries"), DefinitionReceptacleEntries);
	Root->SetNumberField(TEXT("canonical_definitions"), Canonical.Num());
	Root->SetNumberField(TEXT("invalid_or_duplicate_placement_guids"), DuplicateOrInvalidPlacementIds);
	Root->SetNumberField(TEXT("savegame_legacy_pointer_fields"), 2);
	Root->SetBoolField(TEXT("savegame_files_opened"), false);
	Root->SetBoolField(TEXT("migration_authorized"), false);
	TSharedRef<FJsonObject> CountsJson = MakeShared<FJsonObject>();
	for (const TPair<FString, FReferenceCounts>& Entry : Counters)
	{
		const FReferenceCounts& C = Entry.Value;
		TSharedRef<FJsonObject> Fields = MakeShared<FJsonObject>();
		Fields->SetNumberField(TEXT("total"), C.Total);
		Fields->SetNumberField(TEXT("id_candidates"), C.Candidate);
		Fields->SetNumberField(TEXT("unmapped"), C.Unmapped);
		Fields->SetNumberField(TEXT("empty_ids"), C.EmptyId);
		Fields->SetNumberField(TEXT("missing_definition"), C.Missing);
		Fields->SetNumberField(TEXT("invalid_level_or_quantity"), C.InvalidLevelOrQuantity);
		Fields->SetNumberField(TEXT("archetype_mismatch"), C.ArchetypeMismatch);
		CountsJson->SetObjectField(Entry.Key, Fields);
	}
	Root->SetObjectField(TEXT("reference_counts"), CountsJson);
	Root->SetArrayField(TEXT("reference_rows"), PrivateRows);
	FString Text;
	if (!FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Text)) ||
		!FFileHelper::SaveStringToFile(Text, *Output,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4H] Private report write failed."));
		return 6;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4H] No assets or saves modified."));
	return 0;
}
