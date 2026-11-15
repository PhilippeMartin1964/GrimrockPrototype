#include "ARMEMENT/GridArmClean064IIdentityGapCommandlet.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
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
	const FString LegacyPrefix(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

	TSharedPtr<FJsonObject> LoadJson(const FString& Path)
	{
		FString Text;
		TSharedPtr<FJsonObject> Json;
		return FFileHelper::LoadFileToString(Text, *Path) &&
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json)
			? Json : nullptr;
	}

	FString GetString(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key)
	{
		FString Value;
		if (Json.IsValid()) { Json->TryGetStringField(Key, Value); }
		return Value;
	}

	TMap<FString, FString> ExportProperties(const UGridItemDefinitionAsset& Definition)
	{
		TMap<FString, FString> Result;
		for (TFieldIterator<FProperty> Prop(Definition.GetClass()); Prop; ++Prop)
		{
			FString Export;
			Prop->ExportTextItem_Direct(Export,
				Prop->ContainerPtrToValuePtr<void>(&Definition), nullptr,
				const_cast<UGridItemDefinitionAsset*>(&Definition), PPF_None);
			Result.Add(Prop->GetName(), MoveTemp(Export));
		}
		return Result;
	}

	bool IdenticalProperties(const TMap<FString, FString>& A,
		const TMap<FString, FString>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (const TPair<FString, FString>& Entry : A)
		{
			const FString* Other = B.Find(Entry.Key);
			if (!Other || *Other != Entry.Value) { return false; }
		}
		return true;
	}

	TSharedRef<FJsonObject> ToJson(const TMap<FString, FString>& Properties)
	{
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		TArray<FString> Names;
		Properties.GetKeys(Names);
		Names.Sort();
		for (const FString& Name : Names)
		{
			Data->SetStringField(Name, Properties.FindChecked(Name));
		}
		return Data;
	}

	bool SameNameSequence(const TArray<FName>& A, const TArray<FName>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (int32 I = 0; I < A.Num(); ++I)
		{
			if (A[I] != B[I]) { return false; }
		}
		return true;
	}

	// Deliberately limited to identity + metadata. These comparisons do
	// NOT assert physics, action, presentation or SaveGame parity.
	TArray<FString> MetadataDifferences(const UGridItemDefinitionAsset& Old,
		const UGridCanonicalItemDefinitionAsset& New)
	{
		TArray<FString> Differences;
		if (!Old.DisplayName.EqualTo(New.DisplayName))
		{
			Differences.Add(TEXT("DisplayName"));
		}
		if (!Old.Description.EqualTo(New.Description))
		{
			Differences.Add(TEXT("Description"));
		}
		if (!FMath::IsFinite(Old.Weight) ||
			!FMath::IsNearlyEqual(Old.Weight, New.WeightKg, 0.0001f))
		{
			Differences.Add(TEXT("Weight"));
		}
		if (!SameNameSequence(Old.ItemTags, New.Tags))
		{
			Differences.Add(TEXT("Tags"));
		}
		const bool bOldWeapon = !Old.WeaponArchetypeId.IsNone();
		const UGridArmamentDefinitionAsset* NewWeapon =
			Cast<UGridArmamentDefinitionAsset>(&New);
		if (bOldWeapon != (NewWeapon != nullptr) ||
			(bOldWeapon && (!NewWeapon ||
				Old.WeaponArchetypeId != NewWeapon->ArchetypeId)))
		{
			Differences.Add(TEXT("WeaponKindOrArchetype"));
		}
		if (!bOldWeapon)
		{
			const UGridNonArmamentDefinitionAsset* NewItem =
				Cast<UGridNonArmamentDefinitionAsset>(&New);
			if (!NewItem ||
				Old.Icon.ToSoftObjectPath() != NewItem->Icon.ToSoftObjectPath())
			{
				Differences.Add(TEXT("Icon"));
			}
			if (!NewItem ||
				Old.WorldMesh.ToSoftObjectPath() != NewItem->WorldMesh.ToSoftObjectPath())
			{
				Differences.Add(TEXT("WorldMesh"));
			}
		}
		return Differences;
	}

	bool IsValidLegacyPath(const FString& ObjectPath)
	{
		return ObjectPath.StartsWith(LegacyPrefix, ESearchCase::CaseSensitive) &&
			!ObjectPath.Contains(TEXT("..")) &&
			ObjectPath.Contains(TEXT(".")) &&
			!ObjectPath.Contains(TEXT("\\")) &&
			ObjectPath.Len() < 512;
	}
}

