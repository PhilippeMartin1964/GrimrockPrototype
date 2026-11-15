#include "ARMEMENT/GridArmClean064JNonArmProjectionCommandlet.h"

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
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* RootPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
	const FString LegacyRoot(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

	TSharedPtr<FJsonObject> ReadJson(const FString& Filename)
	{
		FString Content;
		TSharedPtr<FJsonObject> Parsed;
		if (!FFileHelper::LoadFileToString(Content, *Filename) ||
			!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Content), Parsed))
		{
			return nullptr;
		}
		return Parsed;
	}

	FString StringField(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Key)
	{
		FString Value;
		if (Obj.IsValid()) { Obj->TryGetStringField(Key, Value); }
		return Value;
	}

	bool ExactLegacyProperties(const UGridItemDefinitionAsset& Legacy,
		const TSharedPtr<FJsonObject>& Recorded)
	{
		if (!Recorded.IsValid()) { return false; }
		int32 Count = 0;
		for (TFieldIterator<FProperty> It(Legacy.GetClass()); It; ++It)
		{
			FString Export;
			It->ExportTextItem_Direct(Export,
				It->ContainerPtrToValuePtr<void>(&Legacy), nullptr,
				const_cast<UGridItemDefinitionAsset*>(&Legacy), PPF_None);
			FString Old;
			if (!Recorded->TryGetStringField(It->GetName(), Old) ||
				Old != Export)
			{
				return false;
			}
			++Count;
		}
		return Count >= 10 && Count == Recorded->Values.Num();
	}

	TSharedRef<FJsonObject> ExportObjectProperties(const UObject& Object)
	{
		TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> It(Object.GetClass()); It; ++It)
		{
			FString Export;
			It->ExportTextItem_Direct(Export,
				It->ContainerPtrToValuePtr<void>(&Object), nullptr,
				const_cast<UObject*>(&Object), PPF_None);
			Result->SetStringField(It->GetName(), Export);
		}
		return Result;
	}

	bool TagsEqual(const TArray<FName>& A, const TArray<FName>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index] != B[Index]) { return false; }
		}
		return true;
	}

	bool CommonFieldsEqual(const UGridItemDefinitionAsset& Old,
		const UGridNonArmamentDefinitionAsset& New)
	{
		return Old.ItemDefinitionId == New.ItemId &&
			Old.DisplayName.EqualTo(New.DisplayName) &&
			Old.Description.EqualTo(New.Description) &&
			FMath::IsFinite(Old.Weight) &&
			Old.Weight >= 0.0f &&
			FMath::IsNearlyEqual(Old.Weight, New.WeightKg, 0.0001f) &&
			TagsEqual(Old.ItemTags, New.Tags) &&
			Old.Icon.ToSoftObjectPath() == New.Icon.ToSoftObjectPath() &&
			Old.WorldMesh.ToSoftObjectPath() == New.WorldMesh.ToSoftObjectPath();
	}

	/** This is a recommendation for review, NOT a confirmed gameplay mapping.
	 * No ammunition category exists in EGridItemType; never derive AmmoKindId
	 * from filename/tags or silently create an ammo definition. */
	FString ChooseFamily(const UGridItemDefinitionAsset& Old)
	{
		switch (Old.ItemType)
		{
			case EGridItemType::Potion:
			case EGridItemType::Food:
				return TEXT("CONSUMABLE_CANDIDATE");
			case EGridItemType::Scroll:
				return Old.bProvidesQuickItemCombatAction
					? TEXT("CONSUMABLE_CANDIDATE")
					: TEXT("GENERIC_REVIEW");
			case EGridItemType::Armor:
			case EGridItemType::Jewelry:
				return TEXT("EQUIPMENT_CANDIDATE");
			case EGridItemType::Torch:
			case EGridItemType::Book:
			case EGridItemType::Key:
			case EGridItemType::Gem:
			case EGridItemType::Component:
			case EGridItemType::Quest:
			case EGridItemType::Misc:
				return TEXT("GENERIC_REVIEW");
			case EGridItemType::None:
			case EGridItemType::Weapon:
			case EGridItemType::Shield:
			default:
				return TEXT("UNCLASSIFIED_REVIEW");
		}
	}

	TSharedRef<FJsonObject> SerializeRow(const FName Id,
		const FString& SourcePath, const FString& Family,
		const UGridNonArmamentDefinitionAsset& Transient,
		const TArray<FString>& ExtraFields,
		const TArray<FString>& Blockers,
		const int32 Occurrences, const bool bCoreEqual)
	{
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("item_id"), Id.ToString());
		Row->SetStringField(TEXT("legacy_source"), SourcePath);
		Row->SetStringField(TEXT("candidate_family"), Family);
		Row->SetStringField(TEXT("transient_class"), Transient.GetClass()->GetPathName());
		Row->SetNumberField(TEXT("world_occurrences"), Occurrences);
		Row->SetBoolField(TEXT("common_fields_exact"), bCoreEqual);
		Row->SetBoolField(TEXT("authoring_approved"), false);
		Row->SetBoolField(TEXT("binary_writes_allowed"), false);
		TArray<TSharedPtr<FJsonValue>> Unmapped;
		for (const FString& Field : ExtraFields)
		{
			Unmapped.Add(MakeShared<FJsonValueString>(Field));
		}
		Row->SetArrayField(TEXT("nondefault_unmapped_legacy_fields"), Unmapped);
		TArray<TSharedPtr<FJsonValue>> Pending;
		for (const FString& Blocker : Blockers)
		{
			Pending.Add(MakeShared<FJsonValueString>(Blocker));
		}
		Row->SetArrayField(TEXT("required_reviews"), Pending);
		Row->SetObjectField(TEXT("transient_projection_exact"), ExportObjectProperties(Transient));
		return Row;
	}
}

