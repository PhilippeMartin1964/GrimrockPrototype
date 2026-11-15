#include "ARMEMENT/GridArmClean064LNonArmPresentationParityCommandlet.h"

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
	const TCHAR* CanonicalPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
	const FString LegacyRoot(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

	TSharedPtr<FJsonObject> ReadJson(const FString& Path)
	{
		FString Content;
		TSharedPtr<FJsonObject> Value;
		if (!FFileHelper::LoadFileToString(Content, *Path) ||
			!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Content), Value))
		{
			return nullptr;
		}
		return Value;
	}

	FString FieldString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name)
	{
		FString Value;
		if (Object.IsValid()) { Object->TryGetStringField(Name, Value); }
		return Value;
	}

	FString Export(const FProperty& Property, const UObject& Owner)
	{
		FString Result;
		Property.ExportTextItem_Direct(Result,
			Property.ContainerPtrToValuePtr<void>(&Owner), nullptr,
			const_cast<UObject*>(&Owner), PPF_None);
		return Result;
	}

	const TArray<FString>& PresentationFields()
	{
		static const TArray<FString> Names = {
			TEXT("LightEmitter"), TEXT("VisualEffects"),
			TEXT("bEnableWorldSparkle"), TEXT("WorldSparkleMaterial"),
			TEXT("WorldSparkleColor"), TEXT("WorldSparkleIntensity"),
			TEXT("WorldSparkleSpeed"), TEXT("WorldSparkleVariation")
		};
		return Names;
	}

	bool IsPresentationDomain(const FString& Domain)
	{
		return Domain == TEXT("LIGHT") || Domain == TEXT("PERSISTENT_VISUAL");
	}

	/** Copy from the real legacy property into a strictly transient *canonical*
	 * non-armament object. A matching name is never enough: require the exact
	 * reflected type, identical value and identical exported representation.
	 * This is DATA parity only; it does not establish runtime effect parity. */
	bool CopyAndCompare(const UGridItemDefinitionAsset& Source,
		UGridNonArmamentDefinitionAsset& Candidate, const FString& Name)
	{
		const FProperty* Old = Source.GetClass()->FindPropertyByName(FName(*Name));
		const FProperty* New = Candidate.GetClass()->FindPropertyByName(FName(*Name));
		if (!Old || !New || !New->SameType(Old)) { return false; }
		const void* SourcePtr = Old->ContainerPtrToValuePtr<void>(&Source);
		void* CandidatePtr = New->ContainerPtrToValuePtr<void>(&Candidate);
		New->CopyCompleteValue(CandidatePtr, SourcePtr);
		return New->Identical(CandidatePtr, SourcePtr) &&
			Export(*Old, Source) == Export(*New, Candidate);
	}
}