UGridArmClean064IIdentityGapCommandlet::UGridArmClean064IIdentityGapCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean064IIdentityGapCommandlet::Main(const FString& Params)
{
	FString InputPath;
	FString OutputPath;
	if (!FParse::Value(*Params, TEXT("Input="), InputPath) ||
		!FParse::Value(*Params, TEXT("Output="), OutputPath) ||
		InputPath.IsEmpty() || OutputPath.IsEmpty() ||
		FPaths::IsSamePath(InputPath, OutputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Distinct private paths required."));
		return 1;
	}
	const TSharedPtr<FJsonObject> Input = LoadJson(InputPath);
	const TArray<TSharedPtr<FJsonValue>>* References = nullptr;
	if (!Input.IsValid() ||
		GetString(Input, TEXT("schema")) != TEXT("ARM-CLEAN06.4H/real-references/v1") ||
		!Input->TryGetArrayField(TEXT("reference_rows"), References) ||
		!References || References->Num() != 95 ||
		Input->GetIntegerField(TEXT("levels_scanned")) != 4 ||
		Input->GetIntegerField(TEXT("canonical_definitions")) != 23 ||
		Input->GetBoolField(TEXT("migration_authorized")))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Prior UE evidence invalid."));
		return 2;
	}
	UGridItemSystemAsset* Registry = LoadObject<UGridItemSystemAsset>(nullptr, CanonicalRoot);
	if (!IsValid(Registry))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Canonical registry missing."));
		return 3;
	}
	TMap<FName, const UGridCanonicalItemDefinitionAsset*> Definitions;
	for (const UGridCanonicalItemDefinitionAsset* Definition : Registry->ItemDefinitions)
	{
		if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
			Definitions.Contains(Definition->ItemId))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Duplicate/invalid canonical identity."));
			return 3;
		}
		Definitions.Add(Definition->ItemId, Definition);
	}
	if (Definitions.Num() != 23) { return 3; }
	const bool bRootDirty = Registry->GetOutermost()->IsDirty();

	TMap<FName, TMap<FString, FString>> FirstProperties;
	TMap<FName, FString> FirstSourcePath;
	TMap<FName, int32> UnmappedOccurrences;
	TMap<FName, TSet<FString>> UnmappedSourcePaths;
	TMap<FName, int32> CandidateOccurrences;
	TSet<FName> ConflictedIds;
	TSet<FName> CandidateMetadataMismatchIds;
	TMap<FName, TSet<FString>> CandidateMismatchFields;
	TArray<TSharedPtr<FJsonValue>> ReferenceRows;
	int32 Unmapped = 0;
	int32 Candidate = 0;
	int32 N0 = 0;
	int32 FixedLevel = 0;
	int32 ChangedSources = 0;
	int32 UnmappedArmed = 0;
	int32 UnmappedNonArmed = 0;
	int32 MissingSources = 0;

	for (const TSharedPtr<FJsonValue>& Value : *References)
	{
		const TSharedPtr<FJsonObject> Source = Value.IsValid() ? Value->AsObject() : nullptr;
		const FString Status = GetString(Source, TEXT("status"));
		const FString Path = GetString(Source, TEXT("source_definition"));
		const FString SerializedId = GetString(Source, TEXT("item_id"));
		if (!Source.IsValid() || !IsValidLegacyPath(Path) ||
			(Status != TEXT("ID_CANDIDATE_ONLY") &&
			 Status != TEXT("NO_CANONICAL_ID")))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Input scope/identity invalid."));
			return 4;
		}
		const UGridItemDefinitionAsset* Legacy =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
		if (!IsValid(Legacy) || Legacy->ItemDefinitionId.IsNone() ||
			Legacy->ItemDefinitionId.ToString() != SerializedId)
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Historical source no longer matches."));
			return 4;
		}
		int32 Level = 0;
		int32 Quantity = 0;
		if (!Source->TryGetNumberField(TEXT("item_level"), Level) ||
			!Source->TryGetNumberField(TEXT("quantity"), Quantity) ||
			Level < 0 || Level > 20 || Quantity < 1)
		{
			return 4;
		}
		if (Level == 0) { ++N0; } else { ++FixedLevel; }
		const FName ItemId = Legacy->ItemDefinitionId;
		const UGridCanonicalItemDefinitionAsset* const* Found = Definitions.Find(ItemId);
		if ((Status == TEXT("ID_CANDIDATE_ONLY")) != (Found != nullptr))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Canonical status changed since 06.4H."));
			return 4;
		}
		const bool bSourceDirty = Legacy->GetOutermost()->IsDirty();
		const TMap<FString, FString> Properties = ExportProperties(*Legacy);
		if (Properties.Num() < 10)
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Incomplete reflected old definition."));
			return 4;
		}
		if (const TMap<FString, FString>* First = FirstProperties.Find(ItemId))
		{
			if (!IdenticalProperties(*First, Properties))
			{
				ConflictedIds.Add(ItemId);
			}
		}
		else
		{
			FirstProperties.Add(ItemId, Properties);
			FirstSourcePath.Add(ItemId, Path);
		}
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("category"), GetString(Source, TEXT("category")));
		Row->SetStringField(TEXT("source_definition"), Path);
		Row->SetStringField(TEXT("item_id"), ItemId.ToString());
		Row->SetStringField(TEXT("placement_guid"), GetString(Source, TEXT("placement_guid")));
		Row->SetNumberField(TEXT("item_level"), Level);
		Row->SetNumberField(TEXT("quantity"), Quantity);
		Row->SetStringField(TEXT("status"), Status);
		if (Found)
		{
			++Candidate;
			CandidateOccurrences.FindOrAdd(ItemId)++;
			const TArray<FString> Differences = MetadataDifferences(*Legacy, **Found);
			if (!Differences.IsEmpty())
			{
				CandidateMetadataMismatchIds.Add(ItemId);
				TSet<FString>& Fields = CandidateMismatchFields.FindOrAdd(ItemId);
				for (const FString& Difference : Differences)
				{
					Fields.Add(Difference);
				}
			}
		}
		else
		{
			++Unmapped;
			UnmappedOccurrences.FindOrAdd(ItemId)++;
			UnmappedSourcePaths.FindOrAdd(ItemId).Add(Path);
			if (Legacy->WeaponArchetypeId.IsNone()) { ++UnmappedNonArmed; }
			else { ++UnmappedArmed; }
		}
		if (Legacy->GetOutermost()->IsDirty() != bSourceDirty)
		{
			++ChangedSources;
		}
		ReferenceRows.Add(MakeShared<FJsonValueObject>(Row));
	}
	if (Candidate != 55 || Unmapped != 40 || N0 + FixedLevel != 95 ||
		ChangedSources != 0 || Registry->GetOutermost()->IsDirty() != bRootDirty)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Live evidence/count drift."));
		return 5;
	}

	TArray<FName> UniqueMissingIds;
	UnmappedOccurrences.GetKeys(UniqueMissingIds);
	UniqueMissingIds.Sort(FNameLexicalLess());
	TArray<TSharedPtr<FJsonValue>> MissingRows;
	int32 MultipleSources = 0;
	for (const FName Id : UniqueMissingIds)
	{
		const FString* FirstPath = FirstSourcePath.Find(Id);
		const TMap<FString, FString>* Properties = FirstProperties.Find(Id);
		if (!FirstPath || !Properties) { return 5; }
		const int32 Sources = UnmappedSourcePaths.FindChecked(Id).Num();
		if (Sources > 1) { ++MultipleSources; }
		TSharedRef<FJsonObject> Missing = MakeShared<FJsonObject>();
		Missing->SetStringField(TEXT("item_id"), Id.ToString());
		Missing->SetStringField(TEXT("representative_source"), *FirstPath);
		Missing->SetNumberField(TEXT("world_occurrences"), UnmappedOccurrences.FindChecked(Id));
		Missing->SetNumberField(TEXT("distinct_legacy_assets"), Sources);
		Missing->SetBoolField(TEXT("source_conflict"), ConflictedIds.Contains(Id));
		Missing->SetObjectField(TEXT("source_fields_exact"), ToJson(*Properties));
		MissingRows.Add(MakeShared<FJsonValueObject>(Missing));
	}
	TArray<TSharedPtr<FJsonValue>> CandidateRows;
	TArray<FName> CandidateIds;
	CandidateOccurrences.GetKeys(CandidateIds);
	CandidateIds.Sort(FNameLexicalLess());
	for (const FName Id : CandidateIds)
	{
		TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
		Item->SetStringField(TEXT("item_id"), Id.ToString());
		Item->SetNumberField(TEXT("world_occurrences"), CandidateOccurrences.FindChecked(Id));
		Item->SetBoolField(TEXT("metadata_mismatch"), CandidateMetadataMismatchIds.Contains(Id));
		TArray<TSharedPtr<FJsonValue>> Fields;
		if (const TSet<FString>* Differences = CandidateMismatchFields.Find(Id))
		{
			TArray<FString> Sorted = Differences->Array();
			Sorted.Sort();
			for (const FString& Field : Sorted)
			{
				Fields.Add(MakeShared<FJsonValueString>(Field));
			}
		}
		Item->SetArrayField(TEXT("metadata_differences"), Fields);
		CandidateRows.Add(MakeShared<FJsonValueObject>(Item));
	}

	TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4I/distinct-identity-gap/v1"));
	Out->SetNumberField(TEXT("world_references"), 95);
	Out->SetNumberField(TEXT("canonical_candidate_occurrences"), Candidate);
	Out->SetNumberField(TEXT("missing_canonical_occurrences"), Unmapped);
	Out->SetNumberField(TEXT("unique_missing_item_ids"), UniqueMissingIds.Num());
	Out->SetNumberField(TEXT("unique_matching_item_ids"), CandidateIds.Num());
	Out->SetNumberField(TEXT("missing_ids_with_multiple_legacy_sources"), MultipleSources);
	Out->SetNumberField(TEXT("ids_with_conflicting_legacy_properties"), ConflictedIds.Num());
	Out->SetNumberField(TEXT("matching_ids_with_metadata_differences"),
		CandidateMetadataMismatchIds.Num());
	Out->SetNumberField(TEXT("N0_occurrences"), N0);
	Out->SetNumberField(TEXT("fixed_level_occurrences"), FixedLevel);
	Out->SetNumberField(TEXT("missing_weapon_occurrences"), UnmappedArmed);
	Out->SetNumberField(TEXT("missing_nonweapon_occurrences"), UnmappedNonArmed);
	Out->SetNumberField(TEXT("legacy_sources_dirty_state_changed"), ChangedSources);
	Out->SetBoolField(TEXT("migration_authorized"), false);
	Out->SetArrayField(TEXT("missing_canonical_id_details"), MissingRows);
	Out->SetArrayField(TEXT("existing_canonical_id_details"), CandidateRows);
	Out->SetArrayField(TEXT("verified_world_references"), ReferenceRows);
	FString Json;
	if (!FJsonSerializer::Serialize(Out, TJsonWriterFactory<>::Create(&Json)) ||
		!FFileHelper::SaveStringToFile(Json, *OutputPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4I] Private report could not be saved."));
		return 6;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4I] Distinct identity inventory completed read-only."));
	return 0;
}