UGridArmClean064JNonArmProjectionCommandlet::UGridArmClean064JNonArmProjectionCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean064JNonArmProjectionCommandlet::Main(const FString& Params)
{
	FString Input;
	FString Output;
	if (!FParse::Value(*Params, TEXT("Input="), Input) ||
		!FParse::Value(*Params, TEXT("Output="), Output) ||
		Input.IsEmpty() || Output.IsEmpty() ||
		FPaths::IsSamePath(Input, Output))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4J] Distinct private input/output paths required."));
		return 1;
	}

	const TSharedPtr<FJsonObject> InputJson = ReadJson(Input);
	const TArray<TSharedPtr<FJsonValue>>* Missing = nullptr;
	if (!InputJson.IsValid() ||
		StringField(InputJson, TEXT("schema")) != TEXT("ARM-CLEAN06.4I/distinct-identity-gap/v1") ||
		!InputJson->TryGetArrayField(TEXT("missing_canonical_id_details"), Missing) ||
		!Missing || Missing->Num() != 11 ||
		InputJson->GetIntegerField(TEXT("world_references")) != 95 ||
		InputJson->GetIntegerField(TEXT("missing_canonical_occurrences")) != 40 ||
		InputJson->GetIntegerField(TEXT("missing_weapon_occurrences")) != 0 ||
		InputJson->GetIntegerField(TEXT("ids_with_conflicting_legacy_properties")) != 0 ||
		InputJson->GetBoolField(TEXT("migration_authorized")))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4J] Source identity report invalid."));
		return 2;
	}

	UGridItemSystemAsset* CanonicalRoot = LoadObject<UGridItemSystemAsset>(nullptr, RootPath);
	if (!IsValid(CanonicalRoot) || CanonicalRoot->ItemDefinitions.Num() != 23)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4J] Canonical registry count changed."));
		return 3;
	}
	TSet<FName> Registered;
	for (const UGridCanonicalItemDefinitionAsset* Entry : CanonicalRoot->ItemDefinitions)
	{
		if (!IsValid(Entry) || Entry->ItemId.IsNone() || Registered.Contains(Entry->ItemId))
		{
			return 3;
		}
		Registered.Add(Entry->ItemId);
	}
	const bool bRootDirty = CanonicalRoot->GetOutermost()->IsDirty();

	int32 Cons = 0, Equip = 0, Generic = 0, Unclassified = 0;
	int32 ProjectionMatches = 0;
	int32 ExtraFieldCount = 0;
	int32 MissingOccurrences = 0;
	int32 WithUnmappedNondefault = 0;
	int32 TotalBlockerCount = 0;
	int32 WithBehavioralBlockers = 0;
	int32 ChangedSourceDirty = 0;
	TSet<FName> Encountered;
	TArray<TSharedPtr<FJsonValue>> Rows;

	for (const TSharedPtr<FJsonValue>& Item : *Missing)
	{
		const TSharedPtr<FJsonObject> Entry = Item.IsValid() ? Item->AsObject() : nullptr;
		const FString OldPath = StringField(Entry, TEXT("representative_source"));
		const FString IdString = StringField(Entry, TEXT("item_id"));
		const FName ItemId(*IdString);
		const TSharedPtr<FJsonObject>* Recorded = nullptr;
		if (!Entry.IsValid() ||
			!Entry->TryGetObjectField(TEXT("source_fields_exact"), Recorded) ||
			!Recorded || !Recorded->IsValid() ||
			OldPath.IsEmpty() ||
			!OldPath.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
			OldPath.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
			OldPath.Contains(TEXT("..")) || OldPath.Contains(TEXT("\\")) ||
			!OldPath.Contains(TEXT(".")) ||
			ItemId.IsNone() || Encountered.Contains(ItemId) ||
			Registered.Contains(ItemId) ||
			Entry->GetBoolField(TEXT("source_conflict")) ||
			Entry->GetIntegerField(TEXT("distinct_legacy_assets")) != 1)
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4J] Private source scope invalid."));
			return 4;
		}
		Encountered.Add(ItemId);
		const int32 Occurrences = Entry->GetIntegerField(TEXT("world_occurrences"));
		if (Occurrences < 1 || Occurrences > 40) { return 4; }
		MissingOccurrences += Occurrences;

		const UGridItemDefinitionAsset* Old =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *OldPath);
		if (!IsValid(Old) || Old->ItemDefinitionId != ItemId ||
			!Old->WeaponArchetypeId.IsNone() ||
			!ExactLegacyProperties(*Old, *Recorded))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4J] Real UE source changed since 06.4I."));
			return 4;
		}
		const bool bOldDirty = Old->GetOutermost()->IsDirty();
		const FString Family = ChooseFamily(*Old);

		UGridNonArmamentDefinitionAsset* Projection = nullptr;
		TSet<FString> MappedFields;
		for (const FString& Field : {
			TEXT("ItemDefinitionId"), TEXT("DisplayName"), TEXT("Description"),
			TEXT("Weight"), TEXT("ItemTags"), TEXT("Icon"), TEXT("WorldMesh"),
			TEXT("ItemType")})
		{
			MappedFields.Add(Field);
		}
		TArray<FString> Blockers;
		if (Family == TEXT("CONSUMABLE_CANDIDATE"))
		{
			++Cons;
			UGridConsumableDefinitionAsset* Consumable =
				NewObject<UGridConsumableDefinitionAsset>(GetTransientPackage(), NAME_None, RF_Transient);
			Consumable->MaxStack = Old->bStackable ? Old->MaxStackSize : 1;
			if (Consumable->MaxStack < 1) { return 4; }
			MappedFields.Add(TEXT("bStackable"));
			MappedFields.Add(TEXT("MaxStackSize"));
			if (Old->bProvidesQuickItemCombatAction)
			{
				Blockers.Add(TEXT("QUICK_ACTION_ID_AND_EFFECTS_REQUIRE_EXPLICIT_BINDING"));
			}
			Blockers.Add(TEXT("CONSUMPTION_POLICY_REVIEW"));
			Projection = Consumable;
		}
		else if (Family == TEXT("EQUIPMENT_CANDIDATE"))
		{
			++Equip;
			UGridOtherEquipmentDefinitionAsset* Equipment =
				NewObject<UGridOtherEquipmentDefinitionAsset>(GetTransientPackage(), NAME_None, RF_Transient);
			Equipment->EquipmentSlots = Old->CompatibleEquipmentSlots;
			Equipment->Bonuses = Old->EquipmentStatBonus;
			MappedFields.Add(TEXT("CompatibleEquipmentSlots"));
			MappedFields.Add(TEXT("EquipmentStatBonus"));
			Blockers.Add(TEXT("EQUIPMENT_SLOT_AND_RESISTANCE_POLICY_REVIEW"));
			Projection = Equipment;
		}
		else
		{
			if (Family == TEXT("GENERIC_REVIEW")) { ++Generic; }
			else { ++Unclassified; }
			Projection = NewObject<UGridNonArmamentDefinitionAsset>(
				GetTransientPackage(), NAME_None, RF_Transient);
			Blockers.Add(TEXT("SPECIALIZED_INTERACTION_POLICY_REVIEW"));
		}
		if (!IsValid(Projection)) { return 4; }
		Projection->ItemId = Old->ItemDefinitionId;
		Projection->DisplayName = Old->DisplayName;
		Projection->Description = Old->Description;
		Projection->WeightKg = Old->Weight;
		Projection->Tags = Old->ItemTags;
		Projection->Icon = Old->Icon;
		Projection->WorldMesh = Old->WorldMesh;

		const bool bCoreEqual = CommonFieldsEqual(*Old, *Projection);
		if (bCoreEqual) { ++ProjectionMatches; }

		// Any old UPROPERTY departing from its class default and lacking a
		// known destination is treated as a blocking semantic difference.
		const UObject* Defaults = Old->GetClass()->GetDefaultObject();
		TArray<FString> UnmappedNondefault;
		for (TFieldIterator<FProperty> It(Old->GetClass()); It; ++It)
		{
			if (!MappedFields.Contains(It->GetName()) &&
				!It->Identical_InContainer(Old, Defaults))
			{
				UnmappedNondefault.Add(It->GetName());
			}
		}
		UnmappedNondefault.Sort();
		ExtraFieldCount += UnmappedNondefault.Num();
		if (!UnmappedNondefault.IsEmpty())
		{
			++WithUnmappedNondefault;
			Blockers.Add(TEXT("NONDEFAULT_LEGACY_PROPERTIES_NEED_OWNER"));
		}
		// These gates remain mandatory even when all legacy values happen
		// to equal their class defaults: no accidental physics or SaveGame
		// claims based on a metadata-only memory projection.
		Blockers.Add(TEXT("THROW_AND_CHAOS_PARITY_NOT_PROVEN"));
		Blockers.Add(TEXT("WORLD_RECEPTACLE_REFERENCE_CUTOVER_NOT_PROVEN"));
		Blockers.Add(TEXT("SAVEGAME_SCHEMA_AND_PERSONAL_SAVES_NOT_PROVEN"));
		Blockers.Add(TEXT("RUNTIME_INTERACTION_AND_PIE_NOT_PROVEN"));
		TotalBlockerCount += Blockers.Num();
		if (!Blockers.IsEmpty()) { ++WithBehavioralBlockers; }

		Rows.Add(MakeShared<FJsonValueObject>(SerializeRow(ItemId,
			OldPath, Family, *Projection, UnmappedNondefault,
			Blockers, Occurrences, bCoreEqual)));

		if (Old->GetOutermost()->IsDirty() != bOldDirty) { ++ChangedSourceDirty; }
	}

	if (Encountered.Num() != 11 || MissingOccurrences != 40 ||
		ProjectionMatches != 11 ||
		Cons + Equip + Generic + Unclassified != 11 ||
		WithBehavioralBlockers != 11 ||
		ChangedSourceDirty != 0 ||
		CanonicalRoot->GetOutermost()->IsDirty() != bRootDirty)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4J] Transient projection integrity failure."));
		return 5;
	}

	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4J/nonarm-dryrun/v1"));
	Report->SetNumberField(TEXT("unique_missing_item_ids"), Encountered.Num());
	Report->SetNumberField(TEXT("missing_world_occurrences"), MissingOccurrences);
	Report->SetNumberField(TEXT("transient_common_field_projections_equal"), ProjectionMatches);
	Report->SetNumberField(TEXT("consumable_candidates"), Cons);
	Report->SetNumberField(TEXT("equipment_candidates"), Equip);
	Report->SetNumberField(TEXT("generic_review_candidates"), Generic);
	Report->SetNumberField(TEXT("unclassified_candidates"), Unclassified);
	Report->SetNumberField(TEXT("ids_with_unmapped_nondefault_properties"), WithUnmappedNondefault);
	Report->SetNumberField(TEXT("unmapped_nondefault_property_occurrences"), ExtraFieldCount);
	Report->SetNumberField(TEXT("ids_with_remaining_behavioral_gates"), WithBehavioralBlockers);
	Report->SetNumberField(TEXT("total_behavioral_gate_entries"), TotalBlockerCount);
	Report->SetNumberField(TEXT("original_package_dirty_state_changes"), ChangedSourceDirty);
	Report->SetBoolField(TEXT("transient_only"), true);
	Report->SetBoolField(TEXT("migration_authorized"), false);
	Report->SetBoolField(TEXT("real_savegames_opened"), false);
	Report->SetArrayField(TEXT("candidate_details_private"), Rows);
	FString Json;
	if (!FJsonSerializer::Serialize(Report,
		TJsonWriterFactory<>::Create(&Json)) ||
		!FFileHelper::SaveStringToFile(Json, *Output,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4J] Private output failed."));
		return 6;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4J] Transient projections completed, cutover blocked."));
	return 0;
}