UGridArmClean064LNonArmPresentationParityCommandlet::
	UGridArmClean064LNonArmPresentationParityCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean064LNonArmPresentationParityCommandlet::Main(const FString& Params)
{
	FString OwnershipPath;
	FString OutputPath;
	if (!FParse::Value(*Params, TEXT("Ownership="), OwnershipPath) ||
		!FParse::Value(*Params, TEXT("Output="), OutputPath) ||
		OwnershipPath.IsEmpty() || OutputPath.IsEmpty() ||
		FPaths::IsSamePath(OwnershipPath, OutputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] Distinct private input/output required."));
		return 1;
	}
	const TSharedPtr<FJsonObject> Ownership = ReadJson(OwnershipPath);
	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (!Ownership.IsValid() ||
		FieldString(Ownership, TEXT("schema")) != TEXT("ARM-CLEAN06.4K/field-ownership/v1") ||
		!Ownership->TryGetArrayField(TEXT("private_item_ownership"), Items) ||
		!Items || Items->Num() != 11 ||
		Ownership->GetIntegerField(TEXT("verified_world_occurrences")) != 40 ||
		Ownership->GetIntegerField(TEXT("active_unmapped_property_occurrences")) != 50 ||
		Ownership->GetIntegerField(TEXT("distinct_active_unmapped_property_names")) != 26 ||
		Ownership->GetIntegerField(TEXT("field_semantic_parity_approved")) != 0 ||
		Ownership->GetBoolField(TEXT("migration_authorized")))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] 06.4K ownership contract drift."));
		return 2;
	}
	const TSharedPtr<FJsonObject>* Domains = nullptr;
	if (!Ownership->TryGetObjectField(TEXT("domain_counts"), Domains) || !Domains ||
		(*Domains)->GetIntegerField(TEXT("LIGHT")) != 1 ||
		(*Domains)->GetIntegerField(TEXT("PERSISTENT_VISUAL")) != 11)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] Presentation domain count drift."));
		return 2;
	}

	UGridItemSystemAsset* Registry = LoadObject<UGridItemSystemAsset>(nullptr, CanonicalPath);
	if (!IsValid(Registry) || Registry->ItemDefinitions.Num() != 23) { return 3; }
	TSet<FName> Registered;
	for (const UGridCanonicalItemDefinitionAsset* Definition : Registry->ItemDefinitions)
	{
		if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
			Registered.Contains(Definition->ItemId)) { return 3; }
		Registered.Add(Definition->ItemId);
	}
	const bool RegistryDirty = Registry->GetOutermost()->IsDirty();
	TSet<FName> Seen;
	int32 Occurrences = 0;
	int32 ActiveFields = 0;
	int32 ActiveLight = 0;
	int32 ActiveVisual = 0;
	int32 PropertyCopies = 0;
	int32 AllFields = 0;
	int32 DirtyChanges = 0;
	TArray<TSharedPtr<FJsonValue>> PrivateRows;
	for (const TSharedPtr<FJsonValue>& RawItem : *Items)
	{
		const TSharedPtr<FJsonObject> Item = RawItem.IsValid() ? RawItem->AsObject() : nullptr;
		const FName ItemId(*FieldString(Item, TEXT("item_id")));
		const FString Path = FieldString(Item, TEXT("source_asset"));
		const TArray<TSharedPtr<FJsonValue>>* Reviews = nullptr;
		if (!Item.IsValid() || ItemId.IsNone() || Seen.Contains(ItemId) ||
			Registered.Contains(ItemId) || Item->GetBoolField(TEXT("conversion_authorized")) ||
			!Path.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
			Path.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
			Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")) ||
			!Path.Contains(TEXT(".")) ||
			!Item->TryGetArrayField(TEXT("field_reviews"), Reviews) || !Reviews ||
			Reviews->Num() != Item->GetIntegerField(TEXT("active_unmapped_fields")))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] Scope invalid."));
			return 4;
		}
		const UGridItemDefinitionAsset* Source =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
		if (!IsValid(Source) || Source->ItemDefinitionId != ItemId ||
			!Source->WeaponArchetypeId.IsNone()) { return 4; }
		const bool WasDirty = Source->GetOutermost()->IsDirty();
		Seen.Add(ItemId);
		Occurrences += Item->GetIntegerField(TEXT("affected_world_occurrences"));
		UGridNonArmamentDefinitionAsset* Candidate =
			NewObject<UGridNonArmamentDefinitionAsset>(GetTransientPackage(), NAME_None, RF_Transient);
		if (!IsValid(Candidate) || Candidate->GetOutermost() != GetTransientPackage()) { return 5; }
		Candidate->ItemId = ItemId;
		Candidate->DisplayName = Source->DisplayName;
		Candidate->Description = Source->Description;
		Candidate->WeightKg = Source->Weight;
		Candidate->Tags = Source->ItemTags;
		Candidate->Icon = Source->Icon;
		Candidate->WorldMesh = Source->WorldMesh;
		int32 ItemActive = 0;
		TSet<FString> SeenFields;
		for (const TSharedPtr<FJsonValue>& RawField : *Reviews)
		{
			const TSharedPtr<FJsonObject> Review = RawField.IsValid() ? RawField->AsObject() : nullptr;
			const FString Name = FieldString(Review, TEXT("property"));
			const FString Domain = FieldString(Review, TEXT("domain"));
			const FProperty* Property = Source->GetClass()->FindPropertyByName(FName(*Name));
			if (!Review.IsValid() || Name.IsEmpty() || SeenFields.Contains(Name) ||
				!Property || Export(*Property, *Source) != FieldString(Review, TEXT("source_value_exact")) ||
				FieldString(Review, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY") ||
				Domain.IsEmpty())
			{
				UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] Saved field value drift."));
				return 5;
			}
			SeenFields.Add(Name);
			++AllFields;
			if (IsPresentationDomain(Domain))
			{
				if (!PresentationFields().Contains(Name) ||
				(Domain == TEXT("LIGHT") && Name != TEXT("LightEmitter")) ||
				(Domain == TEXT("PERSISTENT_VISUAL") && Name == TEXT("LightEmitter")))
			{
					return 5;
				}
				++ActiveFields;
				++ItemActive;
				if (Domain == TEXT("LIGHT")) { ++ActiveLight; }
				else { ++ActiveVisual; }
			}
		}
		// Check *every* light/FX field, including historical defaults. A default
		// may affect torch startup, Niagara activation or Save/Continue.
		for (const FString& Name : PresentationFields())
		{
			if (!CopyAndCompare(*Source, *Candidate, Name))
			{
				UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] Transient field mismatch."));
				return 6;
			}
			++PropertyCopies;
		}
		if (Source->LightEmitter.HasEmitter() != Candidate->LightEmitter.HasEmitter() ||
			Source->LightEmitter.RequiresRuntimeTick() != Candidate->LightEmitter.RequiresRuntimeTick() ||
			Source->LightEmitter.IsValid() != Candidate->LightEmitter.IsValid() ||
			Source->VisualEffects.IsValid() != Candidate->VisualEffects.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] Light/FX pure-data behavior mismatch."));
			return 6;
		}
		if (WasDirty != Source->GetOutermost()->IsDirty()) { ++DirtyChanges; }
		TSharedRef<FJsonObject> PrivateItem = MakeShared<FJsonObject>();
		PrivateItem->SetStringField(TEXT("item_id"), ItemId.ToString());
		PrivateItem->SetStringField(TEXT("source_asset"), Path);
		PrivateItem->SetNumberField(TEXT("active_presentation_properties"), ItemActive);
		PrivateItem->SetNumberField(TEXT("exact_reflected_copies"), PresentationFields().Num());
		PrivateItem->SetBoolField(TEXT("data_parity"), true);
		PrivateItem->SetBoolField(TEXT("runtime_parity"), false);
		PrivateRows.Add(MakeShared<FJsonValueObject>(PrivateItem));
	}
	if (Seen.Num() != 11 || Occurrences != 40 || AllFields != 50 ||
		ActiveFields != 12 || ActiveLight != 1 || ActiveVisual != 11 ||
		PropertyCopies != 88 || DirtyChanges != 0 ||
		RegistryDirty != Registry->GetOutermost()->IsDirty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] Exact scope or dirty invariant failed."));
		return 7;
	}
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4L/nonarm-presentation-data-parity/v1"));
	Report->SetNumberField(TEXT("verified_item_ids"), Seen.Num());
	Report->SetNumberField(TEXT("preserved_world_occurrences"), Occurrences);
	Report->SetNumberField(TEXT("source_active_fields_reviewed"), AllFields);
	Report->SetNumberField(TEXT("active_light_fields"), ActiveLight);
	Report->SetNumberField(TEXT("active_persistent_visual_fields"), ActiveVisual);
	Report->SetNumberField(TEXT("exact_reflected_copies_including_defaults"), PropertyCopies);
	Report->SetNumberField(TEXT("source_dirty_state_changes"), DirtyChanges);
	Report->SetNumberField(TEXT("runtime_semantic_parity_approved"), 0);
	Report->SetBoolField(TEXT("binary_writes_performed"), false);
	Report->SetBoolField(TEXT("savegame_files_opened"), false);
	Report->SetBoolField(TEXT("migration_authorized"), false);
	Report->SetArrayField(TEXT("private_item_projections"), PrivateRows);
	FString Json;
	if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)) ||
		!FFileHelper::SaveStringToFile(Json, *OutputPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4L] Private report write failed."));
		return 8;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4L] 12 active light/visual fields preserved in 88 transient reflected copies. No binary migration."));
	return 0;
}
